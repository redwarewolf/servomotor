




//----------------------------Manejo Memoria-------------------------------------
int conseguirDatosMemoria (char** instruccion, int paginaSolicitada,int offset,int size){
	int resultadoEjecucion;
	char comandoSolicitar = 'S';//comando que le solicito a la memoria para que ande el main_solicitarBytesPagina

	send(socketMemoria,&comandoSolicitar,sizeof(char),0);
	send(socketMemoria,&pcb_actual->pid,sizeof(int),0);
	send(socketMemoria,&paginaSolicitada,sizeof(int),0);
	send(socketMemoria,&offset,sizeof(int),0);
	send(socketMemoria,&size,sizeof(int),0);
	*instruccion=malloc((size+1)*sizeof(char));
	recv(socketMemoria,*instruccion,size,0);
	strcpy(*instruccion+size,"\0");
	recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);
	return resultadoEjecucion;
}

int almacenarDatosEnMemoria(char* buffer, int size,int paginaAGuardar,int offset){
		int resultadoEjecucion=1;
		int comandoAlmacenar = 'C';
		send(socketMemoria,&comandoAlmacenar,sizeof(char),0);
		send(socketMemoria,&pcb_actual->pid,sizeof(int),0);
		send(socketMemoria,&paginaAGuardar,sizeof(int),0);
		send(socketMemoria,&offset,sizeof(int),0);
		send(socketMemoria,&size,sizeof(int),0);
		send(socketMemoria,buffer,size,0);
		recv(socketMemoria,&resultadoEjecucion,sizeof(int),0);

		return resultadoEjecucion;

}
//----------------------------Manejo Memoria-------------------------------------
