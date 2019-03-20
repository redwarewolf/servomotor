
void printFilePermissions(char* archivo){

    struct stat fileStat;
    stat(archivo,&fileStat);


    printf("Information for %s\n",archivo);
    printf("---------------------------\n");
    printf("File Size: \t\t%d bytes\n",fileStat.st_size);
    printf("Number of Links: \t%d\n",fileStat.st_nlink);
    printf("File inode: \t\t%d\n",fileStat.st_ino);

    printf("File Permissions: \t");
    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
}

int archivoEnModoEscritura(char* archivo){
	//Aca en realidad lo que tengo que hacer es recibir del Kernel
	//si esta en modo Escritura ese archivo en la tabla de archivos por proceso
	//pero por ahora queda asi para no andar metiendo sockets de por medio
	 struct stat fileStat;
	    stat(archivo,&fileStat);


	   if(fileStat.st_mode & S_IWGRP){
		   return 1 ;
	   }else{
		   return 0;
	   }
}

int archivoEnModoLectura(char *archivo){
	 struct stat fileStat;
	    stat(archivo,&fileStat);
	        //return 1;

	    if(fileStat.st_mode & S_IROTH){
	 		   return 1 ;
	 	   }else{
	 		   return 0;
	 	   }


}





/*get:  read n bytes from position pos */
void* obtenerBytesDeUnArchivo(FILE *fp, int offset, int size)
{
	printf("Leyendo datos\n");
	printf("Offset:%d\n",offset);
	printf("Size:%d\n",size);

		void* informacion = malloc(size);
		fseek(fp,offset,SEEK_SET);
		fread(informacion,size,1,fp);

		printf("La informacion leida es:%s\n",(char*)informacion);
		fclose(fp);
		return informacion;
}

char *readFile(char *fileName)
{
    FILE *file;
    char *code = malloc(1000 * sizeof(char));
    file = fopen(fileName, "r");
    do
    {
      *code++ = (char)fgetc(file);

    } while(*code != EOF);
    return code;
}


void adx_store_data(const char *filepath, const char *data)
{
    FILE *fp = fopen(filepath, "ab");
    if (fp != NULL)
    {
        fputs(data, fp);
        fclose(fp);
    }
}


int cantBytesFile(char *filepath){


	t_config* configuracion_FS = config_create(filepath);

	return atoi(config_get_string_value(configuracion_FS, "TAMANIO"));
}

