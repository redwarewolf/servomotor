#ifndef _CAPAFS_
#define _CAPAFS_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <commons/bitarray.h>
#include <commons/string.h>


unsigned char *mmapDeBitmap;
t_bitarray * bitarray;

char *puertoFS;
char *puntoMontaje;
char *puerto_Kernel;
char *tamanioBloquesEnChar;
char *cantidadBloquesEnChar;
int config_tamanioBloques;
int config_cantidadBloques;
char* magicNumber;
char *ipFS;
t_config* configuracion_FS;

void testeommap(){
	unsigned char *f;
    int size;
    struct stat s;
    const char * file_name = puntoMontaje;
    int fd = open (file_name, O_RDONLY);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;
    f = (char *) mmap (0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    int i;
    for (i = 0; i < size; i++) {
        char c;

        c = f[i];
        printf("%c",c);
    }


    //Obtener el page size:
   // int pagesize = getpagesize();
    //size = s.st_size;
    //size += pagesize-(size%pagesize);
}




void leerConfiguracion(char* ruta){
	configuracion_FS = config_create(ruta);

	puertoFS= config_get_string_value(configuracion_FS,"PUERTO_FS");
	ipFS= config_get_string_value(configuracion_FS, "IP_FS");
	puntoMontaje = config_get_string_value(configuracion_FS,"PUNTO_MONTAJE");
	puerto_Kernel= config_get_string_value(configuracion_FS,"PUERTO_KERNEL");
}





void leerConfiguracionMetadata(){

	char* puntoMontajeMetadata = string_new();
	string_append(&puntoMontajeMetadata,puntoMontaje);
	string_append(&puntoMontajeMetadata,"Metadata/Metadata.bin");

	configuracion_FS = config_create(puntoMontajeMetadata);

	config_tamanioBloques= atoi(config_get_string_value(configuracion_FS,"TAMANIO_BLOQUES"));
	config_cantidadBloques= atoi(config_get_string_value(configuracion_FS, "CANTIDAD_BLOQUES"));
	magicNumber = config_get_string_value(configuracion_FS,"MAGIC_NUMBER");
}

char** obtArrayDeBloquesDeArchivo(char* ruta){
	configuracion_FS = config_create(ruta);

	return config_get_array_value(configuracion_FS, "BLOQUES");
}


char* obtTamanioArchivo(char* ruta){
	configuracion_FS = config_create(ruta);

	return config_get_string_value(configuracion_FS,"TAMANIO");
}



void imprimirConfiguraciones(){
		printf("---------------------------------------------------\n");
		printf("CONFIGURACIONES\nIP FS:%s\nPUERTO FS:%s\nPUNTO MONTAJE:%s\n",ipFS,puertoFS,puntoMontaje);
		printf("\n \nTAMANIO BLOQUES:%d\nCANTIDAD BLQOUES:%d\nMAGIC NUMBER:%s\n",config_tamanioBloques,config_cantidadBloques,magicNumber);
		printf("---------------------------------------------------\n");
}


void inicializarBitMap(){
	FILE *f;
	f = fopen("../metadata/Bitmap.bin", "wr+");
	int i;
	for ( i=0;i<5192; i++) {
	    fputc(1, f);
	}
	fclose(f);
}

void inicializarMmap(){
    int size;
    struct stat s;

    char* puntoMontajeBitmap = string_new();
    string_append(&puntoMontajeBitmap,puntoMontaje);
    string_append(&puntoMontajeBitmap,"Metadata/Bitmap.bin");

    int fd = open (puntoMontajeBitmap, O_RDWR);

    /* Get the size of the file. */
    int status = fstat (fd, & s);
    size = s.st_size;
	mmapDeBitmap = mmap (0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

}


#endif


