#include "Interrupciones.h"
//FS
t_descriptor_archivo abrir_archivo(t_direccion_archivo direccion, t_banderas flags){

	t_descriptor_archivo descriptorArchivoAbierto;
	int descriptor;
	char comandoCapaFS = 'F';
	char comandoAbrirArchivo = 'A';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoAbrirArchivo,sizeof(char),0);

	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);

	int tamanoDireccion=sizeof(char)*strlen(direccion);
	send(socketKernel,&tamanoDireccion,sizeof(int),0);
	send(socketKernel,direccion,tamanoDireccion,0);

	//enviar los flags al kernel
	char* flagsMapeados;
	flagsMapeados = devolverStringFlags(flags);

	int tamanoFlags=sizeof(char)*strlen(flagsMapeados);
	send(socketKernel,&tamanoFlags,sizeof(int),0);
	send(socketKernel,flagsMapeados,tamanoFlags,0);


	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);

	if(resultadoEjecucion < 0){
		log_error(logConsolaPantalla,"Error del proceso de PID %d al abrir un archivo en modo %s\n",pid,flagsMapeados);
		interrupcion = RES_EJEC_NEGATIVO;
		return 0;
	}
	recv(socketKernel,&descriptor,sizeof(int),0);
	log_info(logConsolaPantalla,"El proceso de PID %d ha abierto un archivo de descriptor %d en modo %s",pid,descriptor,flagsMapeados);
	descriptorArchivoAbierto = (t_descriptor_archivo) descriptor;
	return descriptorArchivoAbierto;
}









void borrar_archivo (t_descriptor_archivo descriptor_archivo){

	char comandoCapaFS = 'F';
	char comandoBorrarArchivo = 'B';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoBorrarArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion>0)log_info(logConsolaPantalla,"El proceso de PID %d ha borrado un archivo de descriptor %d\n",pid,descriptor_archivo);
	else {
		log_error(logConsolaPantalla,"Error del proceso de PID %d al borrar el archivo de descriptor %d",pid,descriptor_archivo);
		interrupcion = RES_EJEC_NEGATIVO;
	}
}









void cerrar_archivo(t_descriptor_archivo descriptor_archivo){

	char comandoCapaFS = 'F';
	char comandoCerrarArchivo = 'P';
	int resultadoEjecucion=0;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoCerrarArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);


	if(resultadoEjecucion>0)log_info(logConsolaPantalla,"El proceso de PID %d ha cerrado un archivo de descriptor %d\n",pid,descriptor_archivo);
	else {
		log_error(logConsolaPantalla,"Error del proceso de PID %d ha cerrado el archivo de descriptor %d",pid,descriptor_archivo);
		interrupcion = RES_EJEC_NEGATIVO;
	}

}








void moverCursor_archivo (t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){

	char comandoCapaFS = 'F';
	char comandoMoverCursorArchivo = 'M';
	int resultadoEjecucion ;
	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoMoverCursorArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	send(socketKernel,&posicion,sizeof(int),0);
	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	if(resultadoEjecucion>0)log_info(logConsolaPantalla,"El proceso de PID %d ha movido el cursor de un archivo de descriptor %d en la posicion %d\n",pid,descriptor_archivo,posicion);
	else {
		log_error(logConsolaPantalla,"Error del proceso de PID %d al mover el cursor de un archivo de descriptor %d en la posicion %d",pid,descriptor_archivo,posicion);
		interrupcion = RES_EJEC_NEGATIVO;
	}
}









void leer_archivo(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){

	char comandoCapaFS = 'F';
	char comandoLeerArchivo = 'O';
	int resultadoEjecucion ;
	void* infoLeida = malloc(tamanio);

	int num_pagina= informacion / config_paginaSize;
	int offset = informacion - (num_pagina * config_paginaSize);

	send(socketKernel,&comandoCapaFS,sizeof(char),0);
	send(socketKernel,&comandoLeerArchivo,sizeof(char),0);
	int pid= pcb_actual->pid;
	send(socketKernel,&pid,sizeof(int),0);
	send(socketKernel,&descriptor_archivo,sizeof(int),0);
	send(socketKernel,&tamanio,sizeof(int),0); //tamanio de la instruccion en bytes que quiero leer


	recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
	char* mensajeRecibido;
	char* valor;
	if(resultadoEjecucion>0){
		recv(socketKernel,infoLeida,tamanio,0);

		almacenarDatosEnMemoria(infoLeida,tamanio,num_pagina,offset);
		if ( conseguirDatosMemoria(&mensajeRecibido, num_pagina,offset, tamanio)<0){
							direccionInvalida();
						}else{
								valor=mensajeRecibido;
						}
		int posicion= num_pagina*config_paginaSize+offset;
		leerFS=malloc(sizeof(t_leerFS));
		leerFS->direccionLogicaHeap=posicion;
		leerFS->stringLeidoDeFs= valor;
		leerFS->tamanio=tamanio;
		log_info(logConsolaPantalla,"\nSe ha guardado correctamente  '%s' en la posicion %d \n",valor,posicion);

	}else{
		log_error(logConsolaPantalla,"Error del proceso de PID %d al leer informacion de un archivo de descriptor %d \n",pid,descriptor_archivo);
		interrupcion = RES_EJEC_NEGATIVO;
	}
}








void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	if(descriptor_archivo==DESCRIPTOR_SALIDA){
		if(tamanio==1){
			informacion= (void *)(long)leerFS->stringLeidoDeFs;
			tamanio= leerFS->tamanio;
			free(leerFS);
		}

		char comandoImprimir = 'X';
		char comandoImprimirPorConsola = 'P';
		send(socketKernel,&comandoImprimir,sizeof(char),0);
		send(socketKernel,&comandoImprimirPorConsola,sizeof(char),0);

		send(socketKernel,&tamanio,sizeof(int),0);
		send(socketKernel,informacion,tamanio,0);
		send(socketKernel,&pcb_actual->pid,sizeof(int),0);
	}else {
		char comandoCapaFS = 'F';
			char comandoEscribirArchivo = 'G';
			int resultadoEjecucion ;
			send(socketKernel,&comandoCapaFS,sizeof(char),0);
			send(socketKernel,&comandoEscribirArchivo,sizeof(char),0);
			int pid= pcb_actual->pid;
			send(socketKernel,&pid,sizeof(int),0);
			send(socketKernel,&descriptor_archivo,sizeof(int),0);
			send(socketKernel,&tamanio,sizeof(int),0);

			/*printf("Informacion void*:%s\n",informacion);
			printf("Informacion char*:%s\n",(char*)informacion);
			printf("Informacion int:%d\n",*(int*)informacion);
			printf("Informacion atoi:%d\n",atoi((char*)informacion));
*/

			send(socketKernel,informacion,tamanio,0); //puntero que apunta a la direccion donde quiero obtener la informacion


			recv(socketKernel,&resultadoEjecucion,sizeof(int),0);
			if(resultadoEjecucion < 0) {
				log_error(logConsolaPantalla,"Error del proceso de PID %d al escribir un archivo de descriptor %d \n",pid,descriptor_archivo);
				interrupcion = RES_EJEC_NEGATIVO;
					return;
				}
			log_info(logConsolaPantalla,"La informacion ha sido escrita con exito en el archivo de descriptor %d PID %d",descriptor_archivo,pid);
	}

}
//FS

