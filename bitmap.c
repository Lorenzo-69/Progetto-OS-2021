#include "bitmap.h"

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
	
	for(int i=start; i<bmap->num_bits; i++){
		BitMapEntryKey entry = BitMap_blockToIndex(i);
		int pos = entry.entry_num;
		int value = (bmap->entries[pos] >> (8-(entry.bit_num))) & 0x01;
		//oppure value = bmap->entries[pos] << entry->bit_num & 0x80;
		if(value == status) return i;
	}
	return -1;
	
}

int BitMap_set(BitMap* bmap, int pos, int status){

  if(pos < 0 || pos > bmap->num_bits) return -1;

  BitMapEntryKey bitmapkey = BitMap_blockToIndex(pos);

  char flag = 1 << bitmapkey.entry_num;  // controllo

  char set = bmap->entries[bitmapkey.entry_num]; // settare

  if(status) {

    bmap->entries[bitmapkey.entry_num] = set | flag;

    return set | flag;

  }else {

    bmap->entries[bitmapkey.entry_num] = set & (~flag);

    return set & (~flag);

  }
}

int BitMap_read_atIndex(BitMap* bmap, int index){
	if(index < 0 || index > bmap->num_bits) return -1;
	
	BitMapEntryKey entry = BitMap_blockToIndex(index);
	int pos = entry.entry_num;
	int value = (bmap->entries[pos] >> (8-entry.bit_num)) & 0x01;
	if(value == 1) return 1;
	else if (value == 0) return 0;

	return -1;
}