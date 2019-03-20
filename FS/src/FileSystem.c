/*
 ============================================================================
 Name        : File.c
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
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <commons/config.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <sys/mman.h>
#include "sockets.h"
#include "logger.h"
#include "permisos.h"
#include "configuracionesLib.h"
#include "funcionesFS.h"


//----------------------------//

typedef struct {
	char* path;
	int sizeActual;
}t_archivo;

t_list* listaArchivos;

int socket_servidor;
void selectorConexiones();
void leerConfiguracion(char* ruta);
void leerConfiguracionMetadata();
void imprimirConfiguraciones();
void connectionHandler();

/*
void printFilePermissions(char* archivo);
int archivoEnModoEscritura(char* archivo);
int archivoEnModoLectura(char* archivo);
char* obtenerBytesDeUnArchivo(FILE *fp, int offset, int size);
char nuevaOrdenDeAccion(int puertoCliente);
*/
int socketServidor;
int finalizarFs = 0 ;

int main(void){
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/FS/config_FileSys");

	leerConfiguracionMetadata();

	imprimirConfiguraciones();

	inicializarLog("/home/utnso/Log/logFS.txt");

	//*********************************************************************
	inicializarMmap();

	bitarray = bitarray_create_with_mode(mmapDeBitmap,(config_tamanioBloques*config_cantidadBloques)/(8*config_tamanioBloques), LSB_FIRST);

	printf("El tamano del bitarray es de : %d\n",bitarray_get_max_bit(bitarray));

	/*int i; //Para testear cuando no existen mas bloques
	for(i=0;i<config_cantidadBloques;i++){
		bitarray_set_bit(bitarray,i);
	}
	*/

	//*********************************************************************
	socketServidor = crear_socket_servidor(ipFS,puertoFS);
	socketKernel=recibirConexion(socketServidor);
	while(!finalizarFs){
		connectionHandler();
	}
	log_warning(logConsolaPantalla,"Modulo Fs finalizado");
	return 0;
}
void inicializarLog(char *rutaDeLog){


		mkdir("/home/utnso/Log",0755);

		logConsola = log_create(rutaDeLog,"FileSystem", false, LOG_LEVEL_INFO);
		logConsolaPantalla = log_create(rutaDeLog,"FileSystem", true, LOG_LEVEL_INFO);

}



void connectionHandler()
{
	char orden;
	char* path;
	int pathSize;
	recv(socketKernel,&orden,sizeof(char),0);

	if(orden=='X') {
		close(socketKernel);
		finalizarFs=1;
	}

	recv(socketKernel,&pathSize,sizeof(int),0);
	path = malloc(pathSize + sizeof(char));
	recv(socketKernel,path,pathSize,0);
	strcpy(path + pathSize , "\0");

    	switch(orden){
		case 'V'://validar archivo
			validarArchivoFunction(path);
			break;
		case 'C'://crear archivo
			crearArchivoFunction(path);
			break;
		case 'B'://borrar archivo
			borrarArchivoFunction(path);
			break;
		case 'O'://obtener datos
			obtenerDatosArchivoFunction(path);
			break;
		case 'G'://guardar archivo
			guardarDatosArchivoFunction(path);
			break;
		default:
			log_error(logConsolaPantalla,"Orden no definida:%c",orden);
			break;
		}
    	orden = '\0';
}
void selectorConexiones() {
	log_info(logConsolaPantalla,"Iniciando selector de conexiones");
	int maximoFD;
	int nuevoFD;
	int socket;
	char orden;

	char remoteIP[INET6_ADDRSTRLEN];
	socklen_t addrlen;
	fd_set readFds;
	fd_set master;
	struct sockaddr_storage remoteaddr;// temp file descriptor list for select()


	if (listen(socketServidor, 15) == -1) {
	perror("listen");
	exit(1);
	}

	FD_SET(socketServidor, &master); // add the listener to the master set

	maximoFD = socketServidor; // keep track of the biggest file descriptor so far, it's this one

	while(1) {
					readFds = master;

					if (select(maximoFD + 1, &readFds, NULL, NULL, NULL) == -1) {
						perror("select");
						log_error(logConsola,"Error en select\n");
						exit(2);
					}

					for (socket = 0; socket <= maximoFD; socket++) {
							if (FD_ISSET(socket, &readFds)) { //Hubo una conexion

									if (socket == socketServidor) {
										addrlen = sizeof remoteaddr;
										nuevoFD = accept(socketServidor, (struct sockaddr *) &remoteaddr,&addrlen);

										if (nuevoFD == -1) perror("accept");

										else {
											FD_SET(nuevoFD, &master);
											if (nuevoFD > maximoFD)	maximoFD = nuevoFD;

											log_info(logConsolaPantalla,"Selectserver: nueva conexion en IP: %s en socket %d",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*) &remoteaddr),remoteIP, INET6_ADDRSTRLEN), nuevoFD);
										}
									}
									else if(socket != 0) {
											recv(socket, &orden, sizeof(char), 0);
											//connectionHandler(socket, orden);
									}
							}
					}
		}
	log_info(logConsolaPantalla,"Finalizando selector de conexiones");
}
