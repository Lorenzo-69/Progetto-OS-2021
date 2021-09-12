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
const char* filename = "./disk_driver_test24.txt";

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
            
			//char disk_filename[255];
			//sprintf(disk_filename, "test/%d.txt", time(NULL));
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
			char disk_filename[255];
			sprintf(disk_filename, "test/%d.txt", time(NULL));
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
		stampa_in_binario(bitmap.entries);
		printf("\n    Il risultato della writeBlock(\"Ciao\", 4) è %d", DiskDriver_writeBlock(&disk, "Ciao", 4,strlen("Ciao")));
        //printf("\n    Il risultato della writeBlock(\"Ciao\", 2) è %d", DiskDriver_writeBlock(&disk, "Ciao", 2,strlen("Ciao")));
		printf("\n    Dopo  => ");
		stampa_in_binario(bitmap.entries);

		// Test DiskDriver_readBlock
		printf("\n\n+++ Test DiskDriver_readBlock()");
		void * dest = malloc(BLOCK_SIZE);
		printf("\n    Controlliamo tramite una readBlock(dest, 4)   => %d", DiskDriver_readBlock(&disk, dest, 4,strlen("Ciao")));
        //printf("\n    Controlliamo tramite una readBlock(dest, 3)   => %d", DiskDriver_readBlock(&disk, dest, 3,sizeof(dest)));
        //printf("\n    Controlliamo tramite una readBlock(dest, 6)   => %d", DiskDriver_readBlock(&disk, dest, 6,sizeof(dest))); //prima era 4
		printf("\n    Dopo la readBlock, la dest contiene           => %s", (char *) dest);

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
			//char disk_filename[255];
			//sprintf(disk_filename, "test/%d.txt", time(NULL));
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
			//char filename_created[255];
			//sprintf(filename_created, "prova_%d.txt", directory_handle->dcb->num_entries);
			
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
		printf("\n\n+++ Test SimpleFS_readDir()");
		printf("\n    Nella cartella ci sono %d elementi:", directory_handle->dcb->num_entries);
		//char ** elenco2 = malloc(directory_handle->dcb->num_entries * 255);
		char elenco2 [directory_handle->dcb->num_entries][255];
		//SimpleFS_readDir(elenco2, directory_handle);
		/*for(i = 0; i < directory_handle->dcb->num_entries; i++) {
			printf("\n    > %s", elenco2[i]);
		}*/

	 	// Test SimpleFS_openFile
		printf("\n\n+++ Test SimpleFS_openFile()");
		//char nome_file[255] = "prova_1.txt";
		const char* nome_file = "prova_1.txt";
		FileHandle * file_handle = malloc(sizeof(FileHandle));
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
		FILE * file_test = fopen("divina_commedia.txt", "r");
		struct stat fileStat;
		stat("divina_commedia.txt", &fileStat);
		char stringa[fileStat.st_size];
		if(use_file_for_test) {
			fread(stringa, sizeof(char), fileStat.st_size, file_test);
		}else{
			strcpy(stringa, "Nel mezzo del cammin di nostra vita mi ritrovai per una selva oscura ché la diritta via era smarrita. Ahi quanto a dir qual era è cosa dura esta selva selvaggia e aspra e forte che nel pensier rinova la paura! Tant'è amara che poco è più morte; ma per trattar del ben ch'i' vi trovai, dirò de l'altre cose ch'i' v'ho scorte. Io non so ben ridir com'i' v'intrai, tant'era pien di sonno a quel punto che la verace via abbandonai. Ma poi ch'i' fui al piè d'un colle giunto, là dove terminava quella valle che m'avea di paura il cor compunto, guardai in alto, e vidi le sue spalle vestite già deNel mezzo del cammin di nostra vita mi ritrovai per una selva oscura ché la diritta via era smarrita. Ahi quanto a dir qual era è cosa dura esta selva selvaggia e aspra e forte che nel pensier rinova la paura! Tant'è amara che poco è più morte; ma per trattar del ben ch'i' vi trovai, dirò de l'altre cose ch'i' v'ho scorte. Io non so ben ridir com'i' v'intrai, tant'era pien di sonno a quel punto che la verace via abbandonai. Ma poi ch'i' fui al piè d'un colle giunto, là dove terminava quella valle che m'avea di paura il cor compunto, guardai in alto, e vidi le sue spalle vestite già d");
			//strcpy(stringa, "Nel mezzo del cammin");
		}
		ret = SimpleFS_write(file_handle, stringa, strlen(stringa));
		printf("\n    SimpleFS_write(file_handle, stringa, %zu) => %d", strlen(stringa), ret);
		if(ret == strlen(stringa)) {
			printf("\n    Scrittura avvenuta correttamente");
		}else{
			printf("\n    Errore nella scrittura del file\n");
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

		// Test SimpleFS_write() con pos != 0
		printf("\n\n+++ Test SimpleFS_write()");
		SimpleFS_seek(file_handle, 12);
		ret = SimpleFS_write(file_handle, "i viaggi", 8);
		printf("\n    SimpleFS_seek(file_handle, 12) => %d", SimpleFS_seek(file_handle, 12));
		printf("\n    SimpleFS_write(file_handle, stringa, %d) => %d", 8, ret);
		if(ret == 8) {
			printf("\n    Scrittura avvenuta correttamente");
		}else{
			printf("\n    Errore nella scrittura del file\n");
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

	 	// Test SimpleFS_read
		printf("\n\n+++ Test SimpleFS_read()");
		int size = file_handle->fcb->fcb.size_in_bytes;
		char data[size];
		printf("\n    SimpleFS_read(file_handle, data, %d) ha restituito: %d", size, SimpleFS_read(file_handle, data, size));
		printf("\n    Adesso \"data\" contiene: %s", data);

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
		
		SimpleFS_changeDir(directory_handle, "pluto");
		for(i=0; i<500; i++) {
			char nomello[255];
			sprintf(nomello, "%s%d%s", "pluto_", i, ".txt");
			SimpleFS_createFile(directory_handle, nomello);
		}
		printf("\n\n    Ho aggiunto %d files in \"pluto\"", directory_handle->dcb->num_entries);
		SimpleFS_changeDir(directory_handle, "..");
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

		// Test SimpleFS_remove
		printf("\n\n+++ Test SimpleFS_remove() [cartella]");
		strcpy(nome_file, "pluto");
		ret = SimpleFS_remove(directory_handle, nome_file);
		printf("\n    SimpleFS_remove(file_handle, \"%s\") => %d", nome_file, ret);
		if(ret >= 0) {
			printf("\n    Cancellazione del file avvenuta correttamente");
		}else{
			printf("\n    Errore nella cancellazione del file\n");
			return 0;
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);

		// Test SimpleFS_remove
		printf("\n\n+++ Test SimpleFS_remove() [file]");
		strcpy(nome_file, "prova_2.txt");
		ret = SimpleFS_remove(directory_handle, nome_file);
		printf("\n    SimpleFS_remove(file_handle, \"%s\") => %d", nome_file, ret);
		if(ret >= 0) {
			printf("\n    Cancellazione del file avvenuta correttamente");
		}else{
			printf("\n    Errore nella cancellazione del file\n");
			return 0;
		}
		printf("\n    BitMap => ");
		stampa_in_binario(disk.bitmap_data);
	}
	printf("\n\n");
}