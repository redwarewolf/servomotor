/*
 * sincronizacion.h

 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef SINCRONIZACION_H_
#define SINCRONIZACION_H_

#include <pthread.h>
#include <semaphore.h>

void inicializarSemaforos();

pthread_mutex_t mutexColaNuevos;
pthread_mutex_t mutexColaListos;
pthread_mutex_t mutexColaTerminados;
pthread_mutex_t mutexColaEjecucion;
pthread_mutex_t mutexColaBloqueados;

pthread_mutex_t mutexListaEspera;

pthread_mutex_t mutexNuevoProceso;
pthread_mutex_t mutexListaFinQuantum;
pthread_mutex_t mutexListaConsolas;
pthread_mutex_t mutexListaCPU;


pthread_mutex_t mutex_config_gradoMultiProgramacion;
pthread_mutex_t mutex_gradoMultiProgramacion;


pthread_mutex_t mutex_masterSet;

pthread_mutex_t mutexListaContable;
pthread_mutex_t mutexListaCodigo;
pthread_mutex_t mutexListaSemaforos;
pthread_mutex_t mutexListaSemYPCB;
pthread_mutex_t mutexListaSemAumentados;
pthread_mutex_t mutexListaAdminHeap;

pthread_mutex_t mutexMemoria;



sem_t sem_admitirNuevoProceso;
sem_t sem_administrarFinProceso;
sem_t sem_planificacion;
sem_t sem_procesoListo;
sem_t sem_CPU;

sem_t sem_envioPCB;
sem_t sem_eliminacionCPU;

sem_t sem_ordenSelect;
sem_t sem_ordenUI;
sem_t sem_listaFinQuantum;
sem_t sem_ListaSemYPCB;
sem_t sem_semAumentados;

void inicializarSemaforos(){
		pthread_mutex_init(&mutexColaNuevos,NULL);
		pthread_mutex_init(&mutexColaListos, NULL);
		pthread_mutex_init(&mutexColaTerminados, NULL);
		pthread_mutex_init(&mutexColaEjecucion,NULL);
		pthread_mutex_init(&mutexColaBloqueados,NULL);

		pthread_mutex_init(&mutexListaConsolas,NULL);
		pthread_mutex_init(&mutexListaCPU,NULL);
		pthread_mutex_init(&mutexListaEspera,NULL);

		pthread_mutex_init(&mutex_config_gradoMultiProgramacion,NULL);
		pthread_mutex_init(&mutex_gradoMultiProgramacion,NULL);

		pthread_mutex_init(&mutex_masterSet,NULL);

		pthread_mutex_init(&mutexListaContable,NULL);
		pthread_mutex_init(&mutexNuevoProceso,NULL);
		pthread_mutex_init(&mutexListaCodigo,NULL);
		pthread_mutex_init(&mutexListaSemaforos,NULL);
		pthread_mutex_init(&mutexListaSemYPCB,NULL);
		pthread_mutex_init(&mutexListaSemAumentados,NULL);
		pthread_mutex_init(&mutexListaAdminHeap,NULL);

		pthread_mutex_init(&mutexMemoria,NULL);

		sem_init(&sem_admitirNuevoProceso, 0, 0);
		sem_init(&sem_administrarFinProceso,0,0);
		sem_init(&sem_procesoListo,0,0);
		sem_init(&sem_CPU,0,0);

		sem_init(&sem_envioPCB,0,0);
		sem_init(&sem_eliminacionCPU,0,0);

		sem_init(&sem_planificacion,0,1);
		sem_init(&sem_ordenSelect,0,0);
		sem_init(&sem_ordenUI,0,1);
		sem_init(&sem_listaFinQuantum,0,0);
		sem_init(&sem_ListaSemYPCB,0,0);
		sem_init(&sem_semAumentados,0,0);
}

#endif /* SINCRONIZACION_H_ */
