/*
 * conexionMemoria.h
 *
 *  Created on: 2/6/2017
 *      Author: utnso
 */

#ifndef CONEXIONMEMORIA_H_
#define CONEXIONMEMORIA_H_
#include "sincronizacion.h"
#include "planificacion.h"

int socketMemoria;
int solicitarContenidoAMemoria(char** mensajeRecibido);
int pedirMemoria(t_pcb* procesoListo);
int almacenarCodigoEnMemoria(t_pcb* procesoListoAutorizado, char* programa, int programSize);
int calcularTamanioParticion(int *programSizeRestante);
int reservarPaginaEnMemoria(int pid);
int escribirEnMemoria(int pid,int pagina,int offset,int size,char* contenido);
void* leerDeMemoria(int pid,int pagina,int offset,int size);
void handshakeMemoria();


int escribirEnMemoria(int pid,int pagina,int offset, int size,char*contenido){
	char comandoEscribir= 'C';
	int resultadoEjecucion = 0;
	void* mensajeAMemoria = malloc(sizeof(char) + sizeof(int)* 4 + size);

	memcpy(mensajeAMemoria,&comandoEscribir,sizeof(char));
	memcpy(mensajeAMemoria + sizeof(char),&pid,sizeof(int));
	memcpy(mensajeAMemoria + sizeof(int)+sizeof(char),&pagina,sizeof(int));
	memcpy(mensajeAMemoria + sizeof(int)*2 + sizeof(char),&offset,sizeof(int));
	memcpy(mensajeAMemoria + sizeof(int)*3 + sizeof(char),&size,sizeof(int));

	memcpy(mensajeAMemoria + sizeof(int)*4 + sizeof(char),contenido,size);
	send(socketMemoria,mensajeAMemoria,sizeof(char) + sizeof(int)* 4 + size,0);

	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

void* leerDeMemoria(int pid,int pagina,int offset,int size){
	char comandoSolicitud= 'S';
	void* buffer = malloc(size + sizeof(char));
	int resultadoEjecucion=0;

		send(socketMemoria,&comandoSolicitud,sizeof(char),0);
		send(socketMemoria,&pid,sizeof(int),0);
		send(socketMemoria,&pagina,sizeof(int),0);
		send(socketMemoria,&offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		recv(socketMemoria,buffer,size,0);
		strcpy(buffer+size,"\0");
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		return buffer;

}

int reservarPaginaEnMemoria(int pid){
	log_info(logKernel,"Solicitando nueva pagina a memoria--->PID:%d",pid);
	char comandoReservarPagina = 'G';
	int cantidadPaginas = 1;
	int resultadoEjecucion=0;

	send(socketMemoria,&comandoReservarPagina,sizeof(char),0);
	send(socketMemoria,&pid,sizeof(int),0);
	send(socketMemoria,&cantidadPaginas,sizeof(int),0);
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

void handshakeMemoria(){
	log_info(logKernel,"Obteniendo tamano de pagina");
	char comandoTamanioPagina = 'P';
	send(socketMemoria,&comandoTamanioPagina,sizeof(char),0);
	recv(socketMemoria,&config_paginaSize,sizeof(int),0);
}

int pedirMemoria(t_pcb* procesoListo){
		log_info(logKernel, "Solicitando Memoria--->PID: %d", procesoListo->pid);
		void* mensajeAMemoria = malloc(sizeof(int)*2 + sizeof(char));
		int paginasTotalesRequeridas = procesoListo->cantidadPaginasCodigo + config_stackSize;
		int resultadoEjecucion=1;
		char comandoInicializacion = 'A';

		memcpy(mensajeAMemoria,&comandoInicializacion,sizeof(char));
		memcpy(mensajeAMemoria + sizeof(char), &procesoListo->pid,sizeof(int));
		memcpy(mensajeAMemoria + sizeof(char) + sizeof(int) , &paginasTotalesRequeridas, sizeof(int));
		send(socketMemoria,mensajeAMemoria,sizeof(int)*2 + sizeof(char),0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		free(mensajeAMemoria);
		return resultadoEjecucion;
}

int almacenarCodigoEnMemoria(t_pcb* procesoListoAutorizado,char* programa, int programSize){
	log_info(logKernel, "Almacenando programa en memoria--->PID: %d", procesoListoAutorizado->pid);
		char* particionCodigo = malloc(config_paginaSize + sizeof(char));
		int particionSize;
		int programSizeRestante = programSize;
		int resultadoEjecucion=0;
		int offset=0;
		int nroPagina;


		for(nroPagina=0; nroPagina<procesoListoAutorizado->cantidadPaginasCodigo && resultadoEjecucion>=0;nroPagina++){
				particionSize=calcularTamanioParticion(&programSizeRestante);
				strncpy(particionCodigo,programa,particionSize);
				programa += particionSize;
				resultadoEjecucion = escribirEnMemoria(procesoListoAutorizado->pid,nroPagina,offset,particionSize,particionCodigo);
		}
		free(particionCodigo);
		return resultadoEjecucion;

}

int calcularTamanioParticion(int *programSizeRestante){
		int mod=*programSizeRestante % config_paginaSize;
				 if(mod == *programSizeRestante){
					return *programSizeRestante;
			 }
				else{
					*programSizeRestante -= config_paginaSize;
					return config_paginaSize;
			 }
}

int solicitarContenidoAMemoria(char ** mensajeRecibido){
	char comandoSolicitud= 'S';
	int pid;
	int paginaSolicitada;
	int offset;
	int size;
	int resultadoEjecucion;
	printf("Se inicializa una peticion de consulta\n");
	send(socketMemoria,&comandoSolicitud,sizeof(char),0);
	printf("Ingrese el pid del proceso solicitado\n");
	scanf("%d",&pid);
	printf("Ingrese la pagina solicitada\n");
	scanf("%d",&paginaSolicitada);
	printf("Ingrese el offset \n");
	scanf("%d",&offset);
	printf("Ingrese el tamano del proceso\n");
	scanf("%d",&size);

	send(socketMemoria,&pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*mensajeRecibido = malloc((size + 1 )*sizeof(char));
	recv(socketMemoria,*mensajeRecibido,size,0);
	strcpy(*mensajeRecibido+size,"\0");
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}




#endif /* CONEXIONMEMORIA_H_ */
