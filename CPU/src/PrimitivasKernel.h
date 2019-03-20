//SEMAFOROS ANSISOP
void wait(t_nombre_semaforo identificador_semaforo){


	char interruptHandler = 'X';
	char comandoWait = 'W';

	int pid = pcb_actual->pid;
	char** string_cortado = string_split(identificador_semaforo, "\n");
	char* identificadorSemAEnviar = string_new();
	int bloquearScriptONo;

	string_append(&identificadorSemAEnviar, string_cortado[0]);
	int tamanio = sizeof(char)*strlen(identificadorSemAEnviar);
	log_info(logConsolaPantalla, "Semaforo a bajar: %s", string_cortado[0]);

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoWait,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,identificadorSemAEnviar,tamanio,0);
	log_info(logConsola, "Enviando datos a la capa memoria del kernel para saber si el script debe bloquearse o no");
	recv(socketKernel,&bloquearScriptONo,sizeof(int),MSG_WAITALL);


	if(bloquearScriptONo < 0){
	//	cpuBloqueadaPorSemANSISOP = 0;

		log_info(logConsolaPantalla, "Script ANSISOP pid: %d bloqueado por semaforo: %s", pcb_actual->pid, string_cortado[0]);

		interrupcion = SEM_WAIT;

	}else log_info(logConsolaPantalla, "Script ANSISOP pid: %d sigue su ejecucion normal", pcb_actual->pid);


	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return;

}
void signal_Ansisop(t_nombre_semaforo identificador_semaforo){
	char interruptHandler = 'X';
	char comandoSignal = 'S';
	int pid = pcb_actual->pid;
	int i = 0;

	if(!(i > 0)){

	char** string_cortado = string_split(identificador_semaforo, "\n");
	char* identificadorSemAEnviar = string_new();
	string_append(&identificadorSemAEnviar, string_cortado[0]);
	int tamanio = sizeof(char)*strlen(identificadorSemAEnviar);
	log_info(logConsolaPantalla, "Semaforo a subir: %s", string_cortado[0]);

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoSignal,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,identificadorSemAEnviar,tamanio,0);

	log_info(logConsola, "Enviando datos a la capa memoria del kernel para subir el semaforo %s",string_cortado[0]);

	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	}
}
//SEMAFOROS ANSISOP

//HEAP
t_puntero reservar (t_valor_variable espacio){
	int pagina,offset;
	int resultadoEjecucion;
	char comandoInterruptHandler = 'X';
	char comandoReservarMemoria = 'R';
	int pid = pcb_actual->pid;
	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoReservarMemoria,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&espacio,sizeof(int),0);
	log_info(logConsolaPantalla, "Enviando datos a la capa memoria de proceso pid:%d para reservar memoria dinamica de %d bytes",pid,espacio);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion < 0) {
		log_info(logConsolaPantalla, "No se pudo reservar memoria, expropiando proceso pid:%d",pid);
		interrupcion = RES_EJEC_NEGATIVO;
		return 0;
	}
	recv(socketKernel,&pagina,sizeof(int),0);
	recv(socketKernel,&offset,sizeof(int),0);

	t_puntero puntero = pagina * config_paginaSize + offset;
	log_info(logConsolaPantalla, "La direccion logica es: %d",puntero);

	return puntero;
}
void liberar (t_puntero puntero){

	int num_pagina= puntero / config_paginaSize;
	int offset = puntero - (num_pagina * config_paginaSize);


	int pid = pcb_actual->pid;
	char comandoInterruptHandler = 'X';
	char comandoLiberarMemoria = 'L';
	int resultadoEjecucion;
	int tamanio = sizeof(t_puntero);

	send(socketKernel,&comandoInterruptHandler,sizeof(char),0);
	send(socketKernel,&comandoLiberarMemoria,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&num_pagina,sizeof(int),0);
	send(socketKernel,&offset,tamanio,0);
	log_info(logConsolaPantalla, "Enviando datos a la capa memoria del kernel para liberar de la pagina:%d offset:%d del proceso pid:%d",num_pagina,offset,pid);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion==1)
		log_info(logConsolaPantalla,"Se ha liberado correctamente el heap previamente reservado apuntando a %d",puntero);
	else{
		interrupcion = RES_EJEC_NEGATIVO;
		log_info(logConsolaPantalla,"No se ha podido liberar el heap apuntada por",puntero);
	}
}
//HEAP






t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	char** string_cortado = string_split(variable, "\n");
	char* variable_string = string_new();
	char interruptHandler = 'X';
	char comandoObtenerCompartida = 'O';
	int pid = pcb_actual->pid;
	string_append(&variable_string, "!");
	string_append(&variable_string, string_cortado[0]);
	int tamanio = sizeof(int)*strlen(variable_string);

	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoObtenerCompartida,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);


	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,variable_string,tamanio,0);
	log_info(logConsolaPantalla, "Enviando datos a la capa memoria del kernel para obtener la variable %s del proceso pid:%d",variable,pid);
	free(variable_string);

	int valor_variable_int;
	recv(socketKernel,&valor_variable_int,sizeof(int),0);

	log_info(logConsolaPantalla, "Valor de la variable compartida: %d", valor_variable_int);
	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return valor_variable_int;
}
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	char** string_cortado = string_split(variable, "\n");
	char* variable_string = string_new();
	char interruptHandler = 'X';
	char comandoAsignarCompartida = 'G';
	int pid = pcb_actual->pid;



	string_append(&variable_string, "!");
	string_append(&variable_string, string_cortado[0]);
	int tamanio = sizeof(int)*strlen(variable_string);

	log_info(logConsolaPantalla, "Enviando datos a la capa memoria para asignar el valor %d: de id: %s del proceso pid:%d", valor,variable,pid);
	send(socketKernel,&interruptHandler,sizeof(char),0);
	send(socketKernel,&comandoAsignarCompartida,sizeof(char),0);
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0);
	send(socketKernel,variable_string,tamanio,0);
	send(socketKernel,&valor,sizeof(int),0);
	free(variable_string);

	int i = 0;
	while(string_cortado[i] != NULL){
		free(string_cortado[i]);
		i++;
	}
	free(string_cortado);
	return valor;
}
//SEMAFOROS ANSISOP

