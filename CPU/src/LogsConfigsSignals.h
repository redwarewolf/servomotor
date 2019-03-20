//-----------------------------LOGS, CONFIGS Y SIGNALS------------------------------------------------------

#include <stdbool.h>
#include <signal.h>


void leerConfiguracion(char* ruta) {
	configuracion_memoria = config_create(ruta);
	ipMemoria = config_get_string_value(configuracion_memoria, "IP_MEMORIA");
	puertoKernel= config_get_string_value(configuracion_memoria, "PUERTO_KERNEL");
	puertoMemoria = config_get_string_value(configuracion_memoria,"PUERTO_MEMORIA");
	ipKernel = config_get_string_value(configuracion_memoria,"IP_KERNEL");
}

void imprimirConfiguraciones(){
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nPUERTO KERNEL:%s\nIP KERNEL:%s\nPUERTO MEMORIA:%s\nIP MEMORIA:%s\n",puertoKernel,ipKernel,puertoMemoria,ipMemoria);
	printf("---------------------------------------------------\n");
}
void inicializarLog(char *rutaDeLog){

	mkdir("/home/utnso/Log",0755);

	logConsola = log_create(rutaDeLog,"CPU", false, LOG_LEVEL_INFO);
	logConsolaPantalla = log_create(rutaDeLog,"CPU", true, LOG_LEVEL_INFO);
}
void signalHandler(int signum)
{

    if (signum == SIGUSR1 || signum == SIGINT)
    {
    	if(procesoFinalizado) {
    		log_warning(logConsolaPantalla,"Cierre por signal,cerrando CPU ...");
    		cerrarTodo();
    		exit(1);
    	}
    	log_warning(logConsolaPantalla,"Cierre por signal, ejecutando ultimas instrucciones del proceso y cerrando CPU ...");
    	cpuFinalizada = 1;
    }
}
void cerrarTodo(){

	char comandoInterruptHandler='X';
	char comandoCierreCpu='C';
	char comandoCerrarMemoria = 'X';


	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoCierreCpu,sizeof(char),0);
	send(socketMemoria,&comandoCerrarMemoria,sizeof(char),0);

}

