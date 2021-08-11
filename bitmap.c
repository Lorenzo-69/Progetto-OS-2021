#include bitmap.h

BitMapEntryKey BitMap_blockToIndex(int num) {
	int idx = num/8;
	char off = num%8;
	BitMapEntryKey entry = {
		.entry_num = idx;
		.bit_num = off;
	}
	return entry;
}

int BitMap_indexToBlock(int entry, uint8_t bit_num){
  return 0;
}

int BitMap_get(BitMap* bmap, int start, int status){
	if(start < 0 || start > bmap->num_bits) return -1;
	
	for(int i=start; i<bmap->num_bits; i++){
		BitMapEntryKey entry = BitMap_blockToIndex(i);
		int pos = entry->entry_num;
		int value = (bmap->entries[pos] >> (8-entry->bit_num)) & 0x01;
		//oppure value = bmap->entries[pos] << entry->bit_num & 0x80;
		if(value == status) return i;
	}
	return -1;
	
}

int BitMap_set(BitMap* bmap, int pos, int status){
  return 0;
}
