#pragma once
#include <stdint.h>
typedef struct{
  int num_bits;
  char* entries;
}  BitMap;

typedef struct {
  int entry_num;
  char bit_num;
} BitMapEntryKey;

//Lorenzo
// converts a block index to an index in the array,
// and a char that indicates the offset of the bit inside the array
BitMapEntryKey BitMap_blockToIndex(int num);

//Stefano
// converts a bit to a linear index
int BitMap_indexToBlock(int entry, uint8_t bit_num);

//Lorenzo
// returns the index of the first bit having status "status"
// in the bitmap bmap, and starts looking from position start
int BitMap_get(BitMap* bmap, int start, int status);

//Stefano
// sets the bit at index pos in bmap to status
int BitMap_set(BitMap* bmap, int pos, int status);
