/*
 * conexionConsola.h
 *
 *  Created on: 23/6/2017
 *      Author: utnso
 */

#ifndef CONEXIONCONSOLA_H_
#define CONEXIONCONSOLA_H_

typedef struct CONSOLA{
	int pid;
	int socketHiloPrograma;
}t_consola;

typedef struct {
	char* codigo;
	int size;
	int pid;
	int socketHiloConsola;
}t_codigoPrograma;

t_list* listaCodigosProgramas;
t_list* listaConsolas;

int buscarSocketHiloPrograma(int pid);
void informarConsola(int socketHiloPrograma,char* mensaje, int size);

int buscarSocketHiloPrograma(int pid){
	int socketConsola;
	_Bool verificaPid(t_consola* consola){
		return consola->pid==pid;
	}

	pthread_mutex_lock(&mutexListaConsolas);
	t_consola* consola=list_remove_by_condition(listaConsolas,(void*)verificaPid);
	socketConsola=consola->socketHiloPrograma;
	list_add(listaConsolas,consola);
	pthread_mutex_unlock(&mutexListaConsolas);

	return socketConsola;
}

void informarConsola(int socketHiloPrograma,char* mensaje, int size){
	send(socketHiloPrograma,&size,sizeof(int),0);
	send(socketHiloPrograma,mensaje,size,0);
}

#endif /* CONEXIONCONSOLA_H_ */
