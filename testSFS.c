#include "bitmap.c" 
#include "disk_driver.c"
#include "simplefs.c"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <stdlib.h>
#define TRUE 1
#define FALSE 0

// TEST
// 1 = BitMap
// 2 = DiskDriver
// 3 = SimpleFS
int test;
int use_global_test = FALSE;
int use_file_for_test = 0;
const char* filename = "./disk_driver_test1.txt";

void stampa_in_binario(char* stringa) {
	int i, j;
	for(i = 0; i < strlen(stringa); i++) {
		char c = stringa[i];
		for (j = 7; j >= 0; j--) {
	      printf("%d", ((c >> j) & 0x01)); 
	  }
	} 
}

int count_blocks(int num_bytes) {
	return num_bytes % BLOCK_SIZE == 0 ? num_bytes / BLOCK_SIZE : ( num_bytes / BLOCK_SIZE ) + 1;
}

int space_in_dir(int * file_blocks, int dim) {
	int i = 0;
	int free_spaces = 0;
	while(i < dim) {
		if(*file_blocks == 0) free_spaces++;
		file_blocks++;
		i++;
	}
	return free_spaces;
}

int main(int argc, char** argv) {

	if(!test) {
		printf("\nCosa vuoi testare?\n1 = BitMap\n2 = DiskDriver\n3 = SimpleFS\n\n>>> ");
	  scanf("%d", &test);
	}

	if(test == 1) {


		// Test BitMap_blockToIndex
		int num = 765;
		printf("\n+++ Test BitMap_blockToIndex(%d)", num);   
		BitMapEntryKey block = BitMap_blockToIndex(num);
		printf("\n    La posizione del blocco è %d, ovvero la entry %d al bit %d", num, block.entry_num, block.bit_num);
	 
		// Test BitMap_indexToBlock 
		printf("\n\n+++ Test BitMap_indexToBlock(block)");
		int posizione = BitMap_indexToBlock(block.entry_num, block.bit_num); 
		printf("\n    Abbiamo la entry %d e il bit %d, ovvero la posizione %d", block.entry_num, block.bit_num, posizione);

		// Test BitMap_set
		DiskDriver disk;
		BitMap bitmap;
		if(use_global_test) {
			DiskDriver_init(&disk, "test/test.txt", 50); 
		}else{
			DiskDriver_init(&disk,filename, 50); 
		}
		bitmap.num_bits = disk.header->bitmap_entries * 8;
		bitmap.entries = disk.bitmap_data;
		printf("\n\n+++ Test BitMap_set()");
		printf("\n+++ Test DiskDriver_init(disk, \"test.txt\", 50)");
		printf("\n    Prima => ");
		stampa_in_binario(bitmap.entries);
		printf("\n    Bitmap_set(6, 1) => %d", BitMap_set(&bitmap, 6, 1));
		printf("\n    Dopo  => ");
		stampa_in_binario(bitmap.entries);

		// Test BitMap_get
		printf("\n\n+++ Test BitMap_get()");
		int start = 6, status = 0;    
		printf("\n    Partiamo dalla posizione %d e cerchiamo %d => %d", start, status, BitMap_get(&bitmap, start, status));
		start = 2, status = 1;
		printf("\n    Partiamo dalla posizione %d e cerchiamo %d => %d", start, status, BitMap_get(&bitmap, start, status));
		start = 11, status = 0;
		printf("\n    Partiamo dalla posizione %d e cerchiamo %d => %d", start, status, BitMap_get(&bitmap, start, status));
		start = 13, status = 1;
		printf("\n    Partiamo dalla posizione %d e cerchiamo %d => %d", start, status, BitMap_get(&bitmap, start, status));

	}else if(test == 2) {

		// Test DiskDriver_init   
		printf("\n+++ Test DiskDriver_init()");
		printf("\n+++ Test DiskDriver_getFreeBlock()");
		printf("\n+++ Test BitMap_get()");
		DiskDriver disk;
		if(use_global_test) {
			DiskDriver_init(&disk, filename, 50); 
		}else{
			DiskDriver_init(&disk, filename, 50); 
		}
		BitMap bitmap;
		bitmap.num_bits = disk.header->bitmap_entries * 8;
		bitmap.entries = disk.bitmap_data;
		printf("\n    BitMap creata e inizializzata correttamente");
		printf("\n    Primo blocco libero => %d", disk.header->first_free_block); 

		// Test DiskDriver_writeBlock  
		printf("\n\n+++ Test DiskDriver_writeBlock()");
		printf("\n+++ Test DiskDriver_flush()");
		printf("\n    Prima => ");
		//int lunghezza = strlen(testo);
		stampa_in_binario(bitmap.entries);
		printf("\n    Il risultato della writeBlock(\"Ciao\", 4) è %d", DiskDriver_writeBlock(&disk, "Ciao", 4,4));
		printf("\n    Dopo  => ");
		stampa_in_binario(bitmap.entries);

		// Test DiskDriver_readBlock
		printf("\n\n+++ Test DiskDriver_readBlock()");
		
		void * dest = malloc(BLOCK_SIZE);
		printf("\n    Controlliamo tramite una readBlock(dest, 4)   => %d", DiskDriver_readBlock(&disk, dest, 4,4));
		printf("\n    Dopo la readBlock, la dest contiene           => %s", (char *) dest);

		free(dest);

		// Test DiskDriver_freeBlock
		printf("\n\n+++ Test DiskDriver_freeBlock()");
		printf("\n    Prima => ");
		stampa_in_binario(bitmap.entries);
		printf("\n    Libero il blocco %d, la funzione ritorna: %d", 4, DiskDriver_freeBlock(&disk, 4));
		printf("\n    Dopo  => ");
		stampa_in_binario(bitmap.entries);

	}else if(test == 3) {

		// Test SimpleFS_init
		printf("\n+++ Test SimpleFS_init()");
		printf("\n+++ Test SimpleFS_format()");
		SimpleFS fs;
		DiskDriver disk;
		if(use_global_test) {
			DiskDriver_init(&disk, filename, 50); 
		}else{
			DiskDriver_init(&disk, filename, 50); 
		}
		DirectoryHandle * directory_handle = SimpleFS_init(&fs, &disk);
		if(directory_handle != NULL) {
			printf("\n    File System creato e inizializzato correttamente");
		}else{
			printf("\n    Errore nella creazione del file system\n");
			return 0;
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

		// Test SimpleFS_createFile
		printf("\n\n+++ Test SimpleFS_createFile()");
		int i, num_file = 4;
		char names [4][255]= {"prova1.txt", "prova2.txt", "prova3.txt", "prova4.txt"};
		for(i = 0; i < num_file; i++) {
			
			if(SimpleFS_createFile(directory_handle,names[i]) != NULL) {
				printf("\n    File %s creato correttamente", names[i]);
			}else{
				printf("\n    Errore nella creazione di %s", names[i]);
			}
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

	 	// Test SimpleFS_mkDir
		printf("\n\n+++ Test SimpleFS_mkDir()");
		int ret = SimpleFS_mkDir(directory_handle, "pluto");
		printf("\n    SimpleFS_mkDir(dh, \"pluto\") => %d", ret);
		if(ret == 0) {
			printf("\n    Cartella creata correttamente");
		}else{
			printf("\n    Errore nella creazione della cartella\n");
			return 0;
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);


	 	// Test SimpleFS_readDir
		int dim = (directory_handle->dcb->num_entries/2)+1;
		printf("\n\n+++ Test SimpleFS_readDir()");
		printf("\n    Nella cartella ci sono %d elementi:", dim);
		char** elenco2 = (char**)malloc((directory_handle->dcb->num_entries) * sizeof(char*));
		for (i = 0; i < (directory_handle->dcb->num_entries); i++) {
			elenco2[i] = (char*)malloc(128*sizeof(char));
		}
		SimpleFS_readDir(elenco2, directory_handle);
		for(i = 0; i < dim; i++) {
			printf("\n    > %s", elenco2[i]);
		}

		for (i = 0; i < (directory_handle->dcb->num_entries); i++) {
			free(elenco2[i]);
		}
		free(elenco2);

	 	// Test SimpleFS_openFile
		printf("\n\n+++ Test SimpleFS_openFile()");
		const char* nome_file = "prova1.txt";
		FileHandle * file_handle;
		file_handle = SimpleFS_openFile(directory_handle, nome_file);
		ret = file_handle == NULL ? -1 : 0;
		printf("\n    SimpleFS_openFile(directory_handle, \"%s\") => %d", nome_file, ret);
		if(file_handle != NULL) {
			printf("\n    File aperto correttamente");
		}else{
			printf("\n    Errore nell'apertura del file\n");
			return 0;
		}

	 	// Test SimpleFS_write
		printf("\n\n+++ Test SimpleFS_write()");
		char* stringa = "Nel mezzo del cammin";
		//int len = strlen(stringa);
		ret = SimpleFS_write(file_handle, stringa, 20);
		printf("\n    SimpleFS_write(file_handle, stringa, %d) => %d", 20, ret);
		if(ret == 20) {
			printf("\n    Scrittura avvenuta correttamente");
		}else{
			printf("\n    Errore nella scrittura del file\n");
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

	 	// Test SimpleFS_read
		printf("\n\n+++ Test SimpleFS_read()");
		char* data = (char*) calloc(21,sizeof(char));
		printf("\n    SimpleFS_read(file_handle, data, %d) ha restituito: %d", 20, SimpleFS_read(file_handle,(void*) data, 20));
		printf("\n    Adesso \"data\" contiene: %s", (char*) data);
		free(data);

		// Test SimpleFS_changeDir
		printf("\n\n+++ Test SimpleFS_changeDir()");
		printf("\n    SimpleFS_changeDir(directory_handle, \"pluto\") => %d", SimpleFS_changeDir(directory_handle, "pluto"));
		printf("\n    SimpleFS_changeDir(directory_handle, \"..\")    => %d", SimpleFS_changeDir(directory_handle, ".."));
		printf("\n    SimpleFS_changeDir(directory_handle, \"..\")    => %d", SimpleFS_changeDir(directory_handle, ".."));
		
		// Test SimpleFS_seek
		printf("\n\n+++ Test SimpleFS_seek()");
		int pos = 10;
		ret = SimpleFS_seek(file_handle, pos);
		printf("\n    SimpleFS_seek(file_handle, %d) => %d", pos, ret);
		if(ret == pos) {
			printf("\n    Spostamento del cursore avvenuto correttamente");
		}else{
			printf("\n    Errore nello spostamento del cursore\n");
			return 0;
		}

		// Test SimpleFS_close
		printf("\n\n+++ Test SimpleFS_close()");
		ret = SimpleFS_close(file_handle);
		printf("\n    SimpleFS_close(file_handle) => %d", ret);
		if(ret >= 0) {
			printf("\n    Chiusura del file avvenuta correttamente");
		}else{
			printf("\n    Errore nella chiusura del file\n");
			return 0;
		}
		
		char namesPluto [5][255]= {"pluto1.txt", "pluto2.txt", "pluto3.txt", "pluto4.txt", "pluto5.txt"};
		SimpleFS_changeDir(directory_handle, "pluto");
		for(i=0; i<5; i++) {
			SimpleFS_createFile(directory_handle, namesPluto[i]);
		}
		printf("\n\n    Ho aggiunto %d files in \"pluto\"", 5);

		dim = (directory_handle->dcb->num_entries/2);
		printf("\n\n+++ Test SimpleFS_readDir()");
		printf("\n    Nella cartella ci sono %d elementi:", dim);
		char** elencopluto = (char**)malloc((directory_handle->dcb->num_entries) * sizeof(char*));
		for (i = 0; i < (directory_handle->dcb->num_entries); i++) {
			elencopluto[i] = (char*)malloc(128*sizeof(char));
		}
		SimpleFS_readDir(elencopluto, directory_handle);
		for(i = 0; i < dim; i++) {
			printf("\n    > %s", elencopluto[i]);
		}


		for (i = 0; i < (directory_handle->dcb->num_entries); i++) {
			free(elencopluto[i]);
		}
		free(elencopluto);


		SimpleFS_changeDir(directory_handle, "..");
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

		dim = (directory_handle->dcb->num_entries/2) + 1;
		printf("\n\n+++ Test SimpleFS_readDir()");
		printf("\n    Nella cartella ci sono %d elementi:", dim);
		char** elenco = (char**)malloc((directory_handle->dcb->num_entries) * sizeof(char*));
		for (i = 0; i < (directory_handle->dcb->num_entries); i++) {
			elenco[i] = (char*)malloc(128*sizeof(char));
		}
		SimpleFS_readDir(elenco, directory_handle);
		for(i = 0; i < dim; i++) {
			printf("\n    > %s", elenco[i]);
		}
		for (i = 0; i < (directory_handle->dcb->num_entries); i++) {
			free(elenco[i]);
		}
		free(elenco);

		// Test SimpleFS_remove
		/*printf("\n\n+++ Test SimpleFS_remove() [cartella]");
		char* directory = "pluto";
		//strcpy(nome_file, "pluto");
		ret = SimpleFS_remove(directory_handle, directory);
		printf("\n    SimpleFS_remove(file_handle, \"%s\") => %d", directory, ret);
		if(ret >= 0) {
			printf("\n    Cancellazione del file avvenuta correttamente");
		}else{
			printf("\n    Errore nella cancellazione del file\n");
			return 0;
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);*/

		// Test SimpleFS_remove
		printf("\n\n+++ Test SimpleFS_remove() [file]");
		char* nome = "prova2.txt";
		ret = SimpleFS_remove(directory_handle, nome);
		printf("\n    SimpleFS_remove(directory_handle, \"%s\") => %d", nome, ret);
		if(ret >= 0) {
			printf("\n    Cancellazione del file avvenuta correttamente");
		}else{
			printf("\n    Errore nella cancellazione del file\n");
			return 0;
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);
		free(directory_handle);
	}
	printf("\n\n");
}
