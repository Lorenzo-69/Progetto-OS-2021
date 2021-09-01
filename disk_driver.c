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
    if ( fd != -1 ){
      fprintf(stderr, "Errore: Impossibile aprire il file. \n");
      return;
    }
    disk_header = (DiskHeader*) mmap(0, sizeof(DiskHeader) + bitmap_size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
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
    fd = open(filename, O_CREAT | O_RDWR, 0666);
    if ( fd != -1 ){
      fprintf(stderr, "Errore: Impossibile aprire il file. \n");
      return;
    }
    disk_header = (DiskHeader*) mmap(0, sizeof(DiskHeader) + bitmap_size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
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

int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  //controllo input
  if ((disk == NULL) || (dest == NULL) || (block_num < 0) || (block_num > disk->header->num_blocks - 1)){
    fprintf(stderr, "Errore: Input non valido. \n");
    return -1;
  }

  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;

  int res = BitMap_read_atIndex(&bmap, block_num);
  if (res == 1) return 0;
  else if (res == 0){
    memcpy(dest, disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE), BLOCK_SIZE);
    return -1;
  } 
  fprintf(stderr, "Errore: porta il pc ad  aggiustare!!!");
  return -2; 
}

int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
  return 0;
}

int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  return 0;
}

int DiskDriver_getFreeBlock(DiskDriver* disk, int start){
  return 0;
}

int DiskDriver_flush(DiskDriver* disk){
  return 0;
}
