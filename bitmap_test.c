#include <stdlib.h>
#include <stdio.h>
#include "bitmap.h"


int main(int argc, char** argv){

    int block0 = BitMap_indexToBlock(0,0);
	if(block0 != 0){
		fprintf(stderr, "Error: wrong block\n");
		return -1;
	}
	printf("Block: %i | Expected: 0\n\n", block0);

    printf("Byte:1 - Bit:0 to Block -->\n");
	int block8 = BitMap_indexToBlock(1,0);
	if(block8 != 8){
		fprintf(stderr, "Error: wrong block\n");
		return -1;
	}
	printf("Block: %i | Expected: 8\n\n", block8);
}