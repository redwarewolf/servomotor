 /*
 * contabilidad.h
 *
 *  Created on: 8/6/2017
 *      Author: utnso
 */

#ifndef CONTABILIDAD_H_
#define CONTABILIDAD_H_

#include  "pcb.h"
#include "sincronizacion.h"

typedef struct{
	int pid;
	int cantRafagas;
	int cantPaginasHeap;
	int cantAlocar;
	int sizeAlocar;
	int cantLiberar;
	int sizeLiberar;
	int cantSysCalls;
}t_contable;


t_list* listaContable;
void crearInformacionContable(int pid);
void actualizarAlocar(int pid,int size);
void actualizarLiberar(int pid,int size);
void actualizarRafagas(int pid, int rafagas);
void actualizarSysCalls(int pid);
void completarRafagas(int pid, int rafagas);
t_contable* buscarInformacionContable(int pid);

void crearInformacionContable(int pid){
	t_contable* proceso = malloc(sizeof(t_contable));
	proceso->pid=pid;
	proceso->cantRafagas=0;
	proceso->cantPaginasHeap=0;
	proceso->cantAlocar = 0;
	proceso->cantLiberar = 0;
	proceso->sizeAlocar = 0;
	proceso->sizeLiberar = 0;
	proceso->cantSysCalls=0;

	pthread_mutex_lock(&mutexListaContable);
	list_add(listaContable,proceso);
	pthread_mutex_unlock(&mutexListaContable);

}

void actualizarSysCalls(int pid){

	t_contable* contabilidad = malloc(sizeof(t_contable));

	pthread_mutex_lock(&mutexListaContable);
	contabilidad = buscarInformacionContable(pid);
	contabilidad->cantSysCalls += 1;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

void actualizarPaginasHeap(int pid){
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = buscarInformacionContable(pid);
	contabilidad->cantPaginasHeap+=1;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

void actualizarRafagas(int pid, int rafagas){
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = buscarInformacionContable(pid);
	if(rafagas > contabilidad->cantRafagas) contabilidad->cantRafagas = rafagas;
	else contabilidad->cantRafagas += rafagas;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

void completarRafagas(int pid, int rafagas){
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = buscarInformacionContable(pid);
	if(rafagas > contabilidad->cantRafagas) contabilidad->cantRafagas = rafagas;
	else contabilidad->cantRafagas = rafagas;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

void actualizarAlocar(int pid,int size){
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = buscarInformacionContable(pid);
	contabilidad->cantAlocar +=1;
	contabilidad->sizeAlocar += size;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

void actualizarLiberar(int pid,int size){
	pthread_mutex_lock(&mutexListaContable);
	t_contable* contabilidad = buscarInformacionContable(pid);
	contabilidad->cantLiberar +=1;
	contabilidad->sizeLiberar += size;
	list_add(listaContable,contabilidad);
	pthread_mutex_unlock(&mutexListaContable);
}

t_contable* buscarInformacionContable(int pid){
	_Bool verificaPid(t_contable* contabilidad){
			return contabilidad->pid == pid;
	}
	return list_remove_by_condition(listaContable,(void*)verificaPid);

}

#endif /* CONTABILIDAD_H_ */
