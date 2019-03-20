/*
 ============================================================================
 Name        : Consola.c
 Author      : Servomotor
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "Consola.h"
#include "hiloPrograma.h"


void signalHandler(int signum);

int main(void) {

	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/Consola/config_Consola");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logConsola.txt");
	inicializarListas();
	inicializarSemaforos();
	flagCerrarConsola = 1;

	socketKernel = crear_socket_cliente(ipKernel, puertoKernel);
	log_info(logConsola,"Creando hilo interfaz usuario\n");
	int err = pthread_create(&hiloInterfazUsuario, NULL, (void*)connectionHandler,NULL);
	if (err != 0) log_error(logConsola,"Error al crear el hilo :[%s]", strerror(err));

	signal(SIGINT, signalHandler);

	pthread_join(hiloInterfazUsuario, NULL);
	log_warning(logConsolaPantalla,"La Consola ha finalizado\n");
	return 0;

}

void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
    	log_warning(logConsolaPantalla,"Finalizando consola\n");
    	cerrarTodo();
    	pthread_kill(hiloInterfazUsuario,SIGUSR1);
    }
    if(signum==SIGUSR1){
    	log_warning(logConsola,"Finalizando hilo de Interfaz de Usuario\n");
    	int exitCode;
    	pthread_exit(&exitCode);
    }
}

void connectionHandler() {

	signal(SIGUSR1,signalHandler);
	char orden;
	int cont=0;
	imprimirInterfaz();

	while (flagCerrarConsola) {
		pthread_mutex_lock(&mutex_crearHilo);
		scanf("%c", &orden);
		log_info(logConsola,"Orden definida %d",orden);
		cont++;

		switch (orden) {
			case 'I':
				crearHiloPrograma();
				break;
			case 'F':
				finalizarPrograma();
				break;
			case 'C':
				limpiarPantalla();
				break;
			case 'Q':
				cerrarTodo();
				break;
			case 'U':
				imprimirInterfaz();
				pthread_mutex_unlock(&mutex_crearHilo);
				break;
			default:
				if(cont!=2){
					log_error(logConsola,"Orden %c no definida", orden);
					cont=0;
				}
				else cont=0;
				pthread_mutex_unlock(&mutex_crearHilo);
				break;
			}

	}
}

void limpiarPantalla(){
	system("clear");
	log_info(logConsola,"Limpiando pantalla");
	imprimirInterfaz();
	pthread_mutex_unlock(&mutex_crearHilo);
}



void cerrarTodo(){
	char comandoInterruptHandler='X';
	char comandoCierreConsola = 'E';
	int i;
	int desplazamiento = 0;

	pthread_mutex_lock(&mutexListaHilos);
	int cantidad= listaHilosProgramas->elements_count;
	flagCerrarConsola = 0;

	log_info(logConsola,"Informando Kernel el cierre de la Consola\n");
	if(cantidad == 0) {
		int bufferProcesosSize = sizeof(int);
		char* mensaje = malloc(sizeof(char)*2 + sizeof(int)*2);

		memcpy(mensaje + desplazamiento,&comandoInterruptHandler,sizeof(char));
		desplazamiento += sizeof(char);

		memcpy(mensaje + desplazamiento,&comandoCierreConsola,sizeof(char));
		desplazamiento += sizeof(char);

		memcpy(mensaje + desplazamiento,&bufferProcesosSize,sizeof(int));
		desplazamiento += sizeof(int);

		memcpy(mensaje + desplazamiento,&cantidad,sizeof(int));
		desplazamiento += sizeof(int);

		send(socketKernel,mensaje,sizeof(char)*2+sizeof(int)*2,0);

		free(mensaje);
		return;
	}

	int bufferSize = sizeof(char)*2 +sizeof(int)*2 + sizeof(int)* cantidad;
	int bufferProcesosSize = sizeof(int) + sizeof(int)*cantidad;

	char* mensaje= malloc(bufferSize);

	t_hiloPrograma* programaAbortar = malloc(sizeof(t_hiloPrograma));

	memcpy(mensaje + desplazamiento,&comandoInterruptHandler,sizeof(char));
	desplazamiento += sizeof(char);

	memcpy(mensaje + desplazamiento,&comandoCierreConsola,sizeof(char));
	desplazamiento+=sizeof(char);

	memcpy(mensaje + desplazamiento,&bufferProcesosSize,sizeof(int));
	desplazamiento +=sizeof(int);

	memcpy(mensaje+desplazamiento,&cantidad,sizeof(int));
	desplazamiento += sizeof(int);

	for(i=0;i<cantidad;i++){
		programaAbortar = (t_hiloPrograma*) list_get(listaHilosProgramas,i);
		memcpy(mensaje + desplazamiento,&programaAbortar->pid,sizeof(int));
		desplazamiento += sizeof(int);
		informarEstadisticas(programaAbortar);
	}
	pthread_mutex_unlock(&mutexListaHilos);

	send(socketKernel,mensaje,bufferSize,0);

	list_destroy_and_destroy_elements(listaHilosProgramas,free);
	free(mensaje);
}
void recibirDatosDelKernel(int socketHiloKernel){
	int pid;
	int size;
	int flagCerrarHilo=1;
	char* mensaje;

	recv(socketHiloKernel, &pid, sizeof(int), 0);
	log_info(logConsolaPantalla,"Al Programa ANSISOP en socket: %d se le ha asignado el PID: %d\n", socketHiloKernel,pid);

	cargarHiloPrograma(pid,socketHiloKernel);
	pthread_mutex_unlock(&mutex_crearHilo);

	while(flagCerrarHilo){
		recv(socketHiloKernel,&size,sizeof(int),0);

		pthread_mutex_lock(&mutexRecibirDatos);


		mensaje = malloc(size * sizeof(char) + sizeof(char));
		recv(socketHiloKernel,mensaje,size,0);
		strcpy(mensaje+size,"\0");

		if(strcmp(mensaje,"F")==0) {
			flagCerrarHilo = 0;
			free(mensaje);
			break;
		}
		printf("Informacion para programa--->PID:%d\n",pid);

		printf("\033[22;32m%s\033[0m\n",mensaje);
		actualizarCantidadImpresiones(pid);
		free(mensaje);
		pthread_mutex_unlock(&mutexRecibirDatos);
	}
	gestionarCierrePrograma(pid);
	log_warning(logConsolaPantalla,"Programa ANSISOP--->PID:%d ha finalizado\n",pid);
	pthread_mutex_unlock(&mutexRecibirDatos);
}

void actualizarCantidadImpresiones(int pid){
	bool verificaPid(t_hiloPrograma* proceso){
			return (proceso->pid == pid);
		}
	pthread_mutex_lock(&mutexListaHilos);
	t_hiloPrograma* programa = list_remove_by_condition(listaHilosProgramas,(void*) verificaPid);
	programa->cantImpresiones += 1;
	list_add(listaHilosProgramas,programa);
	pthread_mutex_unlock(&mutexListaHilos);
}

void leerConfiguracion(char* ruta) {
	configuracion_Consola = config_create(ruta);
	ipKernel = config_get_string_value(configuracion_Consola, "IP_KERNEL");
	puertoKernel = config_get_string_value(configuracion_Consola,"PUERTO_KERNEL");
}

void imprimirConfiguraciones() {

	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP KERNEL:%s\nPUERTO KERNEL:%s\n", ipKernel,
			puertoKernel);
	printf("---------------------------------------------------\n");
}

void imprimirInterfaz(){
	printf("----------------------------------------------------------------------\n");
	printf("Ingresar orden:\n 'I' para iniciar un programa AnSISOP\n 'F' para finalizar un programa AnSISOP\n"
			" 'C' para limpiar la pantalla\n 'Q' para desconectar esta Consola\n 'U' para imprimir Interfaz de Usuario\n");
	printf("----------------------------------------------------------------------\n");
}

void inicializarLog(char *rutaDeLog){
		mkdir("/home/utnso/Log",0755);
		logConsola = log_create(rutaDeLog,"Consola", false, LOG_LEVEL_INFO);
		logConsolaPantalla = log_create(rutaDeLog,"Consola", true, LOG_LEVEL_INFO);
}

void inicializarListas(){
	listaHilosProgramas= list_create();
}

void inicializarSemaforos(){
	pthread_mutex_init(&mutex_crearHilo,NULL);
	pthread_mutex_init(&mutexListaHilos,NULL);
	pthread_mutex_init(&mutexRecibirDatos,NULL);
	sem_init(&sem_crearHilo,0,1);
}
