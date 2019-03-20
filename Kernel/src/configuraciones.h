/*
 * configuraciones.h
 *
 *  Created on: 27/5/2017
 *      Author: utnso
 */

#ifndef CONFIGURACIONES_H_
#define CONFIGURACIONES_H_


void leerConfiguracion(char* ruta);
void imprimirConfiguraciones();
char* leerValoresSemaforos();
char* leerSemaforos();
char* leerGlobales();
t_config* configuracion_kernel;

int config_quantum;
int config_quantumSleep;
char *config_algoritmo;
char **semId;
char **semInit;
char **shared_vars;
int config_stackSize;
int config_paginaSize;
int config_gradoMultiProgramacion;
int gradoMultiProgramacion;

char *ipServidor;
char *ipMemoria;
char *ipFileSys;
char *puertoServidor;
char *puertoMemoria;
char *puertoFileSys;




void imprimirConfiguraciones() {
	printf("---------------------------------------------------\n");
	printf("CONFIGURACIONES\nIP SERVIDOR:%s\nPUERTO SERVIDOR:%s\nIP MEMORIA:%s\nPUERTO MEMORIA:%s\nIP FS:%s\nPUERTO FS:%s\n",
			ipServidor,puertoServidor,ipMemoria,puertoMemoria,ipFileSys,puertoFileSys);
	printf("---------------------------------------------------\n");
	printf(	"QUANTUM:%d\nQUANTUM SLEEP:%d\nALGORITMO:%s\nGRADO MULTIPROG:%d\nSEM IDS:%s\nSEM INIT:%s\nSHARED VARS:%s\nSTACK SIZE:%d\n",
			config_quantum,config_quantumSleep, config_algoritmo, config_gradoMultiProgramacion, leerSemaforos(), leerValoresSemaforos(), leerGlobales()
			, config_stackSize);
	printf("---------------------------------------------------\n");

}

void leerConfiguracion(char* ruta) {

	configuracion_kernel = config_create(ruta);


	ipServidor = config_get_string_value(configuracion_kernel, "IP_SERVIDOR");
	puertoServidor = config_get_string_value(configuracion_kernel,"PUERTO_SERVIDOR");
	ipMemoria = config_get_string_value(configuracion_kernel, "IP_MEMORIA");
	puertoMemoria = config_get_string_value(configuracion_kernel,"PUERTO_MEMORIA");
	ipFileSys = config_get_string_value(configuracion_kernel, "IP_FS");
	puertoFileSys = config_get_string_value(configuracion_kernel, "PUERTO_FS");
	config_quantum = atoi(config_get_string_value(configuracion_kernel, "QUANTUM"));
	config_quantumSleep = atoi(config_get_string_value(configuracion_kernel,"QUANTUM_SLEEP"));
	config_algoritmo = config_get_string_value(configuracion_kernel, "ALGORITMO");
	config_gradoMultiProgramacion = atoi(config_get_string_value(configuracion_kernel,"GRADO_MULTIPROGRAMACION"));
	semId = config_get_array_value(configuracion_kernel, "SEM_IDS");
	semInit = config_get_array_value(configuracion_kernel, "SEM_INIT");
	shared_vars = config_get_array_value(configuracion_kernel, "SHARED_VARS");
	config_stackSize = atoi(config_get_string_value(configuracion_kernel, "STACK_SIZE"));
}

char* leerGlobales(){
	char* globales=string_new();
	int i=0;

	string_append(&globales,"[");

	while(shared_vars[i]!=NULL){

		string_append(&globales,shared_vars[i]);
		if(shared_vars[i+1]!=NULL)string_append(&globales,",");
		i++;
	}

	string_append(&globales,"]");
	return globales;
}
char* leerSemaforos(){
	char* semaforos=string_new();
	int i=0;

	string_append(&semaforos,"[");

	while(semId[i]!=NULL){
		string_append(&semaforos,semId[i]);
		if(semId[i+1]!=NULL)string_append(&semaforos,",");
		i++;
	}

	string_append(&semaforos,"]");
	return semaforos;
}
char* leerValoresSemaforos(){
	char* sem_values=string_new();
		int i=0;

		string_append(&sem_values,"[");

		while(semInit[i]!=NULL){
			string_append(&sem_values,semInit[i]);
			if(semInit[i+1]!=NULL)string_append(&sem_values,",");
			i++;
		}

		string_append(&sem_values,"]");
		return sem_values;
}


#endif /* CONFIGURACIONES_H_ */
