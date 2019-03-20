#include <commons/log.h>

int config_paginaSize;
int config_stackSize;

typedef struct{
	int pagina;
	int offset;
	int size;
}__attribute__((packed)) t_posMemoria;


typedef struct{
	char idVar;
	t_posMemoria* dirVar;
} t_variable;

typedef struct{
	t_list* args; //Lista de argumentos. Cada posicion representa un argumento en el orden de la lista
	t_list* vars; // Lista de t_variable
	int retPos;
	t_posMemoria* retVar;
} t_nodoStack;


typedef struct {
		int pid;
		t_puntero_instruccion programCounter;
		int cantidadPaginasCodigo;
		int cantidadInstrucciones;
		int** indiceCodigo;
		t_size indiceEtiquetasSize;
		char* indiceEtiquetas;
		t_list* indiceStack;
		int exitCode;
	}t_pcb;


void inicializarLog(char *rutaDeLog);
t_log *logConsola;
t_log *logConsolaPantalla;
int contadorPid;

//---------PCB-----------------//
t_pcb* crearPcb (char* programa, int programSize);
int cantidadPaginasCodigoProceso(int programSize);
int calcularIndiceCodigoSize(int cantidadInstrucciones);
int calcularIndiceEtiquetasSize(int cantidadEtiquetas);
int calcularIndiceStackSize(t_list* indiceStack);
int calcularPcbSerializadoSize(t_pcb* pcb);
void serializarPcbYEnviar(t_pcb* pcb,int socket);
t_pcb* recibirYDeserializarPcb(int socketKernel);
void imprimirPcb(t_pcb* pcb);
int** traduccionIndiceCodigoSerializado(t_size cantidadInstrucciones, t_intructions* instrucciones_serializados);
int** inicializarIndiceCodigo(t_size cantidadInstrucciones);
//---------PCB-----------------//



t_pcb* crearPcb (char* programa, int programSize){
	log_info(logConsolaPantalla,"Creando PCB ---> PID: %d", contadorPid);
	t_pcb* pcb = malloc (sizeof(t_pcb));
	t_metadata_program* metadata = metadata_desde_literal(programa);
	pcb->pid = contadorPid;
	pcb->programCounter = metadata->instruccion_inicio;
	pcb->cantidadPaginasCodigo = cantidadPaginasCodigoProceso(programSize);
	pcb->cantidadInstrucciones = metadata->instrucciones_size;
	pcb->indiceCodigo = traduccionIndiceCodigoSerializado(pcb->cantidadInstrucciones,metadata->instrucciones_serializado);
	pcb->indiceEtiquetasSize = metadata->etiquetas_size;
	pcb->indiceEtiquetas = malloc(sizeof(char)* pcb->indiceEtiquetasSize);
	memcpy(pcb->indiceEtiquetas,metadata->etiquetas,sizeof(char)* pcb->indiceEtiquetasSize);
	pcb->indiceStack = list_create();

	metadata_destruir(metadata);
	return pcb;
}


int** traduccionIndiceCodigoSerializado(t_size cantidadInstrucciones, t_intructions* instrucciones_serializados){
	int **indiceCodigo=inicializarIndiceCodigo(cantidadInstrucciones);
	int i;
		for(i = 0; i < cantidadInstrucciones; i++){
			indiceCodigo[i][0] = (instrucciones_serializados + i)->start;
			indiceCodigo[i][1] = (instrucciones_serializados + i)->offset;
		}
		return indiceCodigo;
}

int** inicializarIndiceCodigo(t_size cantidadInstrucciones){
	int i;
	int** indiceCodigo = malloc(sizeof(int*) * cantidadInstrucciones);
	for(i=0; i < cantidadInstrucciones; i++) indiceCodigo[i] = malloc(sizeof(int) * 2);
		return indiceCodigo;
}

void serializarPcbYEnviar(t_pcb* pcb,int socketCPU){
	log_info(logConsolaPantalla, "Serializando PCB ----- PID:%d",pcb->pid);

	int pcbSerializadoSize = calcularPcbSerializadoSize(pcb);
	void* pcbEnviar= malloc(pcbSerializadoSize);
	void * pcbSerializado = pcbEnviar;

	memcpy(pcbSerializado,&pcb->pid,sizeof(int));
	pcbSerializado += sizeof(int);
	memcpy(pcbSerializado, &pcb->cantidadPaginasCodigo, sizeof(int));
	pcbSerializado += sizeof(int);
	memcpy(pcbSerializado, &pcb->programCounter, sizeof(t_puntero_instruccion));
	pcbSerializado += sizeof(t_puntero_instruccion);
	memcpy(pcbSerializado, &pcb->cantidadInstrucciones,sizeof(int));
	pcbSerializado += sizeof(int);

	//log_info(loggerConPantalla, "Serializando Indice de Codigo");
	int i;
	for(i=0;i<pcb->cantidadInstrucciones;i++){
		memcpy(pcbSerializado,&pcb->indiceCodigo[i][0],sizeof(int));
		pcbSerializado += sizeof(int);
		memcpy(pcbSerializado,&pcb->indiceCodigo[i][1],sizeof(int));
		pcbSerializado += sizeof(int);
	}
	//log_info(loggerConPantalla, "Indice de Codigo serializado");


	memcpy(pcbSerializado, &pcb->indiceEtiquetasSize,sizeof(t_size));
	pcbSerializado += sizeof(t_size);
	memcpy(pcbSerializado, pcb->indiceEtiquetas, pcb->indiceEtiquetasSize*sizeof(char));
	pcbSerializado += pcb->indiceEtiquetasSize*sizeof(char);

	memcpy(pcbSerializado, &pcb->indiceStack->elements_count, sizeof(int));
	pcbSerializado += sizeof(int);


	//log_info(loggerConPantalla, "Serializando Stack");
	int j;
	t_nodoStack* node;
	t_variable* variable;

			for(i = 0; i < pcb->indiceStack->elements_count;i++){
				node = (t_nodoStack*) list_get(pcb->indiceStack, i);
				memcpy(pcbSerializado, &node->args->elements_count, sizeof(int));
				pcbSerializado += sizeof(int);

				for(j = 0; j < node->args->elements_count; j++){
					memcpy(pcbSerializado, list_get(node->args, j), sizeof(t_posMemoria));
					pcbSerializado += sizeof(t_posMemoria);
				}

				memcpy(pcbSerializado, &node->vars->elements_count, sizeof(int));
				pcbSerializado += sizeof(int);

				for(j = 0; j < node->vars->elements_count; j++){
					variable = list_get(node->vars, j);
					memcpy(pcbSerializado, &variable->idVar, sizeof(char));
					pcbSerializado += sizeof(char);
					memcpy(pcbSerializado, variable->dirVar, sizeof(t_posMemoria));
					pcbSerializado += sizeof(t_posMemoria);
				}

				memcpy(pcbSerializado, &node->retPos, sizeof(int));
				pcbSerializado += sizeof(int);

				memcpy(pcbSerializado, node->retVar, sizeof(t_posMemoria));
				pcbSerializado += sizeof(t_posMemoria);
			}
			//log_info(loggerConPantalla, "Stack serializado");

	memcpy(pcbSerializado,&pcb->exitCode,sizeof(int));

	send(socketCPU,&pcbSerializadoSize,sizeof(int),0);
	send(socketCPU,pcbEnviar,pcbSerializadoSize,0);
	log_info(logConsolaPantalla, "Pcb serializado y enviado ----- PID: %d ------ socketCPU: %d-----Tamano: %d ", pcb->pid, socketCPU,pcbSerializadoSize);

	//imprimirPcb(pcb);

	free(pcbEnviar);

}

t_pcb* recibirYDeserializarPcb(int socketCPU){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	log_info(logConsolaPantalla, "Recibiendo PCB serializado---- SOCKET:%d", socketCPU);
	int pcbSerializadoSize;
	recv(socketCPU,&pcbSerializadoSize,sizeof(int),MSG_WAITALL);
	void * pcbADeserializar = malloc(pcbSerializadoSize);
	void* pcbSerializado = pcbADeserializar;
	recv(socketCPU,pcbSerializado,pcbSerializadoSize,MSG_WAITALL);

	memcpy(&pcb->pid,pcbSerializado,sizeof(int));
	pcbSerializado += sizeof(int);

	log_info(logConsolaPantalla, "Deserializando PCB ----- PID:%d",pcb->pid);

	memcpy(&pcb->cantidadPaginasCodigo,pcbSerializado,sizeof(int));
	pcbSerializado += sizeof(int);
	memcpy(&pcb->programCounter, pcbSerializado, sizeof(t_puntero_instruccion));
	pcbSerializado += sizeof(t_puntero_instruccion);
	memcpy(&pcb->cantidadInstrucciones, pcbSerializado, sizeof(int));
	pcbSerializado += sizeof(int);
	pcb->indiceCodigo = inicializarIndiceCodigo(pcb->cantidadInstrucciones);

	//log_info(loggerConPantalla, "Deserializando Indice de Codigo");
		int i;
		for(i = 0 ; i < pcb->cantidadInstrucciones ; i++){
			memcpy(&pcb->indiceCodigo[i][0],pcbSerializado,sizeof(int));
			pcbSerializado+= sizeof(int);
			memcpy(&pcb->indiceCodigo[i][1],pcbSerializado,sizeof(int));
			pcbSerializado+= sizeof(int);
		}
	//log_info(loggerConPantalla, "Indice de Codigo deserializado");

		memcpy(&pcb->indiceEtiquetasSize,pcbSerializado, sizeof(t_size));
		pcbSerializado += sizeof(t_size);
		pcb->indiceEtiquetas = malloc(pcb->indiceEtiquetasSize*sizeof(char));
		memcpy(pcb->indiceEtiquetas, pcbSerializado, pcb->indiceEtiquetasSize*sizeof(char));
		pcbSerializado += pcb->indiceEtiquetasSize*sizeof(char);

	pcb->indiceStack=list_create();

		//log_info(loggerConPantalla, "Deserializando Stack");
			int j;
			t_nodoStack* node;
			t_posMemoria* argumento;
			t_variable* variable;

			int cantidadArgumentos;
			int cantidadVariables;
			int cantidadElementosStack;

			memcpy(&cantidadElementosStack,(int*) pcbSerializado,sizeof(int));
			pcbSerializado += sizeof(int);

			for(i = 0; i < cantidadElementosStack;i++){
				node = malloc(sizeof(t_nodoStack));
				node->args = list_create();
				cantidadArgumentos = *((int*)pcbSerializado);
				pcbSerializado += sizeof(int);

				for(j = 0; j < cantidadArgumentos; j++){
					argumento = malloc(sizeof(t_posMemoria));
					*argumento = *((t_posMemoria*)pcbSerializado);
					list_add(node->args,argumento);
					pcbSerializado += sizeof(t_posMemoria);
				}

				cantidadVariables = *((int*)pcbSerializado);
				pcbSerializado += sizeof(int);

				node->vars = list_create();

				for(j = 0; j < cantidadVariables; j++){
					variable = malloc(sizeof(t_variable));
					variable->idVar = *((char*) pcbSerializado);
					pcbSerializado += sizeof(char);
					variable->dirVar = malloc(sizeof(t_posMemoria));
					*variable->dirVar= *((t_posMemoria*) pcbSerializado);
					pcbSerializado += sizeof(t_posMemoria);
					list_add(node->vars,(void*) variable);
				}

				node->retPos = *((int*)pcbSerializado);
				pcbSerializado += sizeof(int);
				node->retVar=malloc(sizeof(t_posMemoria));
				*node->retVar = *((t_posMemoria*)pcbSerializado);
				pcbSerializado += sizeof(t_posMemoria);
				list_add(pcb->indiceStack,(void*) node);

		}
			//log_info(loggerConPantalla, "Stack deserializado");

			memcpy(&pcb->exitCode,pcbSerializado,sizeof(int));
			log_info(logConsolaPantalla,"Pcb deserializado------PID: %d -----SocketCPU: %d -----Tamanio: %d",pcb->pid,socketCPU,pcbSerializadoSize);

			//imprimirPcb(pcb);

		free(pcbADeserializar);
	return pcb;
}


void imprimirPcb(t_pcb* pcb){
	printf("Pid: %d\n", pcb->pid);
		printf("Program Counter: %d\n", pcb->programCounter);
		printf("Cantidad de Paginas: %d\n", pcb->cantidadPaginasCodigo);
		printf("Cantidad de Instrucciones: %d\n", pcb->cantidadInstrucciones);

		printf("\n-------Indice de Codigo-------\n");
		int i;
		for(i = 0; i < pcb->cantidadInstrucciones; i++){
			printf("Instruccion %d:  \t%d %d\n", i, pcb->indiceCodigo[i][0], pcb->indiceCodigo[i][1]);
		}

		printf("\n-------Indice de Etiquetas-------\n");
		printf("Tamano: %d \n",pcb->indiceEtiquetasSize);
		printf("%s\n", pcb->indiceEtiquetas);

		printf("\n-------Indice de Stack-------\n");
		t_nodoStack elemento;
		t_posMemoria* argumento;
		t_variable* variable;
		printf("Cantidad Elementos: %d\n", pcb->indiceStack->elements_count);
		int f;
		for(i = 0; i < pcb->indiceStack->elements_count; i++){
			printf("Elemento %d:\n", i);
			elemento = *((t_nodoStack*)list_get(pcb->indiceStack, i));
			printf("\tCantidad de Argumentos: %d\n", elemento.args->elements_count);
			for(f = 0; f < elemento.args->elements_count; f++){
				printf("\t\tArgumento %d: ", f);
				argumento = list_get(elemento.args, f);
				printf("Pagina: %d, Offset: %d, Size: %d\n", argumento->pagina, argumento->offset, argumento->size);
			}
			printf("\tCantidad de Variables: %d\n", elemento.vars->elements_count);
			for(f = 0; f < elemento.vars->elements_count; f++){
				printf("\t\tVariable %d: ", f);
				variable = list_get(elemento.vars, f);
				printf("Identificador: %c, Pagina: %d, Offset: %d, Size: %d\n", variable->idVar, variable->dirVar->pagina, variable->dirVar->offset, variable->dirVar->size);
			}
			printf("\tDireccion de Retorno: %d\n", elemento.retPos);
			printf("\tPosicion de Retorno:\n");
			printf("\t\tPagina: %d, Offset: %d, Size: %d\n", elemento.retVar->pagina, elemento.retVar->offset, elemento.retVar->size);
		}
}

int calcularPcbSerializadoSize(t_pcb* pcb){
	return sizeof(int)*5+ sizeof(t_puntero_instruccion) + sizeof(t_size)+ calcularIndiceCodigoSize(pcb->cantidadInstrucciones) + calcularIndiceEtiquetasSize(pcb->indiceEtiquetasSize) + calcularIndiceStackSize(pcb->indiceStack);
}
int calcularIndiceStackSize(t_list* indiceStack){
	int i;
	int stackSize=0;
	t_nodoStack* node;
	for(i=0;i<indiceStack->elements_count;i++){
			node = list_get(indiceStack,i);
		stackSize+= sizeof(int)*2 + node->args->elements_count * sizeof(t_posMemoria) + node->vars->elements_count * (sizeof(char)+sizeof(t_posMemoria)) + sizeof(int) +sizeof(t_posMemoria);
	}
	return stackSize;
}

int calcularIndiceCodigoSize(int cantidadInstrucciones){
	return cantidadInstrucciones * sizeof(int) * 2;
}

int calcularIndiceEtiquetasSize(int indiceEtiquetasSize){
	return indiceEtiquetasSize*sizeof(char);
}

void recibirTamanioPagina(int socketKernel){
	char comandoGetPaginaSize= 'P';
	send(socketKernel,&comandoGetPaginaSize,sizeof(char),0);
	recv(socketKernel,&config_paginaSize,sizeof(int),0);
	recv(socketKernel,&config_stackSize,sizeof(int),0);
	log_info(logConsolaPantalla, "El tamanio de la pagina es %d y el stack tiene %d paginas\n",config_paginaSize,config_stackSize);
}

int cantidadPaginasCodigoProceso(int programSize){
	log_info(logConsolaPantalla, "Calculando paginas de codigo requeridas");
	int mod = programSize % config_paginaSize;
	log_info(logConsolaPantalla, "Pagina de codigo requeridas: %d",mod);
	return mod==0? (programSize / config_paginaSize):(programSize / config_paginaSize)+ 1;

}


