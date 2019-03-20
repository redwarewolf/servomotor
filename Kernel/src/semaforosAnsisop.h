/*
 * semaforosAnsisop.h
 *
 *  Created on: 13/6/2017
 *      Author: utnso
 */

#ifndef SEMAFOROSANSISOP_H_
#define SEMAFOROSANSISOP_H_

#include "comandosCPU.h"
#include "listasAdministrativas.h"
#include "excepciones.h"


typedef struct{
	int valor;
	char *id;
}t_semaforo;

typedef struct{
	t_semaforo* semaforo;
	t_list* pids;
}t_semaforoAsociado;

typedef struct{
	char *idSemaforo;
	t_pcb *pcb;

}t_semYPCB;

/*
 * Generales
 */
void obtenerSemaforosANSISOPDeLasConfigs();
void recibirNombreSemaforo(int socketCpu,char ** semaforo);
void buscarSemaforo(char* semaforoId, t_semaforoAsociado **semaforoAsociado);
int tamanioArray(char** array);
/*
 * Wait_Semaforos_ANSISOP
 * */
void waitSemaforoAnsisop(int socketCPU);
int disminuirYConsultarSemaforo(char* semaforoId);
void expropiarProcesoBloqueado(int socket,char* semaforoId);
void encolarProcesoBloqueadoASemaforo(int pid,char* semaforo);

/*
 * Signal_Semaforos_ANSISOP
 */
void signalSemaforoAnsisop(int socketAceptado);
void aumentarYConsultarSemaforo(char* semaforoId);


t_list* colaSemaforos;

/* TODO: No se usan
t_list* listaSemaforosGlobales;
t_list* listaSemYPCB;
*/

void obtenerSemaforosANSISOPDeLasConfigs(){
	int i;
	int tamanio = tamanioArray(semId);

	/*TODO: Ver los mallocs. Hay char* adnetro de los structs. OJOO*/
	for(i = 0; i < tamanio; i++){

		t_semaforo* semaforo = malloc(sizeof(t_semaforo));
		semaforo->id=semId[i];
		semaforo->valor = strtol(semInit[i], NULL, 10);

		t_semaforoAsociado* indiceSemaforo = malloc(sizeof(t_semaforoAsociado));
		indiceSemaforo->semaforo = semaforo;
		indiceSemaforo->pids = list_create();

		list_add(colaSemaforos,indiceSemaforo);

		//list_add(listaSemaforosGlobales,semaforo);
		}
}

void recibirNombreSemaforo(int socketCpu,char ** semaforo){
	int tamanio;
	recv(socketCpu,&tamanio,sizeof(int),0);

	*semaforo = malloc(tamanio + sizeof(char));
	recv(socketCpu,*semaforo,tamanio,0);
	strcpy(*semaforo + tamanio,"\0");
}

void waitSemaforoAnsisop(int socketCPU){
	int expropiar,pid;
	char* semaforoId;
	//t_semYPCB *semYPCB = malloc(sizeof(t_semYPCB));

	recv(socketCPU,&pid,sizeof(int),0);

	recibirNombreSemaforo(socketCPU,&semaforoId);
	log_info(logKernelPantalla, "Wait---> Semaforo:%s", semaforoId);

	expropiar = disminuirYConsultarSemaforo(semaforoId);
	//disminuirSemaforo(semaforo);

	send(socketCPU,&expropiar,sizeof(int),0);

	if(expropiar < 0) {
		//semYPCB->pcb = recibirYDeserializarPcb(socketCPU);
		log_info(logKernelPantalla,"Bloqueando proceso--->PID:%d--->Semaforo:%s",pid,semaforoId);
		expropiarProcesoBloqueado(socketCPU,semaforoId);

	/*	cpuEjecucionAFQPB(socketCPU);

		semYPCB->idSemaforo = semaforo;

		pthread_mutex_lock(&mutexListaSemYPCB);
		list_add(listaSemYPCB, semYPCB);
		pthread_mutex_unlock(&mutexListaSemYPCB);

		sem_post(&sem_ListaSemYPCB);
		*/
	}
	free(semaforoId);
	actualizarSysCalls(pid);
	return;
}

void expropiarProcesoBloqueado(int socket,char* semaforoId){
	int rafagasEjecutadas;
	t_pcb* proceso = recibirYDeserializarPcb(socket);

	recv(socket, &rafagasEjecutadas, sizeof(int), 0);

	actualizarRafagas(proceso->pid,rafagasEjecutadas);
	removerDeColaEjecucion(proceso->pid);
	cambiarEstadoCpu(socket,OCIOSA);

	log_info(logKernelPantalla,"Proceso encolado en Bloqueados--->PID:%d\n",proceso->pid);
	pthread_mutex_lock(&mutexColaBloqueados);
	list_add(colaBloqueados,proceso);
	pthread_mutex_unlock(&mutexColaBloqueados);

	sem_post(&sem_CPU);

	encolarProcesoBloqueadoASemaforo(proceso->pid,semaforoId);
}

void signalSemaforoAnsisop(int socketCpu){
	char* semaforo;
	int pid;
	recv(socketCpu,&pid,sizeof(int),0);

	recibirNombreSemaforo(socketCpu,&semaforo);
	log_info(logKernelPantalla, "Signal--->:Semaforo:%s", semaforo);
	aumentarYConsultarSemaforo(semaforo);


	actualizarSysCalls(pid);
	free(semaforo);
	//	sem_post(&sem_semAumentados);

	return;
}
int disminuirYConsultarSemaforo(char* semaforoId){
	int expropiar = 1;
	//t_semaforo* semaforoAsociado = malloc(sizeof(t_semaforo));

	t_semaforoAsociado* indiceSemaforo = malloc(sizeof(t_semaforoAsociado));
	_Bool verificaId(t_semaforo* semaforo){
				return (!strcmp(semaforo->id,semaforoId));
	}

	//pthread_mutex_lock(&mutexListaSemaforos);

	//buscarSemaforo(semaforoId,&semaforoAsociado);
	buscarSemaforo(semaforoId,&indiceSemaforo);

	//list_add(listaSemaforosGlobales,semaforoAsociado);


	//log_info(logKernel,"Semaforo id: %s", semaforoAsociado->id);
	//log_info(logKernel,"\nSemaforo valor: %d", semaforoAsociado->valor);

	indiceSemaforo->semaforo->valor -= 1;

	if(indiceSemaforo->semaforo->valor < 0) expropiar = -1;



	//if(semaforoAsociado->valor < 1) expropiar = -1;

	list_add(colaSemaforos,indiceSemaforo);
	//pthread_mutex_unlock(&mutexListaSemaforos);

	return expropiar;
}


void desbloquearProceso(int pid){
	_Bool verificaPid(t_pcb* procesoBloqueado){
		return procesoBloqueado->pid == pid;
	}
	pthread_mutex_lock(&mutexColaBloqueados);
	t_pcb* proceso = list_remove_by_condition(colaBloqueados,(void*)verificaPid);
	pthread_mutex_unlock(&mutexColaBloqueados);

	pthread_mutex_lock(&mutexColaListos);
	list_add(colaListos,proceso);
	pthread_mutex_unlock(&mutexColaListos);

	sem_post(&sem_procesoListo);

}

void encolarProcesoBloqueadoASemaforo(int pid,char* semaforoId){

	_Bool verificaId(t_semaforoAsociado* semaforo){
					return (!strcmp(semaforo->semaforo->id,semaforoId));
			}

	t_semaforoAsociado* indiceSemaforo = list_remove_by_condition(colaSemaforos,(void*)verificaId);
	int* pidBloqueado=malloc(sizeof(int));
	*pidBloqueado = pid;
	list_add(indiceSemaforo->pids,pidBloqueado);
	list_add(colaSemaforos,indiceSemaforo);

}


void aumentarYConsultarSemaforo(char* semaforoId){

	_Bool verificaId(t_semaforoAsociado* indiceSemaforo){
			return (!strcmp(indiceSemaforo->semaforo->id,semaforoId));
	}

	t_semaforoAsociado *semaforoAsociado;
	//pthread_mutex_lock(&mutexListaSemaforos);

	buscarSemaforo(semaforoId,&semaforoAsociado);

	semaforoAsociado->semaforo->valor +=1;

	if(semaforoAsociado->pids->elements_count > 0){

			int pid = *(int*)list_remove(semaforoAsociado->pids,0);
			log_info(logKernelPantalla,"Cambiando proceso de Bloqueados a Listos--->PID:%d--->Semaforo:%s\n",pid,semaforoId);
			desbloquearProceso(pid);
	}

	list_add(colaSemaforos,semaforoAsociado);


//	log_info(logKernel,"Semaforo id: %s", semaforoAsociado->id);
	//log_info(logKernel,"\nSemaforo valor: %d", semaforoAsociado->valor);


	//semaforoAsociado->valor += 1;
	//list_add(listaSemaforosGlobales,semaforoAsociado);



	//log_info(logKernel,"Semaforo id: %s", semaforoAsociado->id);
	//log_info(logKernel,"\nSemaforo valor aumentado: %d", semaforoAsociado->valor);


	//pthread_mutex_unlock(&mutexListaSemaforos);



}

void buscarSemaforo(char* semaforoId, t_semaforoAsociado **semaforoAsociado){

	_Bool verificaId(t_semaforoAsociado* semaforo){
				return (!strcmp(semaforo->semaforo->id,semaforoId));
		}

	if(list_any_satisfy(colaSemaforos, (void*) verificaId)){
		*semaforoAsociado = list_remove_by_condition(colaSemaforos,(void*)verificaId);
	}
}


int tamanioArray(char** array){
	int i = 0;
	while(array[i]) i++;
	return i;
}
#endif /* SEMAFOROSANSISOP_H_ */
