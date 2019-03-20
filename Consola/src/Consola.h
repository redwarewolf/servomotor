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
#include <commons/collections/list.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <time.h>
#include <signal.h>

//--------LOG----------------//
void inicializarLog(char *rutaDeLog);
t_log *logConsola;
t_log *logConsolaPantalla;
//--------LOG----------------//


//--------Configuraciones--------------//
void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
t_config* configuracion_Consola;
void imprimirInterfaz();
//--------Configuraciones--------------//

void inicializarListas();
void inicializarSemaforos();
void connectionHandler();
void finalizarPrograma();
void cerrarTodo();
void limpiarPantalla();
char* ipKernel;
char* puertoKernel;
int socketKernel;
pthread_t hiloInterfazUsuario;
int flagCerrarConsola;
