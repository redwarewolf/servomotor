#include "PrimitivasDeInstrucciones.h"
#include "PrimitivasKernel.h"
#include "ManejoInstrucciones.h"
#include "Lecto-EscrituraMemoria.h"
#include "ManejoPCB.h"
#include "PrimitivasFS.h"
#include "LogsConfigsSignals.h"
#include "Interrupciones.h"

int main(void) {
	leerConfiguracion("/home/utnso/workspace/tp-2017-1c-servomotor/CPU/config_CPU");
	imprimirConfiguraciones();
	inicializarLog("/home/utnso/Log/logCPU.txt");

	socketKernel = crear_socket_cliente(ipKernel,puertoKernel);
	socketMemoria = crear_socket_cliente(ipMemoria,puertoMemoria);

	log_info(logConsolaPantalla, "Inicia modulo CPU\n");

	signal(SIGUSR1, signalHandler);
	signal(SIGINT, signalHandler);

	recibirTamanioPagina(socketKernel);

	enviarAlKernelPedidoDeNuevoProceso(socketKernel);

	esperarPCB();

	cerrarTodo();

	return 0;
}



//-----------------------------PEDIDOS AL KERNEL-----------------------------------------
void enviarAlKernelPedidoDeNuevoProceso(int socketKernel){
	char comandoGetNuevoProceso = 'N';
	send(socketKernel,&comandoGetNuevoProceso,sizeof(char),0);
	log_info(logConsolaPantalla,"Se hizo el pedido a Kernel para comenzar a ejecutar Script ANSISOP\n");
}
void recibirYMostrarAlgortimoDePlanificacion(int socketKernel){
	recv(socketKernel,&quantum,sizeof(int),0);
	recv(socketKernel,&retardo_entre_instruccion,sizeof(int),0);

	log_info(logConsolaPantalla,"Quantum:%d\n",quantum);
	retardo_entre_instruccion=retardo_entre_instruccion/1000;
	log_info(logConsolaPantalla,"Quantum sleep:%d ms\n",retardo_entre_instruccion);

	cantidadInstruccionesAEjecutarPorKernel = quantum;

		if(quantum==0){
			log_info(logConsolaPantalla,"\nAlgoritmo FIFO\n");
			return;
		}else log_info(logConsolaPantalla,"\nAlgoritmo: RR de Q:%d\n", quantum);
		return;
}
//-----------------------------PEDIDOS AL KERNEL-----------------------------------------


int cantidadPaginasTotales(){
	int paginasTotales= (config_stackSize + pcb_actual->cantidadPaginasCodigo);
	return paginasTotales;
}




char* devolverStringFlags(t_banderas flags){
	char *flagsAConcatenar = string_new();
	if(flags.creacion==true){
		string_append(&flagsAConcatenar, "c");
	}
	if(flags.lectura==true){
		string_append(&flagsAConcatenar, "r");
	}
	if(flags.escritura==true){
		string_append(&flagsAConcatenar, "w");
	}

	return flagsAConcatenar;
}

