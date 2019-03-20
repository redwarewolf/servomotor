/*
 * planificacion.h

 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "configuraciones.h"
#include "pcb.h"
#include "conexionMemoria.h"
#include "contabilidad.h"
#include "semaforosAnsisop.h"
#include "comandosCPU.h"
#include "heap.h"
#include "conexionConsola.h"
#include "excepciones.h"
#include "listasAdministrativas.h"
#include "capaFilesystem.h"
#include "logs.h"

/*---PAUSA PLANIFICACION---*/


int flagTerminarPlanificadorLargoPlazo = 0;
int flagTerminarPlanificadorCortoPlazo=0;
int flagPlanificacion;
void verificarPausaPlanificacion();
void interfazReanudarPlanificacion();
void interfazPausarPlanificacion();
/*-------------------------*/

/*----LARGO PLAZO--------*/
void planificarLargoPlazo();
void administrarNuevosProcesos();
void administrarFinProcesos();
void crearProceso(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma);
t_codigoPrograma* buscarCodigoDeProceso(int pid);

void terminarProceso(t_pcb* proceso);void finalizarHiloPrograma(int pid);
void liberarRecursosEnMemoria(t_pcb* pcbProcesoTerminado);
void liberarMemoriaDinamica(int pid);
void cambiarEstadoATerminado(t_pcb* procesoTerminar);
void destruirTablaArchivosYActualizarTablaGlobal(int pid);

pthread_t planificadorLargoPlazo;
/*----LARGO PLAZO--------*/

/*----MEDIANO PLAZO--------*/

void planificarMedianoPlazo();
void pcbBloqueadoAReady();
void pcbEjecucionABloqueado();

pthread_t planificadorMedianoPlazo;

/*----MEDIANO PLAZO--------*/

/*----CORTO PLAZO--------*/

void planificarCortoPlazo();
void enviarConfiguracionesQuantum(int socketCPU);
void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex);
void finQuantumAReady();
void agregarAFinQuantum(t_pcb* pcb);

t_list* listaFinQuantum;

pthread_t planificadorCortoPlazo;
pthread_t threadFinQuantumAReady;

/*----CORTO PLAZO--------*/

/*---PLANIFICACION GENERAL-------*/
int obtenerPaginaSiguiente(int pid);
int verificarGradoDeMultiprogramacion();
void aumentarGradoMultiprogramacion();
void disminuirGradoMultiprogramacion();
void encolarProcesoListo(t_pcb *procesoListo);
void cargarConsola(int pid, int idConsola);
void gestionarFinProcesoCPU(int socket);

int contadorPid=0;
/*---PLANIFICACION GENERAL-------*/
void verificarPidColaSemaforos(int pid);
void signalHandler(int sigCode);

/*------------------------LARGO PLAZO-----------------------------------------*/
pthread_t administradorFinProcesos;
pthread_t administradorNuevosProcesos;

void planificarLargoPlazo(){
	pthread_create(&administradorFinProcesos,NULL, (void*)administrarFinProcesos,NULL);
	pthread_create(&administradorNuevosProcesos,NULL, (void*)administrarNuevosProcesos,NULL);

	signal(SIGUSR1,signalHandler);

	pthread_join(administradorNuevosProcesos,NULL);
	pthread_join(administradorFinProcesos,NULL);
	log_info(logKernel,"Planificador largo plazo finalizado");
}

void administrarNuevosProcesos(){
	t_pcb* proceso;
	while(!flagTerminarPlanificadorLargoPlazo){

		verificarPausaPlanificacion();

		sem_wait(&sem_admitirNuevoProceso);
		pthread_mutex_lock(&mutexNuevoProceso);
			if(verificarGradoDeMultiprogramacion()==0 && list_size(colaNuevos)>0 && flagPlanificacion) {

			pthread_mutex_lock(&mutexColaNuevos);
			proceso = list_remove(colaNuevos,0);
			pthread_mutex_unlock(&mutexColaNuevos);

			pthread_mutex_lock(&mutexListaCodigo);
			t_codigoPrograma* codigoPrograma = buscarCodigoDeProceso(proceso->pid);
			pthread_mutex_unlock(&mutexListaCodigo);

			crearProceso(proceso,codigoPrograma);
			}
			pthread_mutex_unlock(&mutexNuevoProceso);
	}
	log_info(logKernel,"Hilo administrador de nuevos procesos finalizado");
}


void crearProceso(t_pcb* proceso,t_codigoPrograma* codigoPrograma){
	log_info(logKernelPantalla,"Cambiando nuevo proceso desde Nuevos a Listos--->PID: %d",proceso->pid);
	pthread_mutex_lock(&mutexMemoria);
	if(inicializarProcesoEnMemoria(proceso,codigoPrograma) < 0 ){
		pthread_mutex_unlock(&mutexMemoria);
				log_error(logKernel ,"No se pudo reservar recursos para ejecutar el programa");
				excepcionReservaRecursos(codigoPrograma->socketHiloConsola,proceso);
			}
	else{
		pthread_mutex_unlock(&mutexMemoria);
		inicializarTablaProceso(proceso->pid);
			encolarProcesoListo(proceso);
			aumentarGradoMultiprogramacion();
			sem_post(&sem_procesoListo);
	}
	free(codigoPrograma);
}


int inicializarProcesoEnMemoria(t_pcb* proceso, t_codigoPrograma* codigoPrograma){
	log_info(logKernel, "Inicializando proceso en memoria--->PID: %d", proceso->pid);
	if((pedirMemoria(proceso))< 0){
				log_error(logKernel ,"Memoria no autorizo la solicitud de reserva--->PID:%d",proceso->pid);
				return -1;
			}
	if((almacenarCodigoEnMemoria(proceso,codigoPrograma->codigo,codigoPrograma->size))< 0){
					log_error(logKernel ,"Memoria no puede almacenar contenido--->PID:%d",proceso->pid);
					return -2;
				}
	return 0;
}

void administrarFinProcesos(){
	int indice;
	t_pcb* proceso;

	while(!flagTerminarPlanificadorLargoPlazo){
		sem_wait(&sem_administrarFinProceso);

				if(flagPlanificacion){
					pthread_mutex_lock(&mutexListaEspera);
						if(!list_is_empty(listaEspera)){

							for(indice = 0; indice< listaEspera->elements_count ; indice++){
								proceso=list_remove(listaEspera,indice);
								terminarProceso(proceso);
							}
						}
						pthread_mutex_unlock(&mutexListaEspera);
				}
	}
	log_info(logKernel,"Hilo administrador de fin de procesos finalizado");
}


t_codigoPrograma* buscarCodigoDeProceso(int pid){
	_Bool verificarPid(t_codigoPrograma* codigoPrograma){
			return (codigoPrograma->pid == pid);
		}

	return list_remove_by_condition(listaCodigosProgramas, (void*)verificarPid);

}

void terminarProceso(t_pcb* proceso){
	log_info(logKernelPantalla,"Terminando proceso--->PID:%d--->Exit Code:%d",proceso->pid,proceso->exitCode);
	log_info(logKernelPantalla,"Descripcion Exit Code:%s",obtenerDescripcionExitCode(proceso->exitCode));

	if(!verificarHiloFinalizado(proceso->pid)){
		informarConsola(buscarSocketHiloPrograma(proceso->pid),obtenerDescripcionExitCode(proceso->exitCode),strlen(obtenerDescripcionExitCode(proceso->exitCode)));
		finalizarHiloPrograma(proceso->pid);
	}

	pthread_mutex_lock(&mutexMemoria);
	liberarRecursosEnMemoria(proceso);
	pthread_mutex_unlock(&mutexMemoria);

	liberarMemoriaDinamica(proceso->pid);

	destruirTablaArchivosYActualizarTablaGlobal(proceso->pid);

	cambiarEstadoATerminado(proceso);
	disminuirGradoMultiprogramacion();
	sem_post(&sem_admitirNuevoProceso);
}


void verificarPidColaSemaforos(int pid){
	int i,j;
	t_semaforoAsociado *semaforo;
	//printf("Verificando cola semaforos\n");
	//printf("Pid a checkear:%d\n",pid);
	for (i = 0; i < list_size(colaSemaforos); ++i) {
		semaforo = list_get(colaSemaforos,i);
		for (j = 0; j < list_size(semaforo->pids); ++j) {
			//printf("Pid a comparar:%d\n",*(int*)list_get(semaforo->pids, j));
			if(pid == *(int*)list_get(semaforo->pids, j)) {
				//printf("\nPID ELIMINADO %d\tSEMAFORO %s\n", pid, semaforo->semaforo->id);
				list_remove(semaforo->pids,j);
			}
		}

	}
}




void finalizarHiloPrograma(int pid){
	t_consola* consola = malloc(sizeof(t_consola));

	char comandoFinalizarHiloPrograma='F';

	_Bool verificaPid(t_consola* consolathread){
			return (consolathread->pid == pid);
	}

		pthread_mutex_lock(&mutexListaConsolas);
		consola = list_remove_by_condition(listaConsolas,(void*)verificaPid);
		pthread_mutex_unlock(&mutexListaConsolas);

		informarConsola(consola->socketHiloPrograma,&comandoFinalizarHiloPrograma,sizeof(char));
		eliminarSocket(consola->socketHiloPrograma);
		free(consola);
}

void liberarRecursosEnMemoria(t_pcb* proceso){
	log_info(logKernel,"Liberando proceso en memoria--->PID: %d",proceso->pid);
	char comandoFinalizarProceso= 'F';

	send(socketMemoria,&comandoFinalizarProceso,sizeof(char),0);
	send(socketMemoria,&proceso->pid,sizeof(int),0);

}

void liberarMemoriaDinamica(int pid){
	log_info(logKernel,"Liberando Memoria Dinamica ");
	int bloquesSinLiberar;
	int sizeSinLiberar;
	_Bool verificaPid(t_contable* proceso){
		return proceso->pid == pid;
	}

	pthread_mutex_lock(&mutexListaContable);
	t_contable* proceso = list_remove_by_condition(listaContable,(void*)verificaPid);
	bloquesSinLiberar = proceso->cantAlocar - proceso->cantLiberar;
	sizeSinLiberar = proceso->sizeAlocar - proceso->sizeLiberar;
	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);

	if(bloquesSinLiberar > 0) log_warning(logKernelPantalla,"El proceso no libero %d bloques de Heap, acumulando %d bytes--->PID:%d",bloquesSinLiberar,sizeSinLiberar,pid);

		destruirTodasLasPaginasHeapDeProceso(pid);

}



void destruirTablaArchivosYActualizarTablaGlobal(int pid){
	int i=0;
	int cont=0;
	_Bool verificaPid(t_indiceTablaProceso* indice){
		return indice->pid==pid;
	}

	t_entradaTablaProceso*entrada;
	t_indiceTablaProceso* indice = list_find(listaTablasProcesos,(void*)verificaPid);


	while(cont<indice->tablaProceso->elements_count){
		//imprimirTablaArchivosProceso(pid);
		//interfaceTablaGlobalArchivos();
		entrada=list_get(indice->tablaProceso,i);
		if(!disminuirOpenYVerificarExistenciaEntradaGlobal(entrada->indiceGlobal)) i++;
		cont++;
	}

	list_remove_and_destroy_by_condition(listaTablasProcesos,(void*)verificaPid,free);
	//list_destroy_and_destroy_elements(indice->tablaProceso,free);
	//free(indice);
}



void cambiarEstadoATerminado(t_pcb* procesoTerminar){
	log_info(logKernelPantalla,"Almacenando en Terminados--->PID:%d\n",procesoTerminar->pid);
	_Bool verificaPid(t_pcb* pcb){
			return (pcb->pid == procesoTerminar->pid);
		}

	pthread_mutex_lock(&mutexColaTerminados);
	list_add(colaTerminados,procesoTerminar);
	pthread_mutex_unlock(&mutexColaTerminados);
}

void disminuirGradoMultiprogramacion(){
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	gradoMultiProgramacion--;
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
}



/*------------------------LARGO PLAZO-----------------------------------------*/

void interfazPausarPlanificacion(){

	if(!flagPlanificacion)log_info(logKernelPantalla,"Planificacion ya pausada\n");
	else{
		flagPlanificacion = 0;
		sem_wait(&sem_planificacion);
		log_info(logKernelPantalla,"Se pauso la planificacion");
	}
}

void interfazReanudarPlanificacion(){


	if(flagPlanificacion) log_info(logKernelPantalla,"Planificacion no se encuentra pausada");
	else{
		flagPlanificacion = 1;

		sem_post(&sem_planificacion);
		sem_post(&sem_planificacion);
		log_info(logKernelPantalla,"Se reanudo la planificacion");
	}
}

void verificarPausaPlanificacion(){
	if(!flagPlanificacion){
		log_info(logKernel,"\nLa planificacion se encuentra pausada\n");
		sem_wait(&sem_planificacion);
	}
}

/*------------------------CORTO PLAZO-----------------------------------------*/

void agregarA(t_list* lista, void* elemento, pthread_mutex_t mutex){

	pthread_mutex_lock(&mutex);
	list_add(lista, elemento);
	pthread_mutex_unlock(&mutex);
}

void planificarCortoPlazo(){
	t_pcb* pcbListo;
	t_cpu* cpuEnEjecucion = malloc(sizeof(t_cpu));

	_Bool verificarCPU(t_cpu* cpu){
		return (cpu->estado == OCIOSA);
	}
	char comandoEnviarPcb = 'P';

	signal(SIGUSR1,signalHandler);


	//pthread_create(&threadFinQuantumAReady, NULL, (void*)finQuantumAReady, NULL);


	while(!flagTerminarPlanificadorCortoPlazo){
		verificarPausaPlanificacion();

		sem_wait(&sem_CPU);
		sem_wait(&sem_procesoListo);


		pthread_mutex_lock(&mutexColaListos);
		pcbListo = list_remove(colaListos,0);
		pthread_mutex_unlock(&mutexColaListos);

		if(list_any_satisfy(listaCPU, (void*) verificarCPU)){

			pthread_mutex_lock(&mutexListaCPU);
			cpuEnEjecucion = list_remove_by_condition(listaCPU,(void*) verificarCPU);
			cpuEnEjecucion->estado = EJECUTANDO;
			cpuEnEjecucion->pid = pcbListo->pid;
			list_add(listaCPU, cpuEnEjecucion);
			pthread_mutex_unlock(&mutexListaCPU);


			send(cpuEnEjecucion->socket,&comandoEnviarPcb,sizeof(char),0);
			printf("hola\n");
			enviarConfiguracionesQuantum(cpuEnEjecucion->socket);

			serializarPcbYEnviar(pcbListo, cpuEnEjecucion->socket);


			pthread_mutex_lock(&mutexColaEjecucion);
			list_add(colaEjecucion, pcbListo);
			pthread_mutex_unlock(&mutexColaEjecucion);

			log_info(logKernelPantalla,"Pcb encolado en Ejecucion--->PID:%d\n",pcbListo->pid);

		}else{

			pthread_mutex_lock(&mutexColaListos);
			list_add(colaListos,pcbListo);
			pthread_mutex_unlock(&mutexColaListos);

			sem_post(&sem_procesoListo);

		}

	}
}

void enviarConfiguracionesQuantum(int socketCPU){
	int quantum = 0; //FIFO--->0 ; RR != 0
	if(!strcmp(config_algoritmo, "RR")) quantum = config_quantum;
		send(socketCPU,&quantum,sizeof(int),0);
		send(socketCPU,&config_quantumSleep,sizeof(int),0);
}

/*
void finQuantumAReady(){

	int indice;
	t_pcb* pcbBuffer;


	while(1){


		sem_wait(&sem_listaFinQuantum);
		verificarPausaPlanificacion();

			for (indice = 0; indice <= list_size(listaFinQuantum); ++indice) {

				pthread_mutex_lock(&mutexListaFinQuantum);
				pcbBuffer = list_remove(listaFinQuantum,indice);
				pthread_mutex_unlock(&mutexListaFinQuantum);


				pthread_mutex_lock(&mutexColaListos);
				list_add(colaListos,pcbBuffer);
				pthread_mutex_unlock(&mutexColaListos);

				log_info(logKernelPantalla, "\nPCB encolado en Listos---> PID: %d\n", pcbBuffer->pid);

				sem_post(&sem_procesoListo);
			}
	}
}

void agregarAFinQuantum(t_pcb* pcb){

	_Bool verificarPidPcb(t_pcb* unPcb){
		return (unPcb->pid == pcb->pid);
	}

	pthread_mutex_lock(&mutexListaFinQuantum);
	list_add(listaFinQuantum, pcb);
	pthread_mutex_unlock(&mutexListaFinQuantum);

	sem_post(&sem_listaFinQuantum);

}
*/
/*------------------------CORTO PLAZO-----------------------------------------*/








/*------------------------MEDIANO PLAZO-----------------------------------------*/
/*
pthread_t threadId;

 void planificarMedianoPlazo(){

	 signal(SIGUSR1,signalHandler);

	 pthread_create(&threadId,NULL, (void*)pcbBloqueadoAReady, NULL);

	 pcbEjecucionABloqueado();

 }
 */
/*
void pcbBloqueadoAReady(){

	int i;

	t_semYPCB *semYPCB;
	t_pcb *pcbADesbloquear;
	t_semaforo *semaforoBuffer = malloc(sizeof(t_semaforo));

	_Bool verificaSemId(t_semYPCB *semYPCBBuffer){
		return (!strcmp(semYPCBBuffer->idSemaforo, semaforoBuffer->id));
	}

	_Bool verificaIdPCB(t_pcb *pcbBuffer){
			 return (pcbBuffer->pid == semYPCB->pcb->pid);
	}

	while(1){
		sem_wait(&sem_semAumentados);

		verificarPausaPlanificacion();

		for(i = 0; i < list_size(listaSemaforosGlobales); ++i) {

			pthread_mutex_lock(&mutexListaSemaforos);
			semaforoBuffer = list_get(listaSemaforosGlobales,i);
			pthread_mutex_unlock(&mutexListaSemaforos);

			if(semaforoBuffer->valor > 0){

				if(list_any_satisfy(listaSemYPCB, (void*)verificaSemId)){

					pthread_mutex_lock(&mutexListaSemYPCB);
					semYPCB = list_remove_by_condition(listaSemYPCB, (void*)verificaSemId);
					pthread_mutex_unlock(&mutexListaSemYPCB);

					if(list_any_satisfy(colaBloqueados, (void*)verificaIdPCB)){

						log_info(logKernelPantalla,"Cambiando proceso desde Bloqueados a Listos--->PID:%d",semYPCB->pcb->pid);

						pthread_mutex_lock(&mutexColaBloqueados);
						pcbADesbloquear = list_remove_by_condition(colaBloqueados,(void*)verificaIdPCB);
						pthread_mutex_unlock(&mutexColaBloqueados);

						pthread_mutex_lock(&mutexColaListos);
						list_add(colaListos,pcbADesbloquear);
						pthread_mutex_unlock(&mutexColaListos);

						sem_post(&sem_procesoListo);
					}
				}
			}
		}
	}
}


void pcbEjecucionABloqueado(){

	int indice;
	t_semYPCB *semYPCB = malloc(sizeof(t_semYPCB));


	_Bool verificaPCB(t_pcb* pcb){
		return (pcb->pid == semYPCB->pcb->pid);
	}

	while(1){

		sem_wait(&sem_ListaSemYPCB);
		verificarPausaPlanificacion();

		for (indice = 0; indice < list_size(listaSemYPCB); ++indice) {

			pthread_mutex_lock(&mutexListaSemYPCB);
			semYPCB = list_get(listaSemYPCB, indice);
			pthread_mutex_unlock(&mutexListaSemYPCB);

			log_info(logKernelPantalla,"Cambiando proceso desde Ejecucion a Bloqueados--->PID:%d",semYPCB->pcb->pid);

			pthread_mutex_lock(&mutexColaEjecucion);
			list_remove_by_condition(colaEjecucion, (void*)verificaPCB);
			pthread_mutex_unlock(&mutexColaEjecucion);

			pthread_mutex_lock(&mutexColaBloqueados);
			list_add(colaBloqueados, semYPCB->pcb);
			pthread_mutex_unlock(&mutexColaBloqueados);
		}
	}
	//free(semYPCB);
}
*/

/*------------------------MEDIANO PLAZO-----------------------------------------*/


/*-------------------PLANIFICACION GENERAL------------------------------------*/



int verificarGradoDeMultiprogramacion(){
	pthread_mutex_lock(&mutex_config_gradoMultiProgramacion);
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	if(gradoMultiProgramacion >= config_gradoMultiProgramacion) {
		pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
		pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);
		return -1;
	}
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
	pthread_mutex_unlock(&mutex_config_gradoMultiProgramacion);
	return 0;
}

void cargarConsola(int pid, int socketHiloPrograma) {

	t_consola *infoConsola = malloc(sizeof(t_consola));
	infoConsola->socketHiloPrograma=socketHiloPrograma;
	infoConsola->pid=pid;

	pthread_mutex_lock(&mutexListaConsolas);
	list_add(listaConsolas,infoConsola);
	pthread_mutex_unlock(&mutexListaConsolas);
}

void encolarProcesoListo(t_pcb *procesoListo){
	pthread_mutex_lock(&mutexColaListos);
	list_add(colaListos,procesoListo);
	pthread_mutex_unlock(&mutexColaListos);
	log_info(logKernelPantalla, "Pcb encolado en Listos--->PID: %d\n", procesoListo->pid);
}


void gestionarFinProcesoCPU(int socketCPU){
	log_info(logKernelPantalla,"CPU: %d finalizo exitosamente un proceso",socketCPU);
	_Bool verificaCpu(t_cpu* cpu){
				return (cpu->socket == socketCPU);
	}


	t_pcb* proceso = recibirYDeserializarPcb(socketCPU);

	proceso->exitCode = exitCodeArray[EXIT_OK]->value;
	completarRafagas(proceso->pid,proceso->cantidadInstrucciones);
	removerDeColaEjecucion(proceso->pid);
	pthread_mutex_lock(&mutexListaEspera);
	list_add(listaEspera,proceso);
	pthread_mutex_unlock(&mutexListaEspera);
	sem_post(&sem_administrarFinProceso);
	cambiarEstadoCpu(socketCPU,OCIOSA);
	sem_post(&sem_CPU);


}


void aumentarGradoMultiprogramacion(){
	pthread_mutex_lock(&mutex_gradoMultiProgramacion);
	gradoMultiProgramacion++;
	pthread_mutex_unlock(&mutex_gradoMultiProgramacion);
}


int obtenerPaginaSiguiente(int pid){
	int pagina;
	_Bool verificaPid(t_pcb* pcb){
			return pcb->pid == pid;
		}

		_Bool verificaPidContable(t_contable* proceso){
				return proceso->pid == pid;
			}

	pthread_mutex_lock(&mutexListaContable);
	t_contable* proceso = list_remove_by_condition(listaContable,(void*)verificaPidContable);

	pthread_mutex_lock(&mutexColaEjecucion);
	t_pcb* pcb = list_remove_by_condition(colaEjecucion,(void*)verificaPid);
	pagina = pcb->cantidadPaginasCodigo + config_stackSize + proceso->cantPaginasHeap ;
	list_add(colaEjecucion,pcb);
	pthread_mutex_unlock(&mutexColaEjecucion);

	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);


	return pagina;
}



void signalHandler(int sigCode){
	int exitCode;
	pthread_exit(&exitCode);
}

#endif /* PLANIFICACION_H_ */
