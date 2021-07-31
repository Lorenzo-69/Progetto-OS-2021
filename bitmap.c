#include <stdlib.h>
#include <stdio.h>

#include "bitmap.h"

// Lorenzo
BitMapEntryKey BitMap_blockToIndex(int num) {
  return null;
}

// Stefano
int BitMap_indexToBlock(int entry, uint8_t bit_num){

  if(bit_num < 0 || entry < 0) return -1;
  int index = ( entry*8 ) + bit_num;
  return index; 

}

// Lorenzo
int BitMap_get(BitMap* bmap, int start, int status){
  return 0;
}

// Stefano
int BitMap_set(BitMap* bmap, int pos, int status){
  return 0;
}
