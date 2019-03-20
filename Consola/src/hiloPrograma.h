/*
 * hiloPrograma.c
 *
 *  Created on: 22/5/2017
 *      Author: utnso
 */

#include <pthread.h>
#include "conexiones.h"
#include <time.h>
#include <semaphore.h>


typedef struct {
	int pid;
	int cantImpresiones;
	int socketHiloKernel;
	struct tm tiempoInicio;
	pthread_t idHilo;
}t_hiloPrograma;

pthread_mutex_t mutexListaHilos;
pthread_mutex_t mutex_crearHilo;
pthread_mutex_t mutexRecibirDatos;

sem_t sem_crearHilo;
t_list* listaHilosProgramas;

time_t now;
struct tm beg;

void crearHiloPrograma();
void* iniciarPrograma(int* socketHiloKernel);
void recibirDatosDelKernel(int socketHiloKernel);
int enviarLecturaArchivo(char *ruta,int socketHiloKernel);
void cargarHiloPrograma(int pid, int socket);
void gestionarCierrePrograma(int pidFinalizar);
void actualizarCantidadImpresiones(int pid);

void informarEstadisticas(t_hiloPrograma* programaAFinalizar);

void crearHiloPrograma(){
	log_info(logConsola,"Creando hilo programa\n");

	t_hiloPrograma* nuevoPrograma = malloc(sizeof(t_hiloPrograma));
	nuevoPrograma->socketHiloKernel=  crear_socket_cliente(ipKernel,puertoKernel);

	nuevoPrograma->tiempoInicio= *localtime(&(time_t){time(NULL)});
	nuevoPrograma->cantImpresiones = 0;
	int err = pthread_create(&nuevoPrograma->idHilo , NULL ,(void*)iniciarPrograma ,&nuevoPrograma->socketHiloKernel);
	if (err != 0) log_error(logConsola,"\nError al crear el hilo :[%s]", strerror(err));


	pthread_mutex_lock(&mutexListaHilos);
	list_add(listaHilosProgramas,nuevoPrograma);

	pthread_mutex_unlock(&mutexListaHilos);

}

void* iniciarPrograma(int* socketHiloKernel){
	char *ruta = malloc(200 * sizeof(char));
	int comandoCerrarSocket='Z';

	_Bool verificaSocket(t_hiloPrograma* programa){
		return programa->socketHiloKernel == *socketHiloKernel;
	}

	printf("Indicar la ruta del archivo AnSISOP que se quiere ejecutar\n");
	scanf("%s", ruta);

	if ((enviarLecturaArchivo(ruta,*socketHiloKernel)) < 0) {
		log_error(logConsolaPantalla,"El archivo indicado es inexistente");
		send(*socketHiloKernel,&comandoCerrarSocket,sizeof(char),0);
		list_remove_and_destroy_by_condition(listaHilosProgramas,(void*)verificaSocket,free);
		close(*socketHiloKernel);
		pthread_mutex_unlock(&mutex_crearHilo);
		return -1;
	}
	free(ruta);

	recibirDatosDelKernel(*socketHiloKernel);
	return 0;
}
void finalizarPrograma(){
	char comandoInterruptHandler = 'X';
	char comandoFinalizarPrograma= 'F';
	int procesoATerminar;
	t_hiloPrograma* proceso;
	log_info(logConsolaPantalla,"Ingresar el PID del programa a finalizar\n");
	scanf("%d", &procesoATerminar);

		bool verificarPid(t_hiloPrograma* proceso){
			return (proceso->pid == procesoATerminar);
		}

		pthread_mutex_lock(&mutexListaHilos);
		if (list_any_satisfy(listaHilosProgramas,(void*)verificarPid)){

			proceso = list_remove_by_condition(listaHilosProgramas,(void*)verificarPid);
			log_info(logConsolaPantalla,"Avisando al kernel que un programa finalizo");

				send(proceso->socketHiloKernel,&comandoInterruptHandler,sizeof(char),0);
				send(proceso->socketHiloKernel,&comandoFinalizarPrograma,sizeof(char),0);
				send(proceso->socketHiloKernel,&procesoATerminar, sizeof(int), 0);
				list_add(listaHilosProgramas,proceso);
			}else	log_error(logConsolaPantalla,"\nPID incorrecto\n");

		pthread_mutex_unlock(&mutexListaHilos);

		pthread_mutex_unlock(&mutex_crearHilo);
}


void gestionarCierrePrograma(int pidFinalizar){
	bool verificarPid(t_hiloPrograma* proceso){
		return (proceso->pid == pidFinalizar);
	}
	pthread_mutex_lock(&mutexListaHilos);

	t_hiloPrograma* programaAFinalizar = list_remove_by_condition(listaHilosProgramas,(void*)verificarPid);

	pthread_mutex_unlock(&mutexListaHilos);

	close(programaAFinalizar->socketHiloKernel);

	informarEstadisticas(programaAFinalizar);

	free(programaAFinalizar);

}



void cargarHiloPrograma(int pid, int socket){
	log_info(logConsola,"Encolando programa en lista de ejecucion--->PID:%d\n",pid);

	_Bool verificarSocket(t_hiloPrograma* hiloPrograma){
					return (hiloPrograma->socketHiloKernel ==socket);
					}
	pthread_mutex_lock(&mutexListaHilos);
	t_hiloPrograma* hiloPrograma= list_remove_by_condition(listaHilosProgramas,(void*)verificarSocket);
	hiloPrograma->pid = pid;
	list_add(listaHilosProgramas,hiloPrograma);
	pthread_mutex_unlock(&mutexListaHilos);
}

int enviarLecturaArchivo(char *ruta,int socketHiloKernel) {
	FILE *f;
	void *mensaje;
	char *bufferArchivo;
	int tamanioArchivo;
	char comandoIniciarPrograma='A';

	if ((f = fopen(ruta, "r+")) == NULL)return -1;

	fseek(f, 0, SEEK_END);
	tamanioArchivo = ftell(f);
	rewind(f);
	log_info(logConsola,"Leyendo el contenido del script del archivo\n");

	bufferArchivo = malloc(tamanioArchivo);

	fread(bufferArchivo, sizeof(char), tamanioArchivo, f);
	mensaje = malloc(sizeof(int) + sizeof(char) + tamanioArchivo);

	log_info(logConsola,"Enviando al kernel la peticion de un nuevo programa con el contenido del script ANSISOP\n");

	memcpy(mensaje, &comandoIniciarPrograma,sizeof(char));
	memcpy(mensaje + sizeof(char), &tamanioArchivo, sizeof(int));
	memcpy(mensaje + sizeof(char) + sizeof(int), bufferArchivo, tamanioArchivo);

	send(socketHiloKernel, mensaje, tamanioArchivo + sizeof(int) + sizeof(char)  , 0);

	free(bufferArchivo);
	free(mensaje);

	return 0;
}


void informarEstadisticas(t_hiloPrograma* programaAFinalizar){
	log_info(logConsola,"Informando estadisticas programa: %d",programaAFinalizar->pid);
	struct tm tiempoFinalizacion = *localtime(&(time_t){time(NULL)});
	double seconds = difftime(mktime(&(tiempoFinalizacion)), mktime(&(programaAFinalizar->tiempoInicio)));


	log_info(logConsola,"\tHora de inicializacion:   %s\n", asctime(&programaAFinalizar->tiempoInicio));
	log_info(logConsola,"\tHora de finalizacion:   %s\n\tTiempo de ejecucion:   %.f Segundos\n\n\tCantidad de impresiones:   %d\n",asctime(&tiempoFinalizacion),seconds,programaAFinalizar->cantImpresiones);


	printf("\033[22;36mEstadisticas programa--->PID:%d\033[0m\n",programaAFinalizar->pid);
	printf("\033[22;36m\tHora de inicializacion:%s\033[0m\n", asctime(&programaAFinalizar->tiempoInicio));
	printf("\033[22;36m\tHora de finalizacion: %s\n\tTiempo de ejecucion: %.f Segundos\n\n\tCantidad de impresiones:%d\033[0m\n",asctime(&tiempoFinalizacion),seconds,programaAFinalizar->cantImpresiones);
}
