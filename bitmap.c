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

  if(pos < 0) return -1;

  BitMapEntryKey bitmapkey = BitMap_blockToIndex(pos);

  char flag = 8 << bitmapkey.entry_num;  // controllo

  char set = bmap->entries[bitmapkey.entry_num]; // settare

  if(status) {

    bmap->entries[bitmapkey.entry_num] = set | flag;

    return set | flag;

  }else {

    bmap->entries[bitmapkey.entry_num] = set & (~flag);

    return set & (~flag);

  }
}
