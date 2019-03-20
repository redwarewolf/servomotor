/*
 ============================================================================
 Name        : Memoria.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

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
#include <commons/string.h>
#include <commons/config.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "conexiones.h"
#include <commons/log.h>
#include <semaphore.h>


//--------LOG----------------//
void inicializarLog(char *rutaDeLog);



t_log *logConsola;
t_log *logConsolaPantalla;
//----------------------------//
//-------------------------------//

sem_t sem;
//-----------------------------//


char* puertoMemoria;//4000
char* ipMemoria;
int marcos;
int marco_size;
int entradas_cache;
int cache_x_proc;
int retardo_memoria;
int contadorConexiones=0;
pthread_t thread_id, thread_id2;
t_config* configuracion_memoria;
int sizeStructsAdmMemoria;

typedef struct
{
	int frame;
	int pid;
	int num_pag;
}struct_adm_memoria;

//char* bitMap;
void* bloque_Memoria;
void* bloque_Cache;
void* bloqueBitUsoCache;
int contadorBitDeUso=1;

int socket_servidor;

//the thread function
void* connection_handler(void * socket);
void* connection_Listener();

//--------------------Funciones Conexiones----------------------------//
int recibirConexion(int socket_servidor);
void gestionarCierreDeConexion(int socket);
//----------------------Funciones Conexiones----------------------------//

char nuevaOrdenDeAccion(int puertoCliente);

void imprimirConfiguraciones();
void leerConfiguracion(char* ruta);
void inicializarMemoriaAdm();

int main_inicializarPrograma(int sock);
int main_solicitarBytesPagina(int sock);
int main_almacenarBytesPagina(int sock);
int main_asignarPaginasAProceso(int sock);
int main_finalizarPrograma(int sock);
int main_liberarPaginaProceso(int sock);

//-----------------------FUNCIONES MEMORIA--------------------------//
int inicializarPrograma(int pid, int cantPaginas);
int solicitarBytesPagina(int pid,int pagina, int offset, int size,char** buffer);
int almacenarBytesPagina(int pid,int pagina, int offset,int size, void* buffer);
int asignarPaginasAProceso(int pid, int cantPaginas, int frame);
int finalizarPrograma(int pid);
int liberarPaginaProceso(int pid, int pagina);
//------------------------------------------------------------------//

//void liberarBitMap(int pos, int size);
//void ocuparBitMap(int pos, int size);
int verificarEspacioLibre();
void escribirEstructuraAdmAMemoria(int pid, int frame, int cantPaginas, int cantPaginasAnteriores);

void borrarProgramDeStructAdms(int pid);
int buscarFrameDePaginaDeProceso(int pid, int pagina);

//void imprimirBitMap();
void imprimirEstructurasAdministrativas();
int buscarFrameVacio();

void interfazHandler();
void modificarRetardo();
void dumpDeMemoria();
void flush();
void size();

void dumpCache();
void contenidoDeMemoria();
void datosAlmacenadosDeProceso();
void datosAlmacenadosEnMemoria();
void vaciarCache();
void tamanioProceso();
void tamanioMemoria();
int cantPaginasDeProceso(int pid);

void inicializarCache();
int buscarEntradaDeProcesoEnCache(int pid, int pagina);
int cantidadEntradasDeProcesoEnCache(int pid);
void leerContenidoEnCache(int entrada,char** buffer, int size, int offset);
void iniciarEntradaEnCache(int pid, int pagina);
void borrarEntradasDeProcesoEnCache(int pid);

int funcionHash(int pid, int pagina);

int main(void)
{
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Memoria/config_Memoria");
	inicializarLog("/home/utnso/Log/logMemoria.txt");
	imprimirConfiguraciones();

	sem_init(&sem,0,0);

	bloque_Memoria= malloc(marco_size*marcos);
	bloque_Cache = malloc((sizeof(int)*2+marco_size)*entradas_cache);
	bloqueBitUsoCache = malloc(sizeof(int)*entradas_cache);

	inicializarMemoriaAdm();
	inicializarCache();

	socket_servidor = crear_socket_servidor(ipMemoria,puertoMemoria);

	pthread_create( &thread_id , NULL , connection_Listener,NULL);

	interfazHandler();
	pthread_join(thread_id,NULL);


	return EXIT_SUCCESS;
}

void leerConfiguracion(char* ruta)
{
	configuracion_memoria = config_create(ruta);
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO");
	ipMemoria = config_get_string_value(configuracion_memoria,"IP_MEMORIA");
	marcos = config_get_int_value(configuracion_memoria,"MARCOS");
	marco_size = config_get_int_value(configuracion_memoria,"MARCO_SIZE");
	entradas_cache = config_get_int_value(configuracion_memoria,"ENTRADAS_CACHE");
	cache_x_proc = config_get_int_value(configuracion_memoria,"CACHE_X_PROC");
	retardo_memoria = config_get_int_value(configuracion_memoria,"RETARDO_MEMORIA");
}

void inicializarMemoriaAdm()
{
	sizeStructsAdmMemoria = ((sizeof(struct_adm_memoria)*marcos)+marco_size-1)/marco_size;

	log_info(logConsolaPantalla,"Las estructuras administrativas de la memoria ocupan %i frames\n",sizeStructsAdmMemoria);

	printf("---------------------------------------------------\n");

	//ocuparBitMap(0,sizeMemoriaAdm);
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	aux.pid=-1;
	aux.num_pag=-1;
	aux.frame = i;
	while(i < marcos)
	{
		if(i<sizeStructsAdmMemoria)
		{
			aux.pid = -9;
			aux.num_pag=i;
		}
		else
		{
			aux.pid = -1;
			aux.num_pag=-1;
		}
		memcpy(bloque_Memoria + i*desplazamiento, &aux, sizeof(struct_adm_memoria));
		i++;
		aux.frame = i;
	}
}

int inicializarPrograma(int pid, int cantPaginas)
{
	log_info(logConsolaPantalla,"Inicializar Programa %d\n",pid);
	int posicionFrame = verificarEspacioLibre();
	if(posicionFrame >= 0)
	{
		//ocuparBitMap(posicionFrame,cantPaginas);
		asignarPaginasAProceso(pid,cantPaginas,posicionFrame);
	}

	return posicionFrame;
}

int solicitarBytesPagina(int pid,int pagina, int offset, int size, char** buffer)
{
	log_info(logConsolaPantalla,"Solicitar Bytes Pagina %d del proceso %d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);
	log_info(logConsolaPantalla,"El frame es : %d\n",frame);
	if(frame != -1){
		memcpy(*buffer,bloque_Memoria + frame*marco_size+offset,size);
		strcpy(*buffer + size , "\0");
		log_info(logConsolaPantalla,"%s\0\n",*buffer);
	}
	return frame;
}

int almacenarBytesPagina(int pid,int pagina, int offset,int size, void* buffer)
{
	log_info(logConsolaPantalla,"Almacenar Bytes A Pagina:%d del proceso:%d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);

	//printf("\nESTOY GUARDANDO :   %s\n",buffer);
	log_info(logConsolaPantalla,"Frame:%d\n",frame);
	if(frame >= 0)
	{
		memcpy(bloque_Memoria + frame*marco_size+offset,buffer,size);
	}
	else
	{
		log_warning(logConsolaPantalla,"No se encontró el PID/Pagina del programa\n");
	}
	return frame;
}

int asignarPaginasAProceso(int pid, int cantPaginas, int posicionFrame)
{
	escribirEstructuraAdmAMemoria(pid,posicionFrame,cantPaginas,cantPaginasDeProceso(pid));
	return EXIT_SUCCESS;
}

int finalizarPrograma(int pid)
{
	log_info(logConsolaPantalla,"\nFinalizar Programa:%d\n",pid);
	borrarProgramDeStructAdms(pid);
	borrarEntradasDeProcesoEnCache(pid);


	return EXIT_SUCCESS;
}

int recibirConexion(int socket_servidor){
	struct sockaddr_storage their_addr;
	 socklen_t addr_size;


	int estado = listen(socket_servidor, 5);

	if(estado == -1){
		log_info(logConsolaPantalla,"\nError al poner el servidor en listen\n");
		close(socket_servidor);
		return 1;
	}


	if(estado == 0){
		log_info(logConsolaPantalla,"\nSe puso el socket en listen\n");
		printf("---------------------------------------------------\n");
	}

	addr_size = sizeof(their_addr);

	int socket_aceptado;
    socket_aceptado = accept(socket_servidor, (struct sockaddr *)&their_addr, &addr_size);

	contadorConexiones ++;
	log_info(logConsolaPantalla,"\n----------Nueva Conexion aceptada numero: %d ---------\n",contadorConexiones);
	log_info(logConsolaPantalla,"----------Handler asignado a (%d) ---------\n",contadorConexiones);

	if (socket_aceptado == -1){
		close(socket_servidor);
		log_error(logConsolaPantalla,"\nError al aceptar conexion\n");
		return 1;
	}
	sem_post(&sem);
	return socket_aceptado;
}

char nuevaOrdenDeAccion(int socketCliente)
{
	char* buffer=malloc(sizeof(char));
	char bufferRecibido;
	printf("\n--Esperando una orden del cliente-- \n");
	recv(socketCliente,buffer,sizeof(char),0);
	bufferRecibido = *buffer;
	log_info(logConsolaPantalla,"El cliente ha enviado la orden: %c\n",bufferRecibido);
	free(buffer);
	return bufferRecibido;
}

int main_inicializarPrograma(int sock)
{
	int pid;
	int cantPaginas;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&cantPaginas,sizeof(int),0);

	log_info(logConsolaPantalla,"PID:%d\n",pid);
	log_info(logConsolaPantalla,"CantPaginas:%d\n",cantPaginas);

	int espacioLibre = verificarEspacioLibre();

	//printf("Bitmap:%s\n",bitMap);

	if(espacioLibre >= cantPaginas)
	{
		int posicionFrame;
		int i = 0;
		while(i<cantPaginas)
		{
			posicionFrame = buscarFrameVacio();
			//ocuparBitMap(posicionFrame,1);
			asignarPaginasAProceso(pid,1,posicionFrame);
			i++;
		}
		sleep(retardo_memoria);
		return 0;
	}
	else
	{
		sleep(retardo_memoria);
		log_warning(logConsolaPantalla,"No hay espacio suficiente en la memoria\n");
		return -1;
	}
}
int main_solicitarBytesPagina(int sock)
{
	int pid;
	int pagina;
	int offset;
	int size;
	char*bufferAEnviar;
	int resultadoEjecucion = 0;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&pagina,sizeof(int),0);
	recv(sock,&offset,sizeof(int),0);
	recv(sock,&size,sizeof(int),0);

	log_info(logConsolaPantalla,"PID:%d\tPagina:%d\tOffset:%d\tSize:%d\n",pid,pagina,offset,size);


	bufferAEnviar=malloc(size+sizeof(char));

	int posicionEnCache = buscarEntradaDeProcesoEnCache(pid,pagina);

	if(posicionEnCache != -1)
	{
		log_info(logConsolaPantalla,"Se encontró la pagina %d del proceso %d en la entrada %d de la cache\n",pagina,pid,posicionEnCache);
		leerContenidoEnCache(posicionEnCache,&bufferAEnviar, size, offset);
	}
	else
	{
		if(cantidadEntradasDeProcesoEnCache(pid) < cache_x_proc){
			iniciarEntradaEnCache(pid,pagina);
		}
		log_info(logConsolaPantalla,"No se encontro la pagina %d del proceso %d en la cache\n",pagina,pid);
		resultadoEjecucion = solicitarBytesPagina(pid,pagina,offset,size,&bufferAEnviar);
		sleep(retardo_memoria);
		//enviar_string(sock,bufferAEnviar);
	}
	send(sock,bufferAEnviar,size,0);
	free(bufferAEnviar);

	return resultadoEjecucion;
}
int main_almacenarBytesPagina(int sock)
{
	int pid;
	int pagina;
	int offset;
	int size;
	void *bytes;

	int resultadoEjecucion = 0;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&pagina,sizeof(int),0);
	recv(sock,&offset,sizeof(int),0);
	recv(sock,&size,sizeof(int),0);
	bytes=malloc(size);
	recv(sock,bytes,size,0);

	log_info(logConsolaPantalla,"PID:%d\tPagina:%d\tOffset:%d\tSize:%d\n",pid,pagina,offset,size);

	resultadoEjecucion = almacenarBytesPagina(pid,pagina,offset,size,bytes);
	if(buscarEntradaDeProcesoEnCache(pid,pagina) >= 0){
		actualizarEntradaEnCache(pid,pagina);
	}
	free(bytes);
	sleep(retardo_memoria);
	return resultadoEjecucion;
}
int main_asignarPaginasAProceso(int sock)
{
	int pid;
	int cantPaginas;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&cantPaginas,sizeof(int),0);

	log_info(logConsolaPantalla,"PID:%d\tCantidad de Paginas:%d\n",pid,cantPaginas);
	//printf("Bitmap:%s\n",bitMap);

	int espacioLibre = verificarEspacioLibre();

	if(espacioLibre >= cantPaginas)
	{
		int posicionFrame;
		int i = 0;
		while(i<cantPaginas)
		{
			posicionFrame = buscarFrameVacio();
			//ocuparBitMap(posicionFrame,1);
			asignarPaginasAProceso(pid,1,posicionFrame);
			i++;
		}
		sleep(retardo_memoria);
		return 0;
	}
	else
	{
		sleep(retardo_memoria);
		log_warning(logConsolaPantalla,"No hay espacio suficiente en la memoria\n");
		return -1;
	}
}
int main_finalizarPrograma(int sock)
{
	int pid;

	recv(sock,&pid,sizeof(int),0);

	finalizarPrograma(pid);
	sleep(retardo_memoria);
	return 0;
}
int main_liberarPaginaProceso(int sock){
	int pid;
	int pagina;
	int resultadoDeEjecucion;

	recv(sock,&pid,sizeof(int),0);
	recv(sock,&pagina,sizeof(int),0);

	log_info(logConsolaPantalla,"PID:%d\tPagina:%d\n",pid,pagina);

	resultadoDeEjecucion = liberarPaginaProceso(pid,pagina);

	sleep(retardo_memoria);
	return resultadoDeEjecucion;
}

int liberarPaginaProceso(int pid, int pagina){
	log_info(logConsolaPantalla,"Liberar Pagina:%d del proceso:%d\n",pagina,pid);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);

	int desplazamiento = sizeof(struct_adm_memoria);
	struct_adm_memoria aux;
	if(frame >= 0){
		aux.num_pag = -1;
		aux.pid = -1;
		memcpy(bloque_Memoria + frame*desplazamiento,&aux,sizeof(struct_adm_memoria)); //Marco que en ese frame no tengo un programa asignado
	}


	return frame;

}

/*void liberarBitMap(int pos, int size)
{
	int i;
	for (i=0; i< size; i++)
	{
		bitMap[pos+i] = (char) '0';
	}
}

void ocuparBitMap(int pos, int size)
{
	int i;
	for (i=0; i< size; i++)
	{
		bitMap[pos+i] = (char) '1';
	}
}
*/

int verificarEspacioLibre()
{
	struct_adm_memoria aux;
	int i = 0, espacioLibre = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	while(i < marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == -1)
		{
			espacioLibre++;
		}
		i++;
	}

	return espacioLibre;
}


/*
 * Funcion que atiende las conexiones de los clientes
 * */
void *connection_handler(void *socket_desc)
{

    int sock = *(int*)socket_desc;
    char orden;
    int resultadoDeEjecucion;
    orden = nuevaOrdenDeAccion(sock);/*<<<<<<<<<<<<<<<<NO MOVER DE ACA POR ALGUNA RAZON ROMPE SI SE LO PONE EN EL WHILE>>>>>>>>>*/

	while(orden != '\0')
	{
		switch(orden)
		{
		case 'A':
			printf("Inicializar Programa\n");
			resultadoDeEjecucion = main_inicializarPrograma(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'S':
			resultadoDeEjecucion = main_solicitarBytesPagina(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'C':
			resultadoDeEjecucion = main_almacenarBytesPagina(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'G':
			resultadoDeEjecucion = main_asignarPaginasAProceso(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'U':
			resultadoDeEjecucion = main_liberarPaginaProceso(sock);
			send(sock,&resultadoDeEjecucion,sizeof(int),0);
			break;
		case 'Q':
			return 0;
		case 'F':
			resultadoDeEjecucion = main_finalizarPrograma(sock);
			break;
		case 'P':
			send(sock,&marco_size,sizeof(int),0);
			break;
		case 'X':
			gestionarCierreDeConexion(sock);
			orden = '\0';
			break;
		default:
			log_error(logConsolaPantalla,"Orden no definida %c",orden);
			break;
		}
		log_info(logConsolaPantalla,"Resultado de ejecucion:%d\n",resultadoDeEjecucion);
		//imprimirBitMap();
		//imprimirEstructurasAdministrativas();
		if(orden != '\0') orden = nuevaOrdenDeAccion(sock);
	}

	log_warning(logConsolaPantalla,"Cliente %d desconectado",sock);
	return 0;
}

void gestionarCierreDeConexion(int socket){
	log_warning(logConsolaPantalla,"Desconectando cliente %d",socket);
	close(socket);
}

void *connection_Listener()
{
	int sock;



	while(1)
	{
		sock = recibirConexion(socket_servidor);
		//Quedo a la espera de CPUs y las atiendo
		sem_wait(&sem);//Es muy importante para que no rompa la Memoria
		if( pthread_create( &thread_id2 , NULL , connection_handler , &sock) < 0)
		{
			perror("could not create thread");
		}

	}
	pthread_join(thread_id2,NULL);
	return 0;
}

void escribirEstructuraAdmAMemoria(int pid, int frame, int cantPaginas, int cantPaginasAnteriores)
{
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	aux.pid=pid;
	aux.num_pag=cantPaginasAnteriores;
	aux.frame = frame;
	while(i < cantPaginas)
	{
		memcpy(bloque_Memoria + frame*desplazamiento + i*desplazamiento, &aux, sizeof(struct_adm_memoria));
		aux.num_pag++;
		aux.frame++;
		i++;
	}
}

void borrarProgramDeStructAdms(int pid)
{
	int i = sizeStructsAdmMemoria;
	int desplazamiento = sizeof(struct_adm_memoria);
	struct_adm_memoria aux;
	while(i<marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento,sizeof(struct_adm_memoria)); //Revisar si esto funciona
		if(aux.pid == pid) //Si el PID del programa en mi estructura Aadministrativa es igual al del programa que quiero borrar
		{
			//liberarBitMap(aux.frame,1); //Marco la posición del frame que me ocupa esa página en particular de ese programa como vacía
			aux.num_pag = -1;
			aux.pid = -1;
			memcpy(bloque_Memoria + i*desplazamiento,&aux,sizeof(struct_adm_memoria)); //Marco que en ese frame no tengo un programa asignado
		}
		i++;
	}
}

int buscarFrameDePaginaDeProceso(int pid, int pagina)
{
	int valorHash = funcionHash(pid,pagina);
	int i = valorHash;
	int desplazamiento = sizeof(struct_adm_memoria);
	struct_adm_memoria aux;
	while(i<marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento,sizeof(struct_adm_memoria));
		if(aux.pid == pid && aux.num_pag == pagina) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero borrar
		{
			return aux.frame;
		}
		i++;
	}
	i = sizeStructsAdmMemoria;
	while(i<valorHash)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento,sizeof(struct_adm_memoria));
		if(aux.pid == pid && aux.num_pag == pagina) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero borrar
		{
			return aux.frame;
		}
		i++;
	}


	return -1;
}

void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP:%s\nPUERTO:%s\nMARCOS:%d\nTAMAÑO MARCO:%d\nENTRADAS CACHE:%d\nCACHE POR PROCESOS:%d\nRETARDO MEMORIA:%d\n",ipMemoria,puertoMemoria,marcos,marco_size,entradas_cache,cache_x_proc,retardo_memoria);
		printf("---------------------------------------------------\n");
}

/*void imprimirBitMap()
{
	printf("BitMap:%s\n",bitMap);
}
*/

void imprimirEstructurasAdministrativas()
{
	struct_adm_memoria auxMemoria;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	log_info(logConsolaPantalla,"---Estructuras Adm De la Memoria---\n");
	log_info(logConsolaPantalla,"Frame/PID/NumPag\n");
	while(i < marcos)
	{
		memcpy(&auxMemoria, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		i++;
		log_info(logConsolaPantalla,"/___%d/__%d/__%d\n",auxMemoria.frame,auxMemoria.pid,auxMemoria.num_pag);
	}

	int pidProc;
	int paginaProc;
	i = 0;
	desplazamiento = sizeof(int)*2+marco_size;
	log_info(logConsolaPantalla,"---Estructuras Adm De la Cache---\n");
	log_info(logConsolaPantalla,"PID/NumPag\n");
	while(i < entradas_cache)
	{
		memcpy(&pidProc, bloque_Cache + i*desplazamiento,sizeof(int));
		memcpy(&paginaProc, bloque_Cache + i*desplazamiento+sizeof(int),sizeof(int));
		i++;
		log_info(logConsolaPantalla,"/__%d/__%d\n",pidProc,paginaProc);
	}
	printf("---------------------------------------------------\n");
}

int buscarFrameVacio()
{
	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	while(i < marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == -1)
		{
			return aux.frame;
		}
		i++;
	}
	return -1;
}

int cantPaginasDeProceso(int pid)
{

	struct_adm_memoria aux;
	int i = 0;
	int desplazamiento = sizeof(struct_adm_memoria);
	int cantPaginas = 0;
	while(i < marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamiento, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			cantPaginas ++;
		}
		i++;
	}
	return cantPaginas;
}

void inicializarLog(char *rutaDeLog)
{
	mkdir("/home/utnso/Log",0755);

	logConsola = log_create(rutaDeLog,"Memoria", false, LOG_LEVEL_INFO);
	logConsolaPantalla = log_create(rutaDeLog,"Memoria", true, LOG_LEVEL_INFO);
}

void interfazHandler()
{
	char orden;
	printf("Esperando una orden para la interfaz\nR-Modificar Retardo\nD-Dump\nF-Flush\nS-Size\n");
	scanf(" %c",&orden);
	while(orden != 'Q')
	{
		switch(orden)
		{
			case 'R':
			{
				modificarRetardo();
				break;
			}
			case 'D':
			{
				dumpDeMemoria();
				break;
			}
			case 'F':
			{
				flush();
				break;
			}
			case 'S':
			{
				size();
				break;
			}
			default:
			{
				log_warning(logConsolaPantalla,"Error: Orden de interfaz no reconocida, ingrese una orden nuevamente\n");
				break;
			}
		}
		printf("Esperando una orden para la interfaz\nR-Modificar Retardo\nD-Dump\nF-Flush\nS-Size\n");
		scanf(" %c",&orden);
	}
}

void modificarRetardo()
{
	printf("Ingrese el nuevo retardo de la memoria\n");
	scanf("%d",&retardo_memoria);
	log_info(logConsolaPantalla,"El retardo de la memoria a sido cambiado a: %d\n",retardo_memoria);
}

void dumpDeMemoria()
{
	printf("Elija qué sección de la memoria desea imprimir\n");
	printf("C-Dump De Cache\nE-Dump de estructuras administrativas\nM-Contenido en Memoria\n");
	char orden;
	scanf(" %c",&orden);
	switch(orden)
	{
		case 'C':
		{
			dumpCache();
			break;
		}
		case 'E':
		{
			imprimirEstructurasAdministrativas();
			break;
		}
		case 'M':
		{
			contenidoDeMemoria();
			break;
		}
		default:
		{
			log_warning(logConsolaPantalla,"Error: Orden de interfaz no reconocida\n");
			break;
		}
	}
}

void flush()
{
	log_info(logConsolaPantalla,"--Flush--\n");
	vaciarCache();
}

void size()
{
	printf("Elija una opcion\n");
	printf("P-Size Proceso\nM-Size Memoria\n");
	char orden;
	scanf(" %c",&orden);
	switch(orden)
	{
		case 'P':
		{
			tamanioProceso();
			break;
		}
		case 'M':
		{
			tamanioMemoria();
			break;
		}
		default:
		{
			log_warning(logConsolaPantalla,"Error: Orden de interfaz no reconocida\n");
			break;
		}
	}
}

void dumpCache()
{
	log_info(logConsolaPantalla,"----Dump Cache ----\n");
	int i = 0;
	int desplazamiento = sizeof(int)*2+marco_size;
	void*contenido = malloc(marco_size);
	int pid;
	int pagina;


	while(i < entradas_cache)
	{
		memcpy(&pid,bloque_Cache + i*desplazamiento,sizeof(int));
		memcpy(&pagina,bloque_Cache + i*desplazamiento + sizeof(int),sizeof(int));
		memcpy(contenido,bloque_Cache + i*desplazamiento + sizeof(int)*2,marco_size);
		if(pid != -1){
			log_info(logConsolaPantalla,"PID:%d\n",pid);
			log_info(logConsolaPantalla,"Pagina:%d\n",pagina);
			log_info(logConsolaPantalla,"Contenido:%s\n",(char*) contenido);
		}

		i++;
	}
	free(contenido);

}

void contenidoDeMemoria()
{
	printf("--Contenido De Memoria--\n");
	printf("T-Mostrar datos almacenados de todos los procesos\nU-Datos almacenados de todos los procesos\n");
	char orden;
	scanf(" %c",&orden);
	switch(orden)
	{
		case 'T':
		{
			datosAlmacenadosEnMemoria();
			break;
		}
		case 'U':
		{
			datosAlmacenadosDeProceso();
			break;
		}
		default:
		{
			log_warning(logConsolaPantalla,"Error: Orden de interfaz no reconocida\n");
			break;
		}
	}
}

void datosAlmacenadosDeProceso()
{
	log_info(logConsolaPantalla,"--Datos Almacenados De Proceso--\n");
	printf("Ingrese el PID del proceso:\n");
	int pid;
	scanf("%d",&pid);
	log_info(logConsolaPantalla,"PID:%d\n",pid);
	struct_adm_memoria aux;
	int i = 0;
	int desplazamientoStruct = sizeof(struct_adm_memoria);
	char* datosFrame = malloc(marco_size);
	while(i< marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamientoStruct, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			memcpy(datosFrame, bloque_Memoria + aux.frame*marco_size, marco_size);
			log_info(logConsolaPantalla,"%s\n",datosFrame);
		}
		i++;
	}
	free(datosFrame);
}

void datosAlmacenadosEnMemoria()
{
	log_info(logConsolaPantalla,"--Datos Almacenados En Memoria--\n");

	struct_adm_memoria aux;
	int i = 0;
	int desplazamientoStruct = sizeof(struct_adm_memoria);
	char* datosFrame = malloc(marco_size);
	while(i< marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamientoStruct, sizeof(struct_adm_memoria));
		if(aux.pid != -9 && aux.pid != -1)
		{
			log_info(logConsolaPantalla,"PID:%d\nFrame:%d\n",aux.pid,aux.frame);
			memcpy(datosFrame, bloque_Memoria + aux.frame*marco_size, marco_size);
			log_info(logConsolaPantalla,"Datos:%s\n",datosFrame);
		}
		i++;
	}
	free(datosFrame);
}

void vaciarCache()
{
	int i = 0;
	int desplazamiento = sizeof(int)*2+marco_size;
	int pid = -1;
	int pagina = -1;

	while(i < entradas_cache)
	{
		memcpy(bloque_Cache + i*desplazamiento,&pid , sizeof(int));
		memcpy(bloque_Cache + i*desplazamiento+sizeof(int),&pagina , sizeof(int));
		i++;
	}

	i = 0;
	int bitUso = -1;
	desplazamiento = sizeof(int);

	while(i < entradas_cache)
	{
		memcpy(bloqueBitUsoCache + i*desplazamiento,&bitUso , sizeof(int));
		i++;
	}
	log_info(logConsolaPantalla,"Cache Vaciada\n");

}

void tamanioProceso()
{
	log_info(logConsolaPantalla,"--Tamaño Proceso--\n");
	printf("Ingrese el PID del proceso:\n");
	int pid;
	scanf("%d",&pid);
	log_info(logConsolaPantalla,"PID:%d\n",pid);
	struct_adm_memoria aux;
	int i = 0,espacioTotal =0;
	int desplazamientoStruct = sizeof(struct_adm_memoria);

	while(i< marcos)
	{
		memcpy(&aux, bloque_Memoria + i*desplazamientoStruct, sizeof(struct_adm_memoria));
		if(aux.pid == pid)
		{
			espacioTotal++;
		}
		i++;
	}
	log_info(logConsolaPantalla,"El proceso %d ocupa %d paginas\n",pid,espacioTotal);
}

void tamanioMemoria()
{
	int espacioLibre = verificarEspacioLibre();
	int espacioOcupado = marcos - espacioLibre;
	log_info(logConsolaPantalla,"Frames Totales:%d\nEspacios Libres:%d\nEspacios Ocupados:%d\n",marcos,espacioLibre,espacioOcupado);
}

void inicializarCache()
{
	log_info(logConsolaPantalla,"-------------Inicializar Cache-------------\n");

	int i = 0;
	int desplazamiento = sizeof(int)*2+marco_size;
	int pid = -1;
	int pagina = -1;

	while(i < entradas_cache)
	{
		memcpy(bloque_Cache + i*desplazamiento,&pid , sizeof(int));
		memcpy(bloque_Cache + i*desplazamiento+sizeof(int),&pagina , sizeof(int));
		i++;
	}

	i = 0;
	int bitUso = -1;
	desplazamiento = sizeof(int);

	while(i < entradas_cache)
	{
		memcpy(bloqueBitUsoCache + i*desplazamiento,&bitUso , sizeof(int));
		i++;
	}


}

int buscarEntradaDeProcesoEnCache(int pid, int pagina)
{
	int i = 0;
	int desplazamiento = sizeof(int)*2+marco_size;
	int pidProc;
	int paginaProc;
	while(i<entradas_cache)
	{
		memcpy(&pidProc, bloque_Cache + i*desplazamiento,sizeof(int));
		memcpy(&paginaProc, bloque_Cache + i*desplazamiento+sizeof(int),sizeof(int));
		if(pidProc == pid && paginaProc == pagina) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero buscar
		{
			return i;
		}
		i++;
	}
	return -1;
}

int cantidadEntradasDeProcesoEnCache(int pid)
{
	int i = 0;
	int desplazamiento = sizeof(int)*2+marco_size;
	int pidProc;
	int contador = 0;
	while(i<entradas_cache)
	{
		memcpy(&pidProc, bloque_Cache + i*desplazamiento,sizeof(int));
		if(pidProc == pid) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero buscar
		{
			contador++;
		}
		i++;
	}
	return contador;
}

void iniciarEntradaEnCache(int pid, int pagina)
{
	int entrada = buscarUnaEntradaParaProcesoEnCache();

	log_info(logConsolaPantalla,"Iniciar Entrada en cache\nEntrada:%d\n",entrada);
	int desplazamiento = sizeof(int)*2+marco_size;

	char *contenidoReal = malloc(marco_size);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);

	log_info(logConsolaPantalla,"Frame a meter en la cache:%d\n",frame);

	memcpy(contenidoReal,bloque_Memoria + frame*marco_size,marco_size);

	log_info(logConsolaPantalla,"Contenido a meter en la cache:%s\n",contenidoReal);

	memcpy(bloque_Cache + entrada*desplazamiento,&pid ,sizeof(int));
	memcpy(bloque_Cache + entrada*desplazamiento+sizeof(int),&pagina ,sizeof(int));
	memcpy(bloque_Cache+entrada*desplazamiento+2*sizeof(int),contenidoReal,marco_size);
	memcpy(bloqueBitUsoCache + entrada*sizeof(int),&contadorBitDeUso , sizeof(int));
	free(contenidoReal);
	contadorBitDeUso++;
}

void actualizarEntradaEnCache(int pid, int pagina)
{

	int desplazamiento = sizeof(int)*2+marco_size;

	char *contenidoReal = malloc(marco_size);
	int frame = buscarFrameDePaginaDeProceso(pid,pagina);
	int entrada = buscarEntradaDeProcesoEnCache(pid,pagina);

	memcpy(contenidoReal,bloque_Memoria + frame*marco_size,marco_size);


	memcpy(bloque_Cache + entrada*desplazamiento,&pid ,sizeof(int));
	memcpy(bloque_Cache + entrada*desplazamiento+sizeof(int),&pagina ,sizeof(int));
	memcpy(bloque_Cache+entrada*desplazamiento+2*sizeof(int),contenidoReal,marco_size);
	memcpy(bloqueBitUsoCache + entrada*sizeof(int),&contadorBitDeUso , sizeof(int));
	free(contenidoReal);
	contadorBitDeUso++;
}

void leerContenidoEnCache(int entrada,char** buffer,int size,int offset)
{
	int desplazamiento = marco_size + sizeof(int)*2;
	memcpy(*buffer, bloque_Cache + entrada*desplazamiento+sizeof(int)*2 + offset,size);

	strcpy(*buffer + size , "\0");
	log_info(logConsolaPantalla,"%s\n",*buffer);
	memcpy(bloqueBitUsoCache + entrada*sizeof(int),&contadorBitDeUso , sizeof(int));
	contadorBitDeUso++;
}

int buscarUnaEntradaParaProcesoEnCache()
{
	int i=0;
	int bitDeUso;
	int bitDeUsoAux;
	int entrada = 0;
	while(i < entradas_cache)
	{
		if(i == 0)
		{
			memcpy(&bitDeUso,bloqueBitUsoCache + i*sizeof(int),sizeof(int));
			if(bitDeUso == -1)
			{
				return i;
			}
		}
		else
		{
			memcpy(&bitDeUsoAux,bloqueBitUsoCache + i*sizeof(int),sizeof(int));
			if(bitDeUsoAux == -1)
			{
				return i;
			}
			if(bitDeUsoAux <= bitDeUso)
			{
				bitDeUso = bitDeUsoAux;
				entrada = i;
			}
		}
		i++;
	}
	return entrada;
}

void borrarEntradasDeProcesoEnCache(int pid)
{
	int desplazamiento = marco_size + sizeof(int)*2;
	int pidAux;
	int i = 0;
	while(i < entradas_cache)
	{
		memcpy(&pidAux, bloque_Cache + i*desplazamiento,sizeof(int));
		if(pidAux == pid) //Si el PID del programa en mi estructura Administrativa es igual al del programa que quiero borrar
		{
			pidAux = -1;
			memcpy(bloque_Cache + i*desplazamiento,&pidAux ,sizeof(int));
			memcpy(bloque_Cache + i*desplazamiento + sizeof(int),&pidAux ,sizeof(int));
			//Marco esa entrada como vacía
		}
		i++;
	}
}

int funcionHash(int pid, int pagina){
	return (sizeStructsAdmMemoria + (pid-1) + pagina)%marcos;
}






