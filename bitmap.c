#include "bitmap.h"
#include <stdio.h>

//Lorenzo
BitMapEntryKey BitMap_blockToIndex(int num) {
	// calcolo l'indice della entry
	int idx = num/8;
	// calcolo offset all'interno della entry
	char off = num%8;
	BitMapEntryKey entry;
	entry.entry_num = idx;
	entry.bit_num = off;
	return entry;
}

// Stefano
int BitMap_indexToBlock(int entry, uint8_t bit_num){

  if(bit_num < 0 || entry < 0) return -1;
  int index = ( entry*8 ) + bit_num;
  return index; 

}

//Lorenzo
int BitMap_get(BitMap* bmap, int start, int status){
	// controllo che start sia nella bitmap
	if(start < 0 || start > bmap->num_bits) return -1;

	int i;
	
	// per ogni bit verifico
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
	// se sforiamo ritorno -1 perchè non è stato trovato
	return -1;
	
}

//Stefano
int BitMap_set(BitMap* bmap, int pos, int status){

	// controllo che pos sia nella bitmap
	if(pos < 0 || pos > bmap->num_bits) return -1;

	// prendo la key nella posizione
	BitMapEntryKey bitmapkey = BitMap_blockToIndex(pos);

	uint8_t val = 1 << (7 - bitmapkey.bit_num);
	// utilizzo l'OR se è 1, l'AND con la negazione se 0
	if(status){
		bmap->entries[bitmapkey.entry_num] |= val;
  	}else{
   		bmap->entries[bitmapkey.entry_num] &= ~(val);
    }
    return status;
}
