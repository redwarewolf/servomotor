/*

 * heap.h
 *
 *  Created on: 19/6/2017
 *      Author: utnso
 */

#ifndef HEAP_H_
#define HEAP_H_
#include "configuraciones.h"
#include <commons/collections/list.h>
#include "conexionMemoria.h"
#include "contabilidad.h"
#include "sincronizacion.h"
#include "excepciones.h"
#include "planificacion.h"
#include "logs.h"

typedef struct
{
	int pagina;
	int pid;
	int sizeDisponible;
}t_adminBloqueHeap;


typedef struct
{
	int bitUso;
    int size;
}__attribute__((packed)) t_bloqueMetadata;

typedef struct{
	int pagina;
	int offset;
}t_punteroCPU;

typedef struct{
	int pid;
	int size;
	int socket;
}t_alocar;

t_list* listaAdmHeap;

void handlerExpropiado(int signal);


void reservarEspacioHeap(t_alocar* data);
t_punteroCPU *verificarEspacioLibreHeap(int size, int pid);
int reservarPaginaHeap(int pid,int pagina);
void compactarPaginaHeap(int pagina, int pid);
void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido);
void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido);
void reservarBloqueHeap(int pid,int size,t_punteroCPU *puntero);
void destruirPaginaHeap(int pidProc, int pagina);
void destruirTodasLasPaginasHeapDeProceso(int pidProc);
int paginaHeapBloqueSuficiente(int posicionPaginaHeap,int pagina,int pid,int size);
void liberarBloqueHeap(int pid, int pagina, int offset);
void imprimirListaAdministrativaHeap();
void imprimirMetadatasPaginaProceso(int pagina, int pid);

void reservarEspacioHeap(t_alocar* data){
	log_info(logKernel,"Reservando %d bytes de memoria dinamica:--->PID:%d",data->size,data->pid);
	int resultadoEjecucion;

	signal(SIGUSR1,handlerExpropiado);

	t_punteroCPU* puntero;

	pthread_mutex_lock(&mutexMemoria);
	puntero = verificarEspacioLibreHeap(data->size, data->pid);
	pthread_mutex_unlock(&mutexMemoria);

	if(puntero->pagina  == -1){

		puntero->pagina = obtenerPaginaSiguiente(data->pid);

		pthread_mutex_lock(&mutexMemoria);
		resultadoEjecucion = reservarPaginaHeap(data->pid,puntero->pagina);
		pthread_mutex_unlock(&mutexMemoria);
		puntero->offset = 0;

		if(resultadoEjecucion < 0){
				excepcionCantidadDePaginas(data->socket,data->pid);
				free(data);
				free(puntero);
				return ;
		}

		actualizarPaginasHeap(data->pid);
		}

	pthread_mutex_lock(&mutexMemoria);
	reservarBloqueHeap(data->pid, data->size,puntero);
	pthread_mutex_unlock(&mutexMemoria);

	puntero->offset += sizeof(t_bloqueMetadata);

	//imprimirMetadatasPaginaProceso(puntero->pagina,data->pid);

	//printf("\nPagina que se le da para ese espacio de memoria:%d\n",puntero->pagina);
	send(data->socket,&resultadoEjecucion,sizeof(int),0);
	send(data->socket,&puntero->pagina,sizeof(int),0);
	send(data->socket,&puntero->offset,sizeof(int),0);
	free(data);
	free(puntero);
}

void handlerExpropiado(int signal){

	if(signal==SIGUSR1){
	log_error(logKernel,"Un servicio de Alocar se ha abortado porque el proceso debio ser expropiado");
	int valor;
	pthread_exit(&valor);
	}

}


t_punteroCPU *verificarEspacioLibreHeap(int size, int pid){
	//log_info(logKernel,"Verificando espacio libre en Heap--->PID:%d",pid);
	int i = 0;
	t_punteroCPU* puntero = malloc(sizeof(t_punteroCPU));
	t_adminBloqueHeap* aux;
	puntero->pagina = -1;

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = (t_adminBloqueHeap*) list_get(listaAdmHeap,i);
		pthread_mutex_unlock(&mutexListaAdminHeap);

		/*printf("i=%d\n",i);
		printf("sizeDisponible=%d\n",aux->sizeDisponible);
		printf("pid=%d\n",aux->pid);*/
		if(aux->sizeDisponible >= size + sizeof(t_bloqueMetadata) && aux->pid == pid)
		{
			//imprimirMetadatasPaginaProceso(aux->pagina, aux->pid);
			compactarPaginaHeap(aux->pagina,aux->pid);
			//imprimirMetadatasPaginaProceso(aux->pagina, aux->pid);
			puntero-> offset = paginaHeapBloqueSuficiente(i,aux->pagina,aux->pid,size);
			//printf("Puntero:%d\n",puntero->offset);
			if(puntero-> offset >= 0){
				puntero->pagina = aux->pagina;
				break;
			}
		}
		i++;
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);
	return puntero;
}


int reservarPaginaHeap(int pid,int pagina){ //Reservo una página de heap nueva para el proceso
	log_info(logKernel,"Reservando nueva pagina de heap--->PID:%d",pid);
	int resultadoEjecucion;
	t_bloqueMetadata aux ;

	void* buffer=malloc(sizeof(t_bloqueMetadata));
	aux.bitUso = -1;
	aux.size = config_paginaSize - sizeof(t_bloqueMetadata);
	memcpy(buffer,&aux,sizeof(t_bloqueMetadata));

	resultadoEjecucion=reservarPaginaEnMemoria(pid);
	if(resultadoEjecucion < 0) return -1;

	resultadoEjecucion=escribirEnMemoria(pid,pagina,0,sizeof(t_bloqueMetadata),buffer);  //Para indicar que está sin usar y que tiene tantos bits libres para utilizarse


	t_adminBloqueHeap* bloqueAdmin=malloc(sizeof(t_adminBloqueHeap));
	bloqueAdmin->pagina = pagina;
	bloqueAdmin->pid = pid;
	bloqueAdmin->sizeDisponible = aux.size;

	/*printf("Pagina Reservada:%d\n",bloqueAdmin->pagina);
	printf("PID Proceso Reservado:%d\n",bloqueAdmin->pid);
	printf("Size Disponible Pagina Reservada:%d\n",bloqueAdmin->sizeDisponible);*/

	list_add(listaAdmHeap, bloqueAdmin);
	free(buffer);
	log_info(logKernel,"Pagina de heap %d reservada--->PID:%d",pagina,pid);
	return resultadoEjecucion;
}


void compactarPaginaHeap(int pagina, int pid){
	log_info(logKernel,"Compactando pagina de heap %d--->PID :%d\n",pagina,pid);
	int offset = 0;
	t_bloqueMetadata actual;
	t_bloqueMetadata siguiente;
	void* buffer= malloc(sizeof(t_bloqueMetadata));
	int sizeMetadatasLiberados = 0;

	actual.size = 0;

	while(offset < config_paginaSize && offset + sizeof(t_bloqueMetadata) + actual.size < config_paginaSize - sizeof(t_bloqueMetadata)){

		buffer = leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata)); //Leo el metadata Actual
		memcpy(&actual,buffer,sizeof(t_bloqueMetadata));

		buffer = leerDeMemoria(pid,pagina,offset + sizeof(t_bloqueMetadata) + actual.size,sizeof(t_bloqueMetadata)); //Leo la posición del metadata que le sigue al actual

		memcpy(&siguiente,buffer,sizeof(t_bloqueMetadata));

		/*printf("Actual bitUso=%d\n",actual.bitUso);
		printf("Actual size=%d\n",actual.size);
		printf("Siguiente bitUso=%d\n",siguiente.bitUso);
		printf("Siguiente size=%d\n",siguiente.size);*/

		if(actual.bitUso == -1 && siguiente.bitUso == -1){

			sizeMetadatasLiberados += sizeof(t_bloqueMetadata);
			actual.size = actual.size + sizeof(t_bloqueMetadata) + siguiente.size;
			memcpy(buffer,&actual,sizeof(t_bloqueMetadata));
			escribirEnMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata),buffer); //Actualizo el metadata en el que me encuentro parado en la memoria

		}
		else{
			offset += sizeof(t_bloqueMetadata) + actual.size;
			actual.size = siguiente.size;
		}
	}
	free(buffer);

	int i = 0;
	t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));

	pthread_mutex_lock(&mutexListaAdminHeap);
		while(i < list_size(listaAdmHeap))
		{
			aux = list_get(listaAdmHeap,i);
			if(aux->pagina == pagina && aux->pid == pid){

				aux->sizeDisponible = aux->sizeDisponible + sizeMetadatasLiberados ;
				list_replace(listaAdmHeap,i,aux);
				break;
			}
			i++;
		}
	pthread_mutex_unlock(&mutexListaAdminHeap);

	log_info(logKernel,"Pagina de heap %d compactada--->PID :%d",pagina,pid);
}

void escribirContenidoPaginaHeap(int pagina, int pid, int offset, int size, void *contenido){
	log_info(logKernel,"Escribiendo contenido en Pagina:%d del PID:%d\n",pagina,pid);
	escribirEnMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size,contenido);
}

void leerContenidoPaginaHeap(int pagina, int pid, int offset, int size, void **contenido){
	log_info(logKernel,"Leyendo contenido en Pagina:%d del PID:%d\n",pagina,pid);
	*contenido = leerDeMemoria(pid,pagina,offset+sizeof(t_bloqueMetadata),size);
}

void reservarBloqueHeap(int pid,int size,t_punteroCPU* puntero){
	log_info(logKernelPantalla,"Reservando bloque de %d bytes en pagina heap:%d --->PID:%d\n",size,puntero->pagina,pid);
	t_bloqueMetadata auxBloque;
	t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));
	int i = 0;
	int sizeLibreViejo;
	void *buffer=malloc(sizeof(t_bloqueMetadata));

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == puntero->pagina && aux->pid == pid){
			aux->sizeDisponible = aux->sizeDisponible - size - sizeof(t_bloqueMetadata);
			list_replace(listaAdmHeap,i,aux);
			break;

		}
		i++;
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);

	buffer = leerDeMemoria(pid,puntero->pagina,puntero->offset,sizeof(t_bloqueMetadata));
	memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));

	//printf("AuxBloque.size:%d\n",auxBloque.size);

	sizeLibreViejo = auxBloque.size;
	auxBloque.bitUso = 1;
	auxBloque.size = size;
	memcpy(buffer,&auxBloque,sizeof(t_bloqueMetadata));

	escribirEnMemoria(pid,puntero->pagina,puntero->offset,sizeof(t_bloqueMetadata),buffer); //Escribo y reservo el metadata que se quiere reservar

	auxBloque.bitUso = -1;
	auxBloque.size = sizeLibreViejo - size - sizeof(t_bloqueMetadata);
	/*printf("SizeLibreViejo:%d\n",sizeLibreViejo);
	printf("Size:%d\n",size);
	printf("Size struct Metadata:%d\n",sizeof(t_bloqueMetadata));*/

	memcpy(buffer,&auxBloque,sizeof(t_bloqueMetadata));

	escribirEnMemoria(pid,puntero->pagina,puntero->offset+sizeof(t_bloqueMetadata)+size,sizeof(t_bloqueMetadata),buffer); //Anuncio cuanto espacio libre queda en el heap en el siguiente metadata

	free(buffer);
	log_info(logKernel,"Bloque de pagina heap %d reservado --->PID:%d",puntero->pagina,pid);
}

void destruirPaginaHeap(int pidProc, int pagina){ //Si quiero destruir una página específica de la lista
	t_adminBloqueHeap* aux;
	int i = 0;

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pagina == pagina && aux->pid == pidProc)
		{
			list_remove(listaAdmHeap,i);
			break;
		}
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);

}

void destruirTodasLasPaginasHeapDeProceso(int pidProc){ //Elimino todas las estructuras administrativas de heap asociadas a un PID
	t_adminBloqueHeap* aux;
	int i = 0;

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
	{
		aux = list_get(listaAdmHeap,i);
		if(aux->pid == pidProc)
		{
			list_remove(listaAdmHeap,i);
		}
		i++;
	}
	pthread_mutex_unlock(&mutexListaAdminHeap);

}

int paginaHeapBloqueSuficiente(int posicionPaginaHeap,int pagina,int pid ,int size){
	//printf("Pagina Heap Bloque Suficiente\n");DATA G__
	int i = 0;


	t_bloqueMetadata auxBloque;
	void *buffer= malloc(sizeof(t_bloqueMetadata));

	while(i < config_paginaSize){

		buffer = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));

		if(auxBloque.size >= size + sizeof(t_bloqueMetadata) && auxBloque.bitUso == -1){
			//printf("Pagina Heap Bloque Suficiente\n");
			free(buffer);
			return i;
		}

		else{
			i = i + sizeof(t_bloqueMetadata) + auxBloque.size;
		}
	}
	//printf("Saliendo Pagina Heap Bloque NO Suficiente\n");
	free(buffer);
	return -1;
}

void imprimirMetadatasPaginaProceso(int pagina, int pid){

	printf("Metadatas de página %d del pid %d\n",pagina,pid);

	t_bloqueMetadata auxBloque;
	void *buffer= malloc(sizeof(t_bloqueMetadata));
	int i = 0;
	while(i < config_paginaSize){

		buffer = leerDeMemoria(pid,pagina,i,sizeof(t_bloqueMetadata));
		memcpy(&auxBloque,buffer,sizeof(t_bloqueMetadata));
		printf("Metadata\nBitUso:%d\nSize:%d\n",auxBloque.bitUso,auxBloque.size);

		i = i + sizeof(t_bloqueMetadata) + auxBloque.size;
	}
	free(buffer);
}

void liberarBloqueHeap(int pid, int pagina, int offset){
	log_info(logKernel,"Liberando bloque de pagina:%d y offset:%d de la memoria dinamica--->PID:%d",pagina,offset,pid);
	offset -= sizeof(t_bloqueMetadata);


	int i = 0;
	t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));
	t_bloqueMetadata bloque;

	void *buffer=malloc(sizeof(t_bloqueMetadata));

	pthread_mutex_lock(&mutexMemoria);
	buffer = leerDeMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata));
	pthread_mutex_unlock(&mutexMemoria);

	memcpy(&bloque,buffer,sizeof(t_bloqueMetadata));

	/*
	printf("Leo:\n");
	printf("Pagina:%d\n",pagina);
	printf("Offset:%d\n",i);
	printf("BitUso:%d\n",bloque.bitUso);
	printf("Size:%d\n",bloque.size);
*/
	bloque.bitUso = -1;

	//printf("\n\nEstoy liberando:%d\n\n",bloque.size);

	actualizarLiberar(pid,bloque.size);
	memcpy(buffer,&bloque,sizeof(t_bloqueMetadata));

	pthread_mutex_lock(&mutexMemoria);
	escribirEnMemoria(pid,pagina,offset,sizeof(t_bloqueMetadata),buffer);
	pthread_mutex_unlock(&mutexMemoria);

	pthread_mutex_lock(&mutexListaAdminHeap);
	while(i < list_size(listaAdmHeap))
		{
			aux = list_get(listaAdmHeap,i);
			if(aux->pagina == pagina && aux->pid == pid){
				aux->sizeDisponible = aux->sizeDisponible + bloque.size;
				list_replace(listaAdmHeap,i,aux);
				break;
			}
			i++;
		}
	pthread_mutex_unlock(&mutexListaAdminHeap);
	//imprimirListaAdministrativaHeap();

	/*imprimirMetadatasPaginaProceso(pagina,pid);

	compactarPaginaHeap(pagina,pid);
	imprimirMetadatasPaginaProceso(pagina,pid);
	imprimirListaAdministrativaHeap();*/
}

void imprimirListaAdministrativaHeap(){
		log_info(logKernel,"Imprimir Lista Administrativas Heap\n");
		printf("Paginas de heap\n");
		t_adminBloqueHeap* aux = malloc(sizeof(t_adminBloqueHeap));
		int i = 0;

		pthread_mutex_lock(&mutexListaAdminHeap);
		while(i < list_size(listaAdmHeap))
		{
			aux = list_get(listaAdmHeap,i);
			log_info(logKernelPantalla,"Pid:%d",aux->pid);
			log_info(logKernelPantalla,"Pagina:%d",aux->pagina);
			log_info(logKernelPantalla,"Size Disponible:%d\n",aux->sizeDisponible);
			i++;
		}
		pthread_mutex_unlock(&mutexListaAdminHeap);
}

#endif /* HEAP_H_ */
