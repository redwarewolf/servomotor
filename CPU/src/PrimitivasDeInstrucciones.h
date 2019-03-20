#include "CPU.h"
#include "Interrupciones.h"
t_puntero definirVariable(t_nombre_variable variable) {


	int nodos_stack = list_size(pcb_actual->indiceStack);
	int posicion_stack;
	int cantidad_variables;
	int cantidad_argumentos;
	int encontre_valor = 1;
	t_nodoStack *nodo;
	t_posMemoria *posicion_memoria;
	t_variable *var;
	t_posMemoria* nueva_posicion_memoria;
	t_variable *nueva_variable;


	if(nodos_stack == 0){
		t_nodoStack *nodo = malloc(sizeof(t_nodoStack));
		nodo->args = list_create();
		nodo->vars = list_create();
		nodo->retPos = 0;
		t_posMemoria *retorno = malloc(sizeof(t_posMemoria));
		retorno->pagina = 0;
		retorno->offset = 0;
		retorno->size = 0;
		nodo->retVar = retorno;
		list_add(pcb_actual->indiceStack, nodo);
	}

	nodos_stack = list_size(pcb_actual->indiceStack);

	for(posicion_stack = (nodos_stack - 1); posicion_stack >= 0; posicion_stack--){
			nodo = list_get(pcb_actual->indiceStack, posicion_stack);
			cantidad_variables = list_size(nodo->vars);
			cantidad_argumentos = list_size(nodo->args);
			if(cantidad_variables != 0){
				var = list_get(nodo->vars, (cantidad_variables - 1));
				posicion_memoria = var->dirVar;
				posicion_stack = -1;
				encontre_valor = 0;
			} else if(cantidad_argumentos != 0){
				posicion_memoria = list_get(nodo->args, (cantidad_argumentos - 1));
				posicion_stack = -1;
				encontre_valor = 0;
			}
		}

		if((variable >= '0') && (variable <= '9')){
				nueva_posicion_memoria = malloc(sizeof(t_posMemoria));
				nodo = list_get(pcb_actual->indiceStack, (nodos_stack - 1));

				if(encontre_valor == 0){

					if(((posicion_memoria->offset + posicion_memoria->size) + 4) > config_paginaSize){
						nueva_posicion_memoria->pagina = (posicion_memoria->pagina + 1);
						nueva_posicion_memoria->offset = 0;
						nueva_posicion_memoria->size = 4;
							if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
								stackOverflow(pcb_actual);
								return 0;
							} else {
							list_add(nodo->args, nueva_posicion_memoria);
							}
					}

					else {
									nueva_posicion_memoria->pagina = posicion_memoria->pagina;
									nueva_posicion_memoria->offset = (posicion_memoria->offset + posicion_memoria->size);
									nueva_posicion_memoria->size = 4;
									if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
										stackOverflow(pcb_actual);
										return 0;
									} else {
										list_add(nodo->args, nueva_posicion_memoria);
									}
					}

				}else {
							if(config_paginaSize < 4){
								printf("Tamaño de pagina menor a 4 bytes\n");
							} else {

								nueva_posicion_memoria->pagina = (cantidadPaginasTotales(pcb_actual) - config_stackSize);
								nueva_posicion_memoria->offset = 0;
								nueva_posicion_memoria->size = 4;
								if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
									stackOverflow(pcb_actual);
									return 0;
								} else {
									list_add(nodo->args, nueva_posicion_memoria);
								}
							}
						}
		}
				else {
						nueva_posicion_memoria = malloc(sizeof(t_posMemoria));
						nueva_variable = malloc(sizeof(t_variable));
						nodo = list_get(pcb_actual->indiceStack, (nodos_stack - 1));
						if(encontre_valor == 0){
							if(((posicion_memoria->offset + posicion_memoria->size) + 4) > config_paginaSize){
								nueva_posicion_memoria->pagina = (posicion_memoria->pagina + 1);
								nueva_posicion_memoria->offset = 0;
								nueva_posicion_memoria->size = 4;
								nueva_variable->idVar = variable;
								nueva_variable->dirVar = nueva_posicion_memoria;
								if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
									stackOverflow(pcb_actual);
									return 0;
								} else {
									list_add(nodo->vars, nueva_variable);
								}
							}else {

								nueva_posicion_memoria->pagina = posicion_memoria->pagina;
								nueva_posicion_memoria->offset = (posicion_memoria->offset + posicion_memoria->size);
								nueva_posicion_memoria->size = 4;
								nueva_variable->idVar = variable;
								nueva_variable->dirVar = nueva_posicion_memoria;
								if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
									stackOverflow(pcb_actual);
									return 0;
								} else {
									list_add(nodo->vars, nueva_variable);
								}
							}

				}	else {
								if(config_paginaSize < 4){
									printf("Tamaño de pagina menor a 4 bytes\n");
									} else {
										nueva_posicion_memoria->pagina = (cantidadPaginasTotales(pcb_actual) - config_stackSize);//ACA MUESTRA -1 PORQUE HAY UNA PAGINA DE CODIGO Y 2 DE STACK
										nueva_posicion_memoria->offset = 0;
										nueva_posicion_memoria->size = 4;
										nueva_variable->idVar = variable;
										nueva_variable->dirVar = nueva_posicion_memoria;
										if(nueva_posicion_memoria->pagina >= cantidadPaginasTotales(pcb_actual)){
											stackOverflow(pcb_actual);
											return 0;
										} else {
											list_add(nodo->vars, nueva_variable);
										}
									}
								}
					}

int posicion= (nueva_posicion_memoria->pagina * config_paginaSize) + nueva_posicion_memoria->offset;
log_info(logConsolaPantalla,"Definida la variable %c en la pagina %d y offset %d del stack",variable,nueva_posicion_memoria->pagina,nueva_posicion_memoria->offset);
return posicion;
}




t_puntero obtenerPosicionVariable(t_nombre_variable variable) {
	log_info(logConsolaPantalla, "Obteniendo la posicion de la variable: %c\n", variable);

	int nodos_stack = list_size(pcb_actual->indiceStack);//obtengo cantidad de nodos
	int cantidad_variables;
	int i;
	int encontre_valor = 1;
	t_nodoStack *nodoUltimo;
	t_posMemoria *posicion_memoria;
	t_variable *var;
	nodoUltimo = list_get(pcb_actual->indiceStack, (nodos_stack - 1));//obtengo el ultimo nodo de la lista
	if((variable >= '0') && (variable <= '9')){// argumento de una funcion
		int variable_int = variable - '0';//lo pasa a int

		posicion_memoria = list_get(nodoUltimo->args, variable_int);//lo busca en la lista de argumentos
		if(posicion_memoria != NULL){
			encontre_valor = 0;
		}
	} else {//si es una variable propiamente dicha

		cantidad_variables = list_size(nodoUltimo->vars);
		for(i = 0; i < cantidad_variables; i++){
			var = list_get(nodoUltimo->vars, i);
			if(var->idVar == variable){
				posicion_memoria = var->dirVar;
				encontre_valor = 0;
			}
		}
	}
	if(encontre_valor == 1){
		log_info(logConsolaPantalla, "ObtenerPosicionVariable: No se encontro variable o argumento\n");
		direccionInvalida();
		return -1;
	}

	int posicion_serializada = (posicion_memoria->pagina * config_paginaSize) + posicion_memoria->offset;//me devuelve la posicion en memoria
	//free(pcb_actual);}
	log_info(logConsolaPantalla,"La posicion de la variable es %d\n",posicion_serializada);
	return posicion_serializada;
}
void finalizar (){
		char comandoFinalizacion = 'T';

		send(socketKernel,&comandoFinalizacion,sizeof(char),0);
		serializarPcbYEnviar(pcb_actual,socketKernel);

		log_info(logConsolaPantalla, "El proceso ANSISOP de PID %d ha finalizado\n", pcb_actual->pid);

		procesoFinalizado=1;

		free(pcb_actual);
}

t_valor_variable dereferenciar(t_puntero puntero) {

	int num_pagina = puntero / config_paginaSize;
	int offset = puntero - (num_pagina * config_paginaSize);
	char *valor_variable_char;
	char* mensajeRecibido;


		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, sizeof(t_valor_variable))<0){
			direccionInvalida();
			return 0;
		}else{
				valor_variable_char=mensajeRecibido;
		}

	t_valor_variable valor_variable = atoi(valor_variable_char);
	free(valor_variable_char);
	log_info(logConsolaPantalla, "Dereferenciar: Valor Obtenido: %d en la posicion %d\n", valor_variable,puntero);
	return valor_variable;

}

void asignar(t_puntero puntero, t_valor_variable variable) {
	int num_pagina = puntero / config_paginaSize;
	int offset = puntero - (num_pagina * config_paginaSize);
	char *valor_variable = string_itoa(variable);
	int resultado = almacenarDatosEnMemoria(valor_variable,sizeof(t_valor_variable),num_pagina, offset);
	if(resultado==-1){
		log_info(logConsolaPantalla, "No se pudo almacenar el contenido %d en %d por excepcion de memoria", valor_variable,puntero);
		excepcionMemoria();
		return;
	}
	log_info(logConsolaPantalla, "Valor a Asignar: %s en la posicion %d\n", valor_variable,puntero);
	free(valor_variable);
}
void retornar(t_valor_variable retorno){

	t_nodoStack *nodo;
	int cantidad_nodos = list_size(pcb_actual->indiceStack);
	nodo = list_remove(pcb_actual->indiceStack, (cantidad_nodos - 1));
	t_posMemoria *posicion_memoria;
	posicion_memoria = nodo->retVar;
	int num_pagina = posicion_memoria->pagina;
	int offset = posicion_memoria->offset;
	char *valor_variable = string_itoa(retorno);
	almacenarDatosEnMemoria(valor_variable,sizeof(t_valor_variable),num_pagina, offset);
	free(valor_variable);
	pcb_actual->programCounter = nodo->retPos;// Puede ser la dir_retorno + 1
	printf("el PC es %d",pcb_actual->programCounter);
	//Elimino el nodo de la lista
	int cantidad_argumentos;
	int cantidad_variables;
	t_variable *var;
	cantidad_argumentos = list_size(nodo->args);
	while(cantidad_argumentos != 0){
		posicion_memoria = list_remove(nodo->args, (cantidad_argumentos - 1));
		free(posicion_memoria);
		cantidad_argumentos = list_size(nodo->args);
	}
	list_destroy(nodo->args);
	cantidad_variables = list_size(nodo->vars);
	while(cantidad_variables != 0){
		var = list_remove(nodo->vars, (cantidad_variables - 1));
		free(var->dirVar);
		free(var);
		cantidad_variables = list_size(nodo->vars);
	}
	list_destroy(nodo->vars);
	free(nodo->retVar);
	free(nodo);


}


void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

t_nodoStack *nodo = malloc(sizeof(t_nodoStack));
nodo->args = list_create();
nodo->vars = list_create();
nodo->retPos = (pcb_actual->programCounter);//Puede ser programCounter + 1
int num_pagina = donde_retornar / config_paginaSize;
int offset = donde_retornar - (num_pagina * config_paginaSize);
t_posMemoria *retorno = malloc(sizeof(t_posMemoria));
retorno->pagina = num_pagina;
retorno->offset = offset;
retorno->size = 4;
nodo->retVar = retorno;
list_add(pcb_actual->indiceStack, nodo);
char** string_cortado = string_split(etiqueta, "\n");
int program_counter = metadata_buscar_etiqueta(string_cortado[0], pcb_actual->indiceEtiquetas, pcb_actual->indiceEtiquetasSize);
	if(program_counter == -1){
		log_info(logConsolaPantalla,"No se encontro la funcion %s en el indice de etiquetas\n", string_cortado[0]);
		direccionInvalida();
		return;
	} else {
		pcb_actual->programCounter = (program_counter - 1);
		cantidadInstruccionesAEjecutarDelPcbActual = cantidadInstruccionesAEjecutarDelPcbActual+(pcb_actual->cantidadInstrucciones-pcb_actual->programCounter);
	}
int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
free(string_cortado);

}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

}

void irAlLabel(t_nombre_etiqueta etiqueta){


char** string_cortado = string_split(etiqueta, "\n");
int program_counter = metadata_buscar_etiqueta(string_cortado[0], pcb_actual->indiceEtiquetas, pcb_actual->indiceEtiquetasSize);
	if(program_counter == -1){
		log_info(logConsolaPantalla, "No se encontro la etiqueta: %s en el indice de etiquetas", string_cortado[0]);
		direccionInvalida();
		return ;
	} else {
		pcb_actual->programCounter = (program_counter-1);
		log_info(logConsolaPantalla, "Actualizando Program Counter a %d, despues de etiqueta: %s", pcb_actual->programCounter+1,etiqueta);
		cantidadInstruccionesAEjecutarDelPcbActual = cantidadInstruccionesAEjecutarDelPcbActual+(pcb_actual->cantidadInstrucciones-pcb_actual->programCounter);
	}
int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
free(string_cortado);

}


