#include "disk_driver.h"
#include "bitmap.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
  int fd;
  int bitmap_size = (num_blocks/8) + 1;
  DiskHeader* disk_header = NULL;
  
  //controlla validità input
  if(disk == NULL || num_blocks <= 0){
    fprintf(stderr, "Errore: Input non valido. \n");
		return;
  }

  //apri o crea file con syscall
  if (!access(filename, F_OK)){
    //file esistente
    fd = open(filename, O_RDWR, 0666);
    if ( fd == -1 ){
      fprintf(stderr, "Errore: Impossibile aprire il file. \n");
      return;
    }
    if(posix_fallocate(fd,0,sizeof(DiskHeader) + bitmap_size) != 0){
      printf("\nerrore fallocate\n");
      close(fd);
      return;
    }
    disk_header = (DiskHeader*) mmap(NULL, sizeof(DiskHeader) + bitmap_size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if (disk_header == MAP_FAILED){
      fprintf(stderr, "Errore: mappatura di diskheader (file esistente)");
      close(fd);
      return;
    }
    disk->header = disk_header;
    disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);
    disk->fd = fd;
  }
  else{
    //file non esistente
    fd = open(filename, O_CREAT | O_RDWR, (mode_t) 0666);
    if ( fd == -1 ){
      fprintf(stderr, "Errore: Impossibile aprire il file. \n");
      return;
    }
    if(posix_fallocate(fd,0,sizeof(DiskHeader) + bitmap_size) != 0){
      printf("\nerrore fallocate\n");
      close(fd);
      return;
    }
    disk_header = (DiskHeader*) mmap(NULL, sizeof(DiskHeader) + bitmap_size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if (disk_header == MAP_FAILED){
      fprintf(stderr, "Errore: mappatura di disk_header (file non esistente)");
      close(fd);
      return;
    }
    
    disk_header->num_blocks = num_blocks;
    disk_header->bitmap_blocks = num_blocks;
    disk_header->bitmap_entries = bitmap_size;
    disk_header->free_blocks = num_blocks;
    disk_header->first_free_block = 0;
    disk->header = disk_header;
    disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);
    disk->fd = fd;
    memset(disk->bitmap_data,0, bitmap_size); 
  }
  return;
}

int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num,int size){
  int fd = disk->fd;
  //controllo input
  if ((disk == NULL) || (dest == NULL) || (block_num < 0) || (block_num > disk->header->num_blocks - 1)){
    fprintf(stderr, "Errore: Input non valido. \n");
    return -1;
  }
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;
  //se il blocco è vuoto, ritorno -1
  if(BitMap_get(&bmap, block_num, 0) == block_num){
    printf("era questo if");
    return -1;
  } 
  //altrimenti leggo il blocco
  memcpy(dest, disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE), BLOCK_SIZE);
  /*int ret;
  int readb = 0;
   off_t offset = lseek(fd,sizeof(DiskHeader) + disk->header->bitmap_entries + (block_num*BLOCK_SIZE),SEEK_SET);
     if(offset==-1){
       printf("errore lseek");
       return -1;
     }
  while(readb < size){
    //printf("while read\n");
    ret = read(fd,dest+readb,size-readb);
    if(ret == -1){
      printf("\nerrore lettura readBlock()\n");
      return -1;
    }
    readb += ret;
  }*/
  return 0;
}

int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num, int size){
  int ret;
  int fd = disk->fd;
  //controllo input
  if ((disk == NULL) || (src == NULL) || (block_num < 0) || (block_num > disk->header->num_blocks - 1) || size < 0){
    fprintf(stderr, "Errore: Input non valido. \n");
    return -1;
  }
  if(strlen((char*)src) * 8 > BLOCK_SIZE) return -1;
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;
  /*
  //decremento il numero di blocchi liberi, se il blocco era libero
  if (BitMap_get(&bmap,block_num,0) == block_num) disk->header->free_blocks--;
  //occupo il blocco
  BitMap_set(&bmap, block_num, 1);
  //scrivo da src nel blocco desiderato
  
  */

 if (BitMap_get(&bmap,block_num,1) == block_num){
   printf("impossibile blocco occupato\n");
   return -1;
 }else{
   disk->header->free_blocks--;
   if(BitMap_set(&bmap, block_num, 1) < 0){
     printf("errore set bitmap");
     return -1;
   } 
   memcpy(disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE), src, BLOCK_SIZE);
   /*

     //printf("era quest'altro if");
     disk->header->first_free_block = DiskDriver_getFreeBlock(disk,0); //???
     int written = 0;
     off_t offset = lseek(fd,sizeof(DiskHeader) + disk->header->bitmap_entries + (block_num*BLOCK_SIZE),SEEK_SET);
     if(offset==-1){
       printf("errore lseek");
       return -1;
     }
     while(written < size){
       printf("\nera il while della write\n");
       ret = write(fd,src+written,size-written);
       if(ret == -1){
         printf("\nerrore write writeBlock()\n");
         return -1;
       }
       written += ret;
       printf("%d bytes written\n",written);*/
     
     
   

 }



  
  /*if (DiskDriver_flush(disk) == -1){
    fprintf(stderr, "Errore: dati non sicronizzati correttamente");
    return -1;
  }
  //agggiorna il primo blocco libero con getFreeBlock
  disk->header->first_free_block = DiskDriver_getFreeBlock(disk, 0);*/
  return 0;
}

int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  //controllo input
  if(block_num > disk->header->num_blocks){
    fprintf(stderr, "Errore: input non valido\n");
    return -1;
  }

  BitMap bitmap;
	bitmap.num_bits = disk->header->bitmap_entries * 8;
	bitmap.entries = disk->bitmap_data;

  if (BitMap_get(&bitmap,block_num,0) == block_num){
   printf("impossibile blocco occupato\n");
   return -1;
 }

  //se il blocco non è già libero, aumento i blocchi liberi nell'header 
  if (DiskDriver_getFreeBlock(disk, block_num) != block_num) disk->header->free_blocks++;
  //preparo la bitmap
  
  //libero il blocco nella bitmap
  BitMap_set(&bitmap, block_num, 0);
	disk->bitmap_data = bitmap.entries;
	DiskDriver_flush(disk);
  //se necessario, aggiorno il first_free_block
  if(block_num < disk->header->first_free_block) disk->header->first_free_block = block_num;
  return 0;
}

int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
  //controllo input
  if ((disk == NULL)){
    fprintf(stderr, "Errore: Disco non valido. \n");
    return -1;
  }
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;
  //altro controllo input
  if ((start < 0) || (start > disk->header->num_blocks)){
    fprintf(stderr, "Errore: Punto d'inizio non valido. \n");
    return -1;
  }
  return BitMap_get(&bmap, start, 0);
}

int DiskDriver_flush(DiskDriver* disk){
  //int dim = sizeof(DiskHeader) + disk->header->bitmap_entries + (disk->header->num_blocks*BLOCK_SIZE); (num_blocks/8) + 1;
  int dim = sizeof(DiskHeader) +(disk->header->num_blocks/8) +1;
  return msync(disk->header, dim, MS_SYNC);
}
