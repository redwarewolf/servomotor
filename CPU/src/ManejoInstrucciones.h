#include "Interrupciones.h"

//----------------------------Manejo Instrucciones-------------------------------------
char* obtener_instruccion(){
	log_info(logConsola,"Obteniendo una instruccion del PC= %d", pcb_actual->programCounter);
	int program_counter = pcb_actual->programCounter;
	int byte_inicio_instruccion = pcb_actual->indiceCodigo[program_counter][0];
	int bytes_tamanio_instruccion = pcb_actual->indiceCodigo[program_counter][1];
	int num_pagina = byte_inicio_instruccion / config_paginaSize;
	int offset = byte_inicio_instruccion - (num_pagina * config_paginaSize);//no es 0 porque evita el begin
	char* mensajeRecibido;
	char* mensajeRecibido2;
	char* instruccion;
	char* continuacion_instruccion;
	int bytes_a_leer_primera_pagina;

	if (bytes_tamanio_instruccion > (config_paginaSize * 2)){
		log_info(logConsolaPantalla,"El tamanio de la instruccion es mayor al tamanio de pagina\n");
		direccionInvalida();
		return 0;
	}
	if ((offset + bytes_tamanio_instruccion) < config_paginaSize){
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, bytes_tamanio_instruccion)<0)
			{
			log_info(logConsolaPantalla,"No se pudo solicitar el contenido\n");
			direccionInvalida();
			return 0;
			}
			else{
				instruccion=mensajeRecibido;
				}
	} else {
		bytes_a_leer_primera_pagina = config_paginaSize - offset;
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, bytes_a_leer_primera_pagina)<0)
					{
					log_info(logConsolaPantalla,"No se pudo solicitar el contenido\n");
					direccionInvalida();
					return 0;
					}
		else{
				instruccion=mensajeRecibido;
			}

		log_info(logConsolaPantalla, "Primer parte de instruccion: %s", instruccion);
		if((bytes_tamanio_instruccion - bytes_a_leer_primera_pagina) > 0){
			if ( conseguirDatosMemoria(&mensajeRecibido2,(num_pagina + 1),0,(bytes_tamanio_instruccion - bytes_a_leer_primera_pagina))<0)
					{log_info(logConsolaPantalla,"No se pudo solicitar el contenido\n");
					direccionInvalida();
					return 0;
					}
						else{
						continuacion_instruccion=mensajeRecibido2;
						log_info(logConsolaPantalla, "Continuacion ejecucion: %s", continuacion_instruccion);
								}

			string_append(&instruccion, continuacion_instruccion);
			free(continuacion_instruccion);
		}else{
			log_info(logConsola, "La continuacion de la instruccion es 0. Ni la leo");
		}
	}
	char** string_cortado = string_split(instruccion, "\n");
	free(instruccion);
	instruccion= string_new();
	string_append(&instruccion, string_cortado[0]);

	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);

	return instruccion;
}
void EjecutarProgramaMedianteAlgoritmo(){ //Fase busqueda: Buscar la instruccion en memoria - La decodificas - Buscas los operandos - Ejecutas - Chequea interrupciones

	cantidadInstruccionesAEjecutarDelPcbActual = pcb_actual->cantidadInstrucciones;

	log_info(logConsola,"La cantidad de instrucciones a ejecutar son %d\n",cantidadInstruccionesAEjecutarDelPcbActual);

	if(cantidadInstruccionesAEjecutarPorKernel==0){ //es FIFO
		while(!procesoFinalizado && cantidadInstruccionesAEjecutarPorKernel < cantidadInstruccionesAEjecutarDelPcbActual){

			ejecutarInstruccion();
			cantidadInstruccionesAEjecutarPorKernel++; //para FIFO en si
			cantidadInstruccionesEjecutadas++;//para contabilidad del kernel
			log_info(logConsola,"Cantidad de instrucciones ejecutadas %d\n", cantidadInstruccionesEjecutadas);

			if(verificaInterrupcion()>0) {
						expropiar();
						break;
					}
		}
	}else{//es RR con quantum = cantidadInstruccionesAEjecutarPorKernel
		procesoFinalizado=0;
		while (!procesoFinalizado && cantidadInstruccionesAEjecutarPorKernel > 0){
			ejecutarInstruccion();
			cantidadInstruccionesAEjecutarPorKernel--; //voy decrementando el Quantum que me dio el kernel hasta llegar a 0
			log_info(logConsola,"Quedan por ejecutar %d instrucciones", cantidadInstruccionesAEjecutarPorKernel);
			cantidadInstruccionesEjecutadas++;//para contabilidad del kernel
			log_info(logConsola,"Cantidad de instrucciones ejecutadas %d", cantidadInstruccionesEjecutadas);

			if(verificaInterrupcion()>0) {
					expropiar();
					break;
				}

			if(!procesoFinalizado && cantidadInstruccionesAEjecutarPorKernel== 0) expropiarPorRR();
		}
	}
}
void ejecutarInstruccion(){

	//char orden;
	//orden = '\0';
	char *instruccion = obtener_instruccion();

	sleep(retardo_entre_instruccion);

	log_warning(logConsolaPantalla,"Evaluando -> %s\n", instruccion );
	analizadorLinea(instruccion , &functions, &kernel_functions);


	/*recv(socketKernel,&orden,sizeof(char),MSG_DONTWAIT); //espero sin bloquearme ordenes del kernel

	if(orden == 'F')  {
		printf("Me llego una interrupciones del recv NO bloqeuante\n");
		interrupcion = FINALIZADO_VOLUNTARIAMENTE;
	}
	*/

	free(instruccion);

	pcb_actual->programCounter = pcb_actual->programCounter + 1;
}

//----------------------------Manejo Instrucciones-------------------------------------
