#include "bitmap.h"
#include <stdio.h>

BitMapEntryKey BitMap_blockToIndex(int num) {
	int idx = num/8;
	char off = num%8;
	BitMapEntryKey entry;
	entry.entry_num = idx;
	entry.bit_num = off;
	return entry;
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){

  if(bit_num < 0 || entry < 0) return -1;
  int index = ( entry*8 ) + bit_num;
  return index; 

}

int BitMap_get(BitMap* bmap, int start, int status){
	if(start < 0 || start > bmap->num_bits) return -1;

	int i;

	for(i = start; i < bmap->num_bits; i++) {
		BitMapEntryKey entry = BitMap_blockToIndex(i);
		int pos = entry.entry_num;

	 	int value = (bmap->entries[pos] & (1 << (7 - entry.bit_num))); 

		if(status == 1) {
			if(value > 0) return i;
		}else{
			if(value == 0) return i;
		}
	}
	return -1;
	
}

int BitMap_set(BitMap* bmap, int pos, int status){


	if(pos < 0 || pos > bmap->num_bits) return -1;

	BitMapEntryKey bitmapkey = BitMap_blockToIndex(pos);

	uint8_t val = 1 << (7 - bitmapkey.bit_num);
	if(status){
		bmap->entries[bitmapkey.entry_num] |= val;
  	}else{
   		bmap->entries[bitmapkey.entry_num] &= ~(val);
    }
    return status;
}
