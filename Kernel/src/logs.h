
#ifndef LOGS_H_
#define LOGS_H_
#include <commons/log.h>

t_log *logKernelPantalla;
t_log *logKernel;

void inicializarLog(char *rutaDeLog);

void inicializarLog(char *rutaDeLog){

		mkdir("/home/utnso/Log",0755);

		logKernel = log_create(rutaDeLog,"Kernel", false, LOG_LEVEL_INFO);
		logKernelPantalla = log_create(rutaDeLog,"Kernel", true, LOG_LEVEL_INFO);
}

#endif /* LOGS_H_ */
