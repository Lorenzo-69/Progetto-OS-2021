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
  int bitmap_size = (num_blocks/8)+1;
  DiskHeader* disk_header = NULL;

	if(disk == NULL || num_blocks <= 0){
		fprintf(stderr,"Errore: Input non valido DiskDriver_init. \n");
		return;
	}

	if(!access(filename,F_OK)){

    fd = open(filename,O_RDWR,(mode_t)0666);

    if(fd == -1){
			fprintf(stderr,"Errore: Impossibile aprire il file DIskDriver_init. \n");
			return;
		}

    disk_header = (DiskHeader*) mmap(0, sizeof(DiskHeader)+bitmap_size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(disk_header == MAP_FAILED){
		  fprintf(stderr,"Errore: mmap fallito DIskDriver_init \n");
      return;
    }

		disk->header = disk_header;
    disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);

	}
	else{

    fd = open(filename, O_RDWR|O_CREAT|O_TRUNC,(mode_t)0666);

    if(fd == -1){
			fprintf(stderr,"Errore: Impossibile aprire il file DiskDriver_init");
			return;
		}
    if(posix_fallocate(fd,0,sizeof(DiskHeader)+bitmap_size) > 0){
      fprintf(stderr,"Errore posix f-allocate");
      close(fd);
      return;
    }
    disk_header = (DiskHeader*) mmap(0, sizeof(DiskHeader)+bitmap_size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(disk_header == MAP_FAILED){
		  fprintf(stderr,"Error: mmap failed \n");
      close(fd);
      return;
    }
    disk->header = disk_header;
    disk->bitmap_data = (char*)disk_header + sizeof(DiskHeader);
    disk_header->num_blocks = num_blocks;
    disk_header->bitmap_blocks = num_blocks;
    disk_header->bitmap_entries = bitmap_size; 
    disk_header->free_blocks = num_blocks;
    disk_header->first_free_block = 0;
    memset(disk->bitmap_data,0, bitmap_size);

	}

	disk->fd = fd;
	return;
}

int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num,int size){
  if(block_num >= disk->header->bitmap_blocks || block_num < 0 || dest == NULL || disk == NULL){
    fprintf(stderr,"Errore: Input non valido DiskDriver_readBlock\n");
    return -1;
  } 
  BitMap bitmap;
  bitmap.num_bits = disk->header->bitmap_blocks;
  bitmap.entries = disk->bitmap_data;

  if(BitMap_get(&bitmap, block_num, 0) == block_num){
    fprintf(stderr,"Errore: Blocco vuoto DiskDriver_readBlock\n");
    return -1;
  } 
  int fd = disk->fd;
    
	off_t offset = lseek(fd, sizeof(DiskHeader)+disk->header->bitmap_entries+(block_num*BLOCK_SIZE), SEEK_SET);	
	if(offset == -1){
		fprintf(stderr,"Errore: lseek DiskDriver_readBlock\n");
		return -1;
	}

	int ret, readb = 0;
	while(readb < size){																	
		if((ret = read(fd, dest + readb, size - readb)) == -1){
			if(ret == -1){
        fprintf(stderr,"Errore: read DiskDriver_readBlock\n");
        return -1;
      }
		}
		readb += ret;						
	}
  return 0;
}

int DiskDriver_writeBlock(DiskDriver* disk, void* src, int block_num, int size){
  int ret;
  int fd = disk->fd;
  //controllo input
  if ((disk == NULL) || (src == NULL) || (block_num < 0) || (block_num > disk->header->num_blocks - 1) || size < 0){
    fprintf(stderr, "Errore: Input non valido. DiskDriver_writeblock\n");
    return -1;
  }
  if(strlen((char*)src) * 8 > BLOCK_SIZE) return -1;
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;

 if (BitMap_get(&bmap,block_num,1) == block_num){
  fprintf(stderr,"Errore: impossibile blocco occupato DiskDriver_writeblock\n");
  return -1;
 }else{
   if(BitMap_set(&bmap, block_num, 1) < 0){
    fprintf(stderr,"Errore: errore set bitmap DiskDriver_writeBlock\n");
    return -1;
   } 
    int written = 0;
    off_t offset = lseek(fd,sizeof(DiskHeader) + disk->header->bitmap_entries + (block_num*BLOCK_SIZE),SEEK_SET);
    if(offset==-1){
      fprintf(stderr,"Errore: errore lseek DiskDriver_writeBlock\n");
      return -1;
    }
    while(written < size){
      ret = write(fd,src+written,size-written);
      if(ret == -1){
        fprintf(stderr,"Errore: errore write DiskDriver_writeBlock\n");
        return -1;
      }
      written += ret;
    }
    disk->header->free_blocks--;
  }
  disk->header->first_free_block = DiskDriver_getFreeBlock(disk, 0);
  return 0;
}
int DiskDriver_freeBlock(DiskDriver* disk, int block_num){
  //controllo input
  if(block_num > disk->header->num_blocks){
    fprintf(stderr, "Errore: input non valido DiskDriver_freeBlock\n");
    return -1;
  }

  BitMap bitmap;
	bitmap.num_bits = disk->header->bitmap_entries * 8;
	bitmap.entries = disk->bitmap_data;

  if (BitMap_get(&bitmap,block_num,0) == block_num){
   fprintf(stderr,"Errore: impossibile blocco occupato DiskDriver_freeBlock\n");
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
    fprintf(stderr, "Errore: Disco non valido DIskDriver_getFreeBlock. \n");
    return -1;
  }
  //preparo bitmap
  BitMap bmap;
  bmap.num_bits = disk->header->bitmap_blocks;
  bmap.entries = disk->bitmap_data;
  //altro controllo input
  if ((start < 0) || (start > disk->header->num_blocks)){
    fprintf(stderr, "Errore: Punto d'inizio non valido DIskDriver_getFreeBlock. \n");
    return -1;
  }
  return BitMap_get(&bmap, start, 0);
}

int DiskDriver_flush(DiskDriver* disk){
  int dim = sizeof(DiskHeader) +(disk->header->num_blocks/8) +1;
  return msync(disk->header, dim, MS_SYNC);
}
