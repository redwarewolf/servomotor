#include "sockets.h"
#include <commons/log.h>
#include <commons/collections/list.h>
#include "logger.h"
 #include <fcntl.h>

int socketKernel;

void validarArchivoFunction(char* path);
void crearArchivoFunction(char* path);
void borrarArchivoFunction(char* path);
void obtenerDatosArchivoFunction(char* path);
void guardarDatosArchivoFunction(char* path);

void guardarDatosArchivoFunction2(char* path);
void* leerParaGuardar(char* nombreArchivoRecibido,int size,int cursor);


void printBitmap(){

	int j;
	for(j=0;j<config_cantidadBloques;j++){
        bool a = bitarray_test_bit(bitarray,j);
        printf("%i", a);
	}
	//bitarray_clean_bit(bitarray,3);
	printf("\n");
}

void actualizarMetadataArchivo(char* path,int size,t_list* nuevosBloques);

void validarArchivoFunction(char* path){
	int validado;

	char *rutaAbsoluta = string_new();
	string_append(&rutaAbsoluta, puntoMontaje);
	string_append(&rutaAbsoluta, "Archivos");
	string_append(&rutaAbsoluta, path);
	log_info(logConsolaPantalla,"Validando existencia de archivo--->Direccion:%s",rutaAbsoluta);

	if( access(rutaAbsoluta , F_OK ) != -1 ) {
	    // file exists
		log_info(logConsolaPantalla,"Archivo %s existente en FS\n",path);
		validado=1;
		send(socketKernel,&validado,sizeof(int),0);
	} else {
	    // file doesn't exist
	  log_warning(logConsolaPantalla,"Archivo :%s no existente\n",path);
	   validado=0;
	   send(socketKernel,&validado,sizeof(int),0);
	}

}

bool esArchivo(char* archivo){
	int i=0;

	while(archivo[i]!='\0'){
		if(archivo[i] == '.') return true;
		i++;
	}
	return false;
}


void crearArchivoFunction(char* path){ // /Carpeta1/Carpeta2/archivo.bin

	int validado;

	char* montajeCarpeta = string_new();

	char *rutaAbsoluta = string_new();
	string_append(&rutaAbsoluta, puntoMontaje);
	string_append(&rutaAbsoluta, "Archivos");
	string_append(&rutaAbsoluta, path);

	log_info(logConsolaPantalla,"Creando archivo--->Direccion:%s",rutaAbsoluta);

	char* carpetaSiguiente = strtok(path,"/");


	if(!esArchivo(carpetaSiguiente)){
		printf("Verificando directorio:%s\n",carpetaSiguiente);

		string_append(&montajeCarpeta,puntoMontaje);
		string_append(&montajeCarpeta,"Archivos/");
		string_append(&montajeCarpeta,carpetaSiguiente);
		mkdir(montajeCarpeta,0777);
		string_append(&montajeCarpeta,"/");

		while((carpetaSiguiente = strtok(NULL,"/")) != NULL){

				if(!esArchivo(carpetaSiguiente)) {
				string_append(&montajeCarpeta,carpetaSiguiente);
				printf("Verificando directorio:%s\n",carpetaSiguiente);
				mkdir(montajeCarpeta, 0777);
				string_append(&montajeCarpeta,"/");
				}
				else break;
		}
	}



	//Recorro bitmap y veo si hay algun bloque para asignarle
	//por default se le asigna un bloque al archivo recien creado
	int j;
	int encontroUnBloque=0;
	int bloqueEncontrado=0;
	for(j=0;j<config_cantidadBloques;j++){

        bool bit = bitarray_test_bit(bitarray,j);
        if(bit==0){
        	encontroUnBloque=1;
        	bloqueEncontrado=j;
        	break;
        }
	}

	if(encontroUnBloque==1){
		FILE* fp = fopen(rutaAbsoluta, "wb");
		//asignar bloque en el metadata del archivo(y marcarlo como ocupado en el bitmap)
		//escribir el metadata ese del archivo (TAMANO y BLOQUES)

		log_info(logConsolaPantalla,"Asignando bloque al nuevo archivo--->Bloque: %d.bin",bloqueEncontrado);
		bitarray_set_bit(bitarray,bloqueEncontrado);

		char *dataAPonerEnFile = string_new();
		string_append(&dataAPonerEnFile, "TAMANIO=");
		string_append(&dataAPonerEnFile, "0");
		string_append(&dataAPonerEnFile,"\n");
		string_append(&dataAPonerEnFile, "BLOQUES=[");
		char* numerito=string_itoa(bloqueEncontrado);
		string_append(&dataAPonerEnFile,numerito);
		string_append(&dataAPonerEnFile,"]");

		log_info(logConsolaPantalla,"Guardando info de metadata en %s\n",rutaAbsoluta);
		adx_store_data(rutaAbsoluta,dataAPonerEnFile);

		validado=1;
		send(socketKernel,&validado,sizeof(int),0);
		fclose(fp);
	}else{
		validado=-1;
		log_error(logConsolaPantalla,"No existen bloques disponibles para crear el archivo\n");
		send(socketKernel,&validado,sizeof(int),0);
	}

}


void borrarArchivoFunction(char* path){

	int validado;

	char *rutaAbsoluta = string_new();
	string_append(&rutaAbsoluta, puntoMontaje);
	string_append(&rutaAbsoluta, "Archivos");
	string_append(&rutaAbsoluta, path);
	log_info(logConsolaPantalla,"Borrando archivo:%s",rutaAbsoluta);

	if( access(rutaAbsoluta, F_OK ) != -1 ) {
	   //poner en un array los bloques de ese archivo para luego liberarlos

		char** arrayBloques=obtArrayDeBloquesDeArchivo(rutaAbsoluta);

	   if(remove(rutaAbsoluta) == 0){
		   //marcar los bloques como libres dentro del bitmap (recorriendo con un for el array que cree arriba)
		   int d=0;
		   log_info(logConsolaPantalla,"Liberando bloques");
		   while(!(arrayBloques[d] == NULL)){
			   log_info(logConsolaPantalla,"Liberando bloque: %s.bin",arrayBloques[d]);
			  bitarray_clean_bit(bitarray,atoi(arrayBloques[d]));
		      d++;
		   }
		   validado=1;
		   send(socketKernel,&validado,sizeof(char),0);
		   //send diciendo que se elimino correctamente el archivo
		   log_info(logConsolaPantalla,"El archivo ha sido borrar--->Archivo:%s\n",rutaAbsoluta);
	   }
	   else
	   {
		   log_error(logConsolaPantalla,"Excepecion de filesystem al borrar archivo--->Archivo:%s\n",rutaAbsoluta);
		   validado=-1;
		   send(socketKernel,&validado,sizeof(char),0);
	      //send que no se pudo eliminar el archivo
	   }
	}else {
		log_error(logConsolaPantalla,"El archivo no se puede borrar porque no existe--->Archivo:%s\n",rutaAbsoluta);
		validado=-1;
		send(socketKernel,&validado,sizeof(char),0);
		//send diciendo que hubo un error y no se pudo eliminar el archivo
	}
}


void obtenerDatosArchivoFunction(char* path){//ver tema puntero , si lo tenog que recibir o que onda

	int validado;
	int cursor;
	int size;

	recv(socketKernel,&cursor,sizeof(int),0);
	recv(socketKernel,&size,sizeof(int),0);

	log_info(logConsolaPantalla,"Obteniendo datos--->Archivo:%s--->Cursor:%d--->Size:%d",path,cursor,size);

	char *rutaAbsoluta = string_new();
	string_append(&rutaAbsoluta, puntoMontaje);
	string_append(&rutaAbsoluta, "Archivos");
	string_append(&rutaAbsoluta, path);

	log_info(logConsolaPantalla,"Direccion--->:%s",rutaAbsoluta);

	if( access(rutaAbsoluta, F_OK ) != -1 ) {

		char** arrayBloques=obtArrayDeBloquesDeArchivo(rutaAbsoluta);

		   int cantidadBloquesNecesito;
		   if(((size+cursor)%config_tamanioBloques)==0) cantidadBloquesNecesito = ((size+cursor)/config_tamanioBloques);
		   else cantidadBloquesNecesito = ((size+cursor)/config_tamanioBloques)+1;

		   log_info(logConsolaPantalla,"Cantidad de bloques a leer:%d",cantidadBloquesNecesito);

		   int d=0;
		   int tamanioBloqueAcumulado = config_tamanioBloques;
		   while(!(cursor<tamanioBloqueAcumulado)){ //Para saber cual es el primer bloque a leer
			   d++;
			   tamanioBloqueAcumulado += config_tamanioBloques;
		   }
		   /*
		    * Cursor = 160
		    * Bloque = 64
		    * Acumulado = 64
		    *
		    * 1. Cursor<Acumulado d++ -> Acumulado +=Bloque
		    * Acumulado=128
		    *
		    * 2. Cursor < Acumulado d++ -> Acumulado +=Bloque
		    * Acumulado=192
		    *
		    * 3. Cursor ya es menos que Acumulador.
		    */

		   FILE *bloque;

		   int sizeRestante=size;


		   void* informacion = malloc(size);
		   int desplazamiento=0;


		   int sizeDentroBloque=0;
		   int cantidadBloquesLeidos=0;

		   while(cantidadBloquesLeidos < cantidadBloquesNecesito){

			   log_info(logConsolaPantalla,"Leyendo data del bloque:%s",arrayBloques[d]);

			   if(cantidadBloquesLeidos==0){

				   if(size>config_tamanioBloques-cursor) sizeDentroBloque=config_tamanioBloques-cursor;//Leo la porcion restante del bloque
				   else sizeDentroBloque = size; //Leo lo suficiente
				   printf("El tamano a leer del bloque es :%d\n",sizeDentroBloque);

				   char *nombreBloque = string_new();
				   string_append(&nombreBloque, puntoMontaje);
				   string_append(&nombreBloque, "Bloques/");
				   string_append(&nombreBloque, arrayBloques[d]);
				   string_append(&nombreBloque, ".bin");

				   bloque=fopen(nombreBloque, "rb");

				   memcpy(informacion + desplazamiento,obtenerBytesDeUnArchivo(bloque,cursor,sizeDentroBloque),sizeDentroBloque);
				   //data=(char*)obtenerBytesDeUnArchivo(bloque,cursor,sizeDentroBloque); //En el primero bloque, arranca del cursor
				  // string_append(&infoTraidaDeLosArchivos,data);
				   sizeRestante -= sizeDentroBloque;
				   desplazamiento += sizeDentroBloque;
			   }
			   else{
				   if(sizeRestante < config_tamanioBloques) sizeDentroBloque = sizeRestante; //Es el ultimo bloque a leer
				   else sizeDentroBloque = config_tamanioBloques; //Leo to-do el bloque
				   printf("El tamano a leer del bloque es :%d\n",sizeDentroBloque);
				   char *nombreBloque = string_new();
				   string_append(&nombreBloque, puntoMontaje);
				   string_append(&nombreBloque, "Bloques/");
				   string_append(&nombreBloque, arrayBloques[d]);
				   string_append(&nombreBloque, ".bin");

				   bloque=fopen(nombreBloque, "rb");

				   memcpy(informacion + desplazamiento, obtenerBytesDeUnArchivo(bloque,0,sizeDentroBloque),sizeDentroBloque);

				  // data=(char*)obtenerBytesDeUnArchivo(bloque,0,sizeDentroBloque); //Siempre arranca del principio
				  // string_append(&infoTraidaDeLosArchivos,data);
				   sizeRestante -= sizeDentroBloque;
				   desplazamiento += sizeDentroBloque;
			   }
			   d++; //Avanzo de bloque
			   cantidadBloquesLeidos++;
		   }



		log_info(logConsolaPantalla,"Informacion leida:%s\n",informacion); //Esta trayendo un caracter de mas, si llega al final del bloque
		//si todod ok
		validado=1;
		send(socketKernel,&validado,sizeof(int),0);
		send(socketKernel,informacion,size,0);
	} else {
		log_error(logConsolaPantalla,"No se puede leer porque el archivo no existe\n");
		validado=-1;
		send(socketKernel,&validado,sizeof(int),0); //El archivo no existe
	}


}

void guardarDatosArchivoFunction(char* path){//ver tema puntero, si lo tengo que recibir o que onda
	FILE* bloque;
	int validado;
	int cursor;
	int size;
	int cuantosBloquesMasNecesito;
	t_list* nuevosBloques = list_create();

	recv(socketKernel,&cursor,sizeof(int),0);
	recv(socketKernel,&size,sizeof(int),0);
	void* buffer = malloc(size);
	recv(socketKernel,buffer,size,0);

	log_info(logConsolaPantalla,"Guardando datos--->Archivo:%s--->Informacion:%s",path,(char*)buffer);

	char *rutaAbsoluta = string_new();
	string_append(&rutaAbsoluta, puntoMontaje);
	string_append(&rutaAbsoluta, "Archivos");
	string_append(&rutaAbsoluta, path);

	log_info(logConsolaPantalla,"Direccion--->:%s",rutaAbsoluta);


	if( access( rutaAbsoluta, F_OK ) != -1 ) {

		char** arrayBloques=obtArrayDeBloquesDeArchivo(rutaAbsoluta);

		int bloquesExistentes = 0;
		int i=0;
		while(arrayBloques[i]!=NULL){
			bloquesExistentes ++;
			i++;
		}

		printf("Cantidad de bloques del archivo:%d\n",bloquesExistentes);

		 int indiceBloque=0;
		 int tamanioBloqueAcumulado = config_tamanioBloques;
	   while(!(cursor<tamanioBloqueAcumulado)){ //Para saber cual es el primer bloque a escribir
					   indiceBloque++;
					   tamanioBloqueAcumulado += config_tamanioBloques;
				   }

	   bool elArchivoTieneBloquesSuficientes = bloquesExistentes*config_tamanioBloques > size+cursor;
	   printf("%s\n",elArchivoTieneBloquesSuficientes? "True":"False");

	   printf("Tamano bloque acumulador :%d\n",tamanioBloqueAcumulado);


		char *direccionBloque = string_new();
		string_append(&direccionBloque, puntoMontaje);
		string_append(&direccionBloque, "Bloques/");
		string_append(&direccionBloque, arrayBloques[indiceBloque]);
		string_append(&direccionBloque, ".bin");

		printf("Tamano del archivo : %d\n",atoi(obtTamanioArchivo(rutaAbsoluta)));

		if(size + cursor < tamanioBloqueAcumulado){ //Me alcanza con el bloque donde estoy parado?
			log_info(logConsolaPantalla,"Escribiendo en bloque:%s\n",direccionBloque);
			bloque = fopen(direccionBloque,"r+b");
			fseek(bloque,0,SEEK_SET);

			fseek(bloque,config_tamanioBloques-(tamanioBloqueAcumulado - cursor),SEEK_SET);

			fwrite(buffer,1,size,bloque);
			log_info(logConsolaPantalla,"Datos guardados--->Archivo:%s--->Informacion:%s",path,(char*)buffer);
			fclose(bloque);
		}

		else{ //Necesito mas bloques. Nuevos o existentes

			int bytesNecesarios = size+cursor-tamanioBloqueAcumulado;


		   if((bytesNecesarios%config_tamanioBloques) == 0) cuantosBloquesMasNecesito = 1;

		   if(bytesNecesarios < config_tamanioBloques) cuantosBloquesMasNecesito = 1;

		   if(bytesNecesarios > config_tamanioBloques) {
			   cuantosBloquesMasNecesito = bytesNecesarios / config_tamanioBloques ;
			   cuantosBloquesMasNecesito += 1;
		   }
			printf("Bloques de mas que necesito:%d",cuantosBloquesMasNecesito);

			int numeroBloque;
			int bloquesEncontrados;


		bloquesEncontrados = cuantosBloquesMasNecesito;

		if(!elArchivoTieneBloquesSuficientes){
			bloquesEncontrados = 0;
			for(numeroBloque=0;numeroBloque<config_cantidadBloques;numeroBloque++){
		        int bit = bitarray_test_bit(bitarray,numeroBloque);
		        if(bit==0){
		        	bloquesEncontrados++;
		        	list_add(nuevosBloques,&numeroBloque);
		        }
		        if(bloquesEncontrados==cuantosBloquesMasNecesito) break;
			}
		}


			printf("Bloques encontrados :%d",bloquesEncontrados);

			if(bloquesEncontrados>=cuantosBloquesMasNecesito){
				log_info(logConsolaPantalla,"Existen bloques disponibles para almacenar la informacion");
				//guardamos en los bloques deseados


				int sizeRestante = size;
				int desplazamiento = 0;//Para el buffer


				//Primero usamos el bloque correspondiente al cursor - No queremos frag interna
				bloque = fopen(direccionBloque,"r+b");
				fseek(bloque,0,SEEK_SET);

				fseek(bloque,config_tamanioBloques-(tamanioBloqueAcumulado-cursor),SEEK_SET);
				log_info(logConsolaPantalla,"Escribiendo en bloque:%s",direccionBloque);
				fwrite(buffer,1,(tamanioBloqueAcumulado-cursor),bloque); //Bloque=64 y el tamanoActual = 40 --> Escribo 24 Bytes
				fclose(bloque);

				sizeRestante -= (tamanioBloqueAcumulado-cursor);
				desplazamiento += (tamanioBloqueAcumulado-cursor);
				indiceBloque++;

				//Despues empezamos a usar los bloques disponibles
				int s=indiceBloque;
				while(arrayBloques[s]!= NULL){
					char *nombreBloque=string_new();
					string_append(&nombreBloque, puntoMontaje);
					string_append(&nombreBloque, "Bloques/");
					string_append(&nombreBloque, arrayBloques[s]);
					string_append(&nombreBloque, ".bin");
					log_info(logConsolaPantalla,"Escribiendo en bloque:%s",nombreBloque);

					bloque=fopen(nombreBloque,"r+b");
					fseek(bloque,0,SEEK_SET);


					if(sizeRestante < config_tamanioBloques){
						fwrite(buffer + desplazamiento,1,sizeRestante,bloque);
						sizeRestante -= sizeRestante;
						desplazamiento += sizeRestante;
						printf("Le alcanzo con los bloques disponbiles");
						break;
					}
					else fwrite(buffer + desplazamiento,1,config_tamanioBloques,bloque);

					sizeRestante -=config_tamanioBloques;
					desplazamiento += config_tamanioBloques;
					s++;
				}


				if(!elArchivoTieneBloquesSuficientes){
					log_info(logConsolaPantalla,"Solicitando nuevos bloques");
				//Ahora pedimos nuevos.
				for(s=0;s<cuantosBloquesMasNecesito;s++){
					char *nombreBloque = string_new();
					string_append(&nombreBloque, puntoMontaje);
					string_append(&nombreBloque, "Bloques/");
					string_append(&nombreBloque, string_itoa(*(int*)list_get(nuevosBloques,s)));
					string_append(&nombreBloque, ".bin");
					log_info(logConsolaPantalla,"Escribiendo en bloque:%s",nombreBloque);

					bloque=fopen(nombreBloque,"wb");

					if(sizeRestante>config_tamanioBloques){

						printf("Tengo que cortar el string\n");

						fwrite(buffer + desplazamiento,1,config_tamanioBloques,bloque);
						sizeRestante -= config_tamanioBloques;
						desplazamiento += config_tamanioBloques;

					}else{ //Si entra aca, ya la proxima sale, entonces no actualizamos nada

						printf("No tuve que cortar el string\n");
						//mandarlo to-do de una
						fwrite(buffer + desplazamiento,1,sizeRestante,bloque);
					}
					fclose(bloque);

					//actualizamos el bitmap
					bitarray_set_bit(bitarray,*(int*)list_get(nuevosBloques,s));
				}
				}

			}else{
				log_error(logConsolaPantalla,"No existen suficientes bloques para escribir la informacion solicitada\n");
				validado=-1;
				send(socketKernel,&validado,sizeof(int),0);
				return;
			}
		}

		actualizarMetadataArchivo(rutaAbsoluta,size,nuevosBloques);

		validado=1;
		send(socketKernel,&validado,sizeof(int),0);
		log_info(logConsolaPantalla,"Datos almacenados--->Archivo:%s--->Informacion:%s\n",path,(char*)buffer);
	}else{
		log_error(logConsolaPantalla,"No se puede guardar datos porque el archivo no fue creado--->Archivo:%s\n",path);
		validado=-1;
		send(socketKernel,&validado,sizeof(int),0); //El archivo no existe
	}

}

void actualizarMetadataArchivo(char* path,int size,t_list* nuevosBloques){

	log_info(logConsolaPantalla,"Actualizando metadata de archivo:%s",path);
	int i;
			//Obtenemos los datos viejos
			int tamanioArchivoViejo=atoi(obtTamanioArchivo(path));
			char**arrayBloquesViejos=obtArrayDeBloquesDeArchivo(path);

			int tamanioNuevo= tamanioArchivoViejo + size;

			 //actualizamos el metadata del archivo con los nuevos bloques y el nuevo tamano del archivo
			FILE *fp = fopen(path, "w");//Para que borre to-do lo que tenia antes
			char *metadataFile = string_new();

			string_append(&metadataFile, "TAMANIO=");
			string_append(&metadataFile, string_itoa(tamanioNuevo));
			string_append(&metadataFile,"\n");

			string_append(&metadataFile, "BLOQUES=[");

			int indiceBloque=0;
		   while(!(arrayBloquesViejos[indiceBloque] == NULL)){
			   string_append(&metadataFile,arrayBloquesViejos[indiceBloque]);
			   if(arrayBloquesViejos[indiceBloque+1]!=NULL)string_append(&metadataFile,",");
			   indiceBloque++;
		   }

			for(i=0;i<nuevosBloques->elements_count;i++){
				string_append(&metadataFile,",");
				char* bloqueString=string_itoa(*(int*)list_get(nuevosBloques,i));//string_itoa(bloqs[z])
				string_append(&metadataFile,bloqueString);
			}


			string_append(&metadataFile,"]");
			fclose(fp); //Lo cierro porque la proxima linea lo volvia a abrir
			adx_store_data(path,metadataFile);
}





void* leerParaGuardar(char* nombreArchivoRecibido,int size,int cursor)
{
	char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);

	   printf("size:%d\n",size);
	   printf("Tamanio bloque:%d\n",config_tamanioBloques);

	   int cantidadBloquesNecesito;
	   if(((size+cursor)%config_tamanioBloques)==0) cantidadBloquesNecesito = ((size+cursor)/config_tamanioBloques);
	   else cantidadBloquesNecesito = ((size+cursor)/config_tamanioBloques)+1;

	   printf("Cantidad de bloques que necesito leer :%d\n",cantidadBloquesNecesito);


	   int d=0;
	   int tamanioBloqueAcumulado = config_tamanioBloques;
	   while(!(cursor<tamanioBloqueAcumulado)){ //Para saber cual es el primer bloque a leer
		   d++;
		   tamanioBloqueAcumulado += config_tamanioBloques;
	   }

	   /*
	    * Cursor = 160
	    * Bloque = 64
	    * Acumulado = 64
	    *
	    * 1. Cursor<Acumulado d++ -> Acumulado +=Bloque
	    * Acumulado=128
	    *
	    * 2. Cursor < Acumulado d++ -> Acumulado +=Bloque
	    * Acumulado=192
	    *
	    * 3. Cursor ya es menos que Acumulador.
	    */





	   FILE *bloque;

	   int sizeRestante=size;

	   char* infoTraidaDeLosArchivos = string_new();
	   char* data;
	   int sizeDentroBloque=0;
	   int cantidadBloquesLeidos=0;

	   while(cantidadBloquesLeidos < cantidadBloquesNecesito){

		   printf("Leyendo data del bloque:%s\n",arrayBloques[d]);

		   if(cantidadBloquesLeidos==0){

			   if(size>config_tamanioBloques-cursor) sizeDentroBloque=config_tamanioBloques-cursor;//Leo la porcion restante del bloque
			   else sizeDentroBloque = size; //Leo lo suficiente
			   printf("El tamano a leer del bloque es :%d\n",sizeDentroBloque);


			   char *nombreBloque = string_new();
			   string_append(&nombreBloque, puntoMontaje);
			   string_append(&nombreBloque, "Bloques/");
			   string_append(&nombreBloque, arrayBloques[d]);
			   string_append(&nombreBloque, ".bin");

			   bloque=fopen(nombreBloque, "rb");

			   data=(char*)obtenerBytesDeUnArchivo(bloque,cursor,sizeDentroBloque); //En el primero bloque, arranca del cursor

			   string_append(&infoTraidaDeLosArchivos,data);
			   sizeRestante -= sizeDentroBloque;
		   }
		   else{
			   if(sizeRestante < config_tamanioBloques) sizeDentroBloque = sizeRestante; //Es el ultimo bloque a leer
			   else sizeDentroBloque = config_tamanioBloques; //Leo to-do el bloque

			   char *nombreBloque = string_new();
			   string_append(&nombreBloque, puntoMontaje);
			   string_append(&nombreBloque, "Bloques/");
			   string_append(&nombreBloque, arrayBloques[d]);
			   string_append(&nombreBloque, ".bin");

			   bloque=fopen(nombreBloque, "rb");

			   data=(char*)obtenerBytesDeUnArchivo(bloque,0,sizeDentroBloque); //Siempre arranca del principio

			   string_append(&infoTraidaDeLosArchivos,data);
			   sizeRestante -= sizeDentroBloque;
		   }
		   d++; //Avanzo de bloque
		   cantidadBloquesLeidos++;
	   }



	 return infoTraidaDeLosArchivos;

}

void guardarDatosArchivoFunction2(char* path){//ver tema puntero, si lo tengo que recibir o que onda
	FILE* bloque;
	int validado;
	int cursor;
	int size;

	int cuantosBloquesMasNecesito;
	t_list* nuevosBloques = list_create();


	recv(socketKernel,&cursor,sizeof(int),0);
	printf("Puntero:%d\n",cursor);

	recv(socketKernel,&size,sizeof(int),0);
	printf("Tamano de la data:%d\n",size);
	void* buffer = malloc(size);

	recv(socketKernel,buffer,size,0);
	printf("Data :%s\n",(char*)buffer);

	log_info(logConsolaPantalla,"Guardando datos--->Archivo:%s--->Informacion:%s",path,(char*)buffer);

	char *nombreArchivoRecibido = string_new();
	string_append(&nombreArchivoRecibido, puntoMontaje);
	string_append(&nombreArchivoRecibido, "Archivos/");
	string_append(&nombreArchivoRecibido, path);

	printf("Toda la ruta :%s\n",nombreArchivoRecibido);

	if(access(nombreArchivoRecibido, F_OK ) != -1 ) {
		printf("Toda la ruta jejeje");
		void* todaLaInfoTraida=leerParaGuardar(path,atoi(obtTamanioArchivo(nombreArchivoRecibido)),0);

		printf("Data leida de los bloques:%s\n",todaLaInfoTraida);

		//modificar al string


		char *stringComoSeQuiere = string_new();
		string_append(&stringComoSeQuiere, string_substring_until(todaLaInfoTraida, cursor));

		printf("Archivo hasta el cursor :%s\n",stringComoSeQuiere);
		printf("Buffer a escribir:%s\n",buffer);
		string_append(&stringComoSeQuiere, (char*)buffer);

		printf("Archivo sobreescrito:%s\n",stringComoSeQuiere);



		char* loQueQuedoDespuesDePisarLosBytes=string_new();
		loQueQuedoDespuesDePisarLosBytes=string_substring_from(todaLaInfoTraida, size + cursor);

		printf("Lo que queda por concatenar:%s",loQueQuedoDespuesDePisarLosBytes);

		if(string_length(loQueQuedoDespuesDePisarLosBytes)>0){
			string_append(&stringComoSeQuiere, loQueQuedoDespuesDePisarLosBytes);
		}

		printf("Archivo actualizado :%s\n",stringComoSeQuiere);


		char** arrayBloques=obtArrayDeBloquesDeArchivo(nombreArchivoRecibido);


		int indiceBloque=0;
		while(!(arrayBloques[indiceBloque] == NULL)){


			//Vuelvo a poner todo en mis bloques con el string modificado
			char *direccionBloque = string_new();
			string_append(&direccionBloque, puntoMontaje);
			string_append(&direccionBloque, "Bloques/");
			string_append(&direccionBloque, arrayBloques[indiceBloque]);
			string_append(&direccionBloque, ".bin");


			bloque = fopen(direccionBloque,"w");
			fwrite(stringComoSeQuiere,config_tamanioBloques,1,bloque); //guardo hasta el size que me permite el bloque
			fclose(bloque);

			//Nose si esta bien que me permita escribir hasta tamanioBloques por que quiza escribe basura si es que es el ultimo bloque

			//cortar el string
			stringComoSeQuiere=string_substring_from(stringComoSeQuiere, config_tamanioBloques);
			indiceBloque++;

			//if((arrayBloques[indiceBloque+1] == NULL)) break;
		}






		//detectar si lo que quedo de string cortado le queda algo , en ese caso empiezo a pedir mas bloques.
		int seNecesecitaronMasBloques=0;
		if(string_length(stringComoSeQuiere)>0){
			seNecesecitaronMasBloques==1;
		}

		if(seNecesecitaronMasBloques){
	       size=string_length(stringComoSeQuiere);
		   if((size%config_tamanioBloques) == 0) cuantosBloquesMasNecesito = 1;
		   if(size < config_tamanioBloques) cuantosBloquesMasNecesito = 1;
		   if((size%config_tamanioBloques) == size) {
			   cuantosBloquesMasNecesito = size / config_tamanioBloques ;
			   cuantosBloquesMasNecesito += 1;
		   }

			printf("Bloques de mas que necesito:%d\n",cuantosBloquesMasNecesito);

			int numeroBloque;
			int bloquesEncontrados=0;


			for(numeroBloque=0;numeroBloque<config_cantidadBloques;numeroBloque++){
		        bool bit = bitarray_test_bit(bitarray,numeroBloque);
		        if(bit==0){
		        	bloquesEncontrados++;
		        	list_add(nuevosBloques,&numeroBloque);
		        }
		        if(bloquesEncontrados==cuantosBloquesMasNecesito) break;
			}

			printf("Bloques encontrados :%d\n",bloquesEncontrados);

			//void* stringComoSeQuiere=(void* stringComoSeQuiere);
			if(bloquesEncontrados>=cuantosBloquesMasNecesito){
							log_info(logConsolaPantalla,"Existen bloques disponibles para almacenar la informacion");
							//guardamos en los bloques deseados

							int s;

							int sizeRestante = size;
							int desplazamiento = 0;//Para el buffer

							for(s=0;s<cuantosBloquesMasNecesito;s++){
								char *nombreBloque = string_new();
								string_append(&nombreBloque, puntoMontaje);
								string_append(&nombreBloque, "Bloques/");
								string_append(&nombreBloque, string_itoa(*(int*)list_get(nuevosBloques,s)));
								string_append(&nombreBloque, ".bin");

								printf("Voy a guardar en el bloque:%s\n",string_itoa(*(int*)list_get(nuevosBloques,s)));

								bloque=fopen(nombreBloque,"w");

								if(sizeRestante>config_tamanioBloques){ //if(string_length(loQueVaQuedandoDeBuffer)>tamanioBloques)
									printf("Tengo que cortar el string\n");
									//cortar el string
									fwrite(stringComoSeQuiere + desplazamiento , config_tamanioBloques,1,bloque);
									//char* recortado=string_substring_until(buffer + desplazamiento, tamanioBloques);
									//adx_store_data(nombreBloque,recortado);
									sizeRestante -= config_tamanioBloques;
									desplazamiento += config_tamanioBloques;
									//loQueVaQuedandoDeBuffer=string_substring_from(loQueVaQuedandoDeBuffer, tamanioBloques);

								}else{ //Si entra aca, ya la proxima sale, entonces no actualizamos nada
									printf("No tuve que cortar el string\n");
									//mandarlo to-do de una
									fwrite(stringComoSeQuiere + desplazamiento,sizeRestante,1,bloque);
									//adx_store_data(nombreBloque,buffer + desplazamiento);

								}
								fclose(bloque);

								//actualizamos el bitmap
								bitarray_set_bit(bitarray,*(int*)list_get(nuevosBloques,s));
							}

			}else{
				log_error(logConsolaPantalla,"No existen suficientes bloques para escribir la informacion solicitada");
				validado=0;
				send(socketKernel,&validado,sizeof(int),0);
				return;
			}
		}

		if(seNecesecitaronMasBloques==1){
			actualizarMetadataArchivo(nombreArchivoRecibido,size,nuevosBloques);
		}

		validado=1;
		send(socketKernel,&validado,sizeof(int),0);





	}else{
		log_error(logConsolaPantalla,"El archivo no fue creado--->Archivo:%s",path);
		validado=0;
		send(socketKernel,&validado,sizeof(int),0); //El archivo no existe
	}

}
