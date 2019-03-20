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
#include <commons/log.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <pthread.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include "conexiones.h"
#include <parser/metadata_program.h>
#include "PCB.h"
//-----------------------------------------------------------------------------------------------------------------
char* devolverStringFlags(t_banderas flags);
//-----------------------------------------------------------------------------------------------------------------
int almacenarDatosEnMemoria(char* buffer, int size,int paginaAGuardar,int offset);
int conseguirDatosMemoria (char** instruccion, int paginaSolicitada,int offset,int size);
//-----------------------------------------------------------------------------------------------------------------
void enviarAlKernelPedidoDeNuevoProceso(int socketKernel);
void recibirYMostrarAlgortimoDePlanificacion(int socketKernel);
//-----------------------------------------------------------------------------------------------------------------
void esperarPCB();
void establecerPCB();
void recibirPCB();
void imprimirPCB();
//-----------------------------------------------------------------------------------------------------------------
void ejecutarInstruccion();
void EjecutarProgramaMedianteAlgoritmo();
char* obtener_instruccion();
//-----------------------------------------------------------------------------------------------------------------
void signalHandler(int signum);
void cerrarTodo ();
int cantidadPaginasTotales();
//-----------------------------------------------------------------------------------------------------------------
void nuevaOrdenDeAccion(int socketCliente, char nuevaOrden);
void connectionHandlerKernel(int socketAceptado, char orden);
//-----------------------------------------------------------------------------------------------------------------
void expropiarVoluntariamente();
void expropiarPorRR();
void expropiarPorKernel();
void expropiarPorDireccionInvalida();
void expropiarPorStackOverflow();
void expropiarPorRRYCerrar();
void stackOverflow();
void expropiadoVoluntariamente();


//-----------------------------------------------------------------------------------------------------------------
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
void connectionHandler();
void recibirTamanioPagina();
//-----------------------------------------------------------------------------------------------------------------

t_config* configuracion_memoria;
char* puertoKernel;
char* puertoMemoria;
char* ipMemoria;
char* ipKernel;
//----------------------------//

//------------------Sockets Globales-------//
int socketMemoria;
int socketKernel;
int socketInterrupciones;
//-----------------------------------------//
pthread_t lineaInterrupciones;
t_pcb *pcb_actual;
int cpuOcupada=1;
int cpuFinalizadaPorSignal=1;
int cantidadInstruccionesAEjecutarPorKernel=0;
int cpuExpropiadaPorKernel=1;
int cpuBloqueadaPorSemANSISOP=1;
int cantidadInstruccionesEjecutadas=0;
int quantum;
int cantidadInstruccionesAEjecutarDelPcbActual;
int recibiPcb=1;
int retardo_entre_instruccion;
//-------------------------------------------------------------------------PRIMITIVAS------------------------------------//
//---------Primitivas Comunes----------//
t_puntero definirVariable(t_nombre_variable variable);
t_puntero obtenerPosicionVariable(t_nombre_variable variable);
void asignar(t_puntero puntero, t_valor_variable variable);
t_valor_variable dereferenciar(t_puntero puntero);
void finalizar();
void retornar(t_valor_variable retorno);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void irAlLabel(t_nombre_etiqueta etiqueta);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);

//---------Primitivas Kernel----------//
void wait(t_nombre_semaforo identificador_semaforo);
void signal_Ansisop(t_nombre_semaforo identificador_semaforo);
t_puntero reservar (t_valor_variable espacio);
void liberar (t_puntero puntero);
t_descriptor_archivo abrir_archivo(t_direccion_archivo direccion, t_banderas flags);
void borrar_archivo (t_descriptor_archivo descriptor_archivo);
void cerrar_archivo(t_descriptor_archivo descriptor_archivo);
void moverCursor_archivo (t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void leer_archivo(t_descriptor_archivo descriptor_archivo,t_puntero informacion, t_valor_variable tamanio);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
//-------------------------------------------------------------------------PRIMITIVAS------------------------------------//
typedef struct CPU {
	char* stringLeidoDeFs;
	int tamanio;
	int direccionLogicaHeap;
}t_leerFS;
t_leerFS * leerFS;
AnSISOP_funciones functions = {
	.AnSISOP_definirVariable	=definirVariable,
	.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
	.AnSISOP_finalizar =finalizar,
	.AnSISOP_dereferenciar	= dereferenciar,
	.AnSISOP_asignar	= asignar,
	 .AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
	 .AnSISOP_asignarValorCompartida = asignarValorCompartida,
	 .AnSISOP_irAlLabel = irAlLabel,
	 .AnSISOP_llamarSinRetorno=llamarSinRetorno, .AnSISOP_retornar = retornar,
	 .AnSISOP_llamarConRetorno = llamarConRetorno

};

AnSISOP_kernel kernel_functions = {
		.AnSISOP_wait= wait,
		.AnSISOP_signal = signal_Ansisop,
		.AnSISOP_reservar = reservar,
		.AnSISOP_liberar = liberar,
		.AnSISOP_abrir = abrir_archivo,
		.AnSISOP_borrar = borrar_archivo,
		.AnSISOP_cerrar = cerrar_archivo,
		.AnSISOP_moverCursor = moverCursor_archivo,
		.AnSISOP_escribir = escribir,
		.AnSISOP_leer = leer_archivo
};

