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
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;
  //se il blocco è vuoto, ritorno -1
  if(BitMap_get(&bmap, block_num, 0) == block_num) return -1;
  //altrimenti leggo il blocco
  memcpy(dest, disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE), BLOCK_SIZE);
  return 0;
}

int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num){
  //controllo input
  if ((disk == NULL) || (src == NULL) || (block_num < 0) || (block_num > disk->header->num_blocks - 1)){
    fprintf(stderr, "Errore: Input non valido. \n");
    return -1;
  }
  if(strlen((char*)src) * 8 > BLOCK_SIZE) return -1;
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;
  //decremento il numero di blocchi liberi, se il blocco era libero
  if (BitMap_get(&bmap,block_num,0) == block_num) disk->header->free_blocks--;
  //occupo il blocco
  BitMap_set(&bmap, block_num, 1);
  //scrivo da src nel blocco desiderato
  memcpy(disk->bitmap_data + disk->header->bitmap_entries + (block_num * BLOCK_SIZE), src, BLOCK_SIZE);

  //TODO  
  //flush dati con DiskDriver_Flush

  //agggiorna il primo blocco libero con getFreeBlock
  disk->header->first_free_block = DiskDriver_getFreeBlock(disk, 0);
  return 0;
}

int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
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
  int dim = sizeof(DiskHeader) + disk->header->bitmap_entries + (disk->header->num_blocks*BLOCK_SIZE);
  return msync(disk->header, dim, MS_SYNC);
}
