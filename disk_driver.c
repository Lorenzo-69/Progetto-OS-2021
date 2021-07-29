#include "disk_driver.h"

void DiskDriver_init(DiskDriver* disk, const char* filename, int num_blocks){
  return;
}

int DiskDriver_readBlock(DiskDriver* disk, void* dest, int block_num){
  return 0;
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
