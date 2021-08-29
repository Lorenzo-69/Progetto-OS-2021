#include "disk_driver.h"
#include "simplefs.h"

//Stefano
// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {
    if(fs == NULL || disk == NULL) return NULL;

    fs->disk = disk;

    FirstDirectoryBlock* fdb = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
    if(fdb == NULL){
        return NULL
    }

    // controllare che il blocco sia disponibile
    int res = DiskDriver_readBlock(disk,fdb,0,sizeof(FirstDirectoryBlock));
    if(res == -1){
        free(fdb);
        return NULL;
    }

    DirectoryHandle* dir = (DirectoryHandle*) malloc(sizeof(DirectoryHandle));
    if(dir == NULL) {
        return NULL;
    }

    dir->sfs = fs;
    dir-> dcb = dcb;
    dir->directory = NULL;
    dir->current_block = NULL;
    dir->pos_in_dir = 0;
    dir->pos_in_block = 0;

    return dir;
}

//Lorenzo
// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs);

//Stefano
// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {
    if(d == NULL || filename == NULL) return NULL;

    SimpleFS* fs = d->sfs;
    DiskDriver* disk = fs->disk;
    FirstDirectoryBlock* fdb = d->dcb;

    if(fs == NULL || disk == NULL || fdb == NULL) return NULL;

    // prendere dal disco il blocco libero
    int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
    if(new_block == -1) return NULL;


    // creare primo blocco del file
    FirstFileBlock* fcb = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
    if(fcb == NULL) {
        return NULL;
    }
    fcb->header.pre = -1;
    fcb->header.post = -1;
    fcb->num = 0;

    for(int i=0; i<120; i++) {
        fcb->header.blocks[i] = -1;   // tutti i blocchi vuoti a -1
    }

    // settare FileControlBlock del FirstDirectoryBlock
    fcb->fcb.directory_block = fdb->fcd.block_in_disk;
    fcb->fcb.block_in_disk = new_block; // inserire blocco libero del disco
    strncpy(fcb->fcb.name,filename,128);
    fcb->fcb.size_in_bytes = BLOCK_SIZE;
    fcb->fcb.size_in_blocks = 1;
    fcb->fcb.is_dir = 0;

    // creare blocco successivo
    int free_block = DiskDriver_getFreeBlock(disk,new_block+1);
    if(free_block == -1){
        free(fcb);
        return NULL;
    }

    fcb->header.blocks[0] = free_block;

    FileBlock* file = (FileBlock*) malloc(sizeof(FileBlock));
    if(file == NULL) {
        free(file);
        return NULL;
    }
    
    // settare num
    file->pos = 0;
    file->num = new_block; // inserire blocco libero del disco
    int len = BLOCK_SIZE -sizeof(int) - sizeof(int);
    for(int i=0; i<len ; i++) {
        file->data[i] = -1;
    }

    // scrivere su disco il file
    ret = DiskDriver_writeBlock(disk,fcb,new_block,sizeof(FirstFileBlock));
    if(ret == -1){
        free(fcb);
        free(file);
        return NULL;
    }

    ret = DiskDriver_writeBlock(disk,file,free_block,sizeof(FileBlock));
    if(ret == -1){
        free(fcb);
        free(file);
        return NULL;
    }


    FileHandle* fh = (FileHandle*) malloc(sizeof(FileHandle));
    fh->sfs = fs;
    fh->directory = fdb;
    fh->current_block = NULL;
    fh->pos_in_file = 0;
    fh->fcb = fcb;

    free(file);
    return fh;
}

//Lorenzo
// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d);


//Stefano
// opens a file in the  directory d. The file should be exisiting
FileHandle* SimpleFS_openFile(DirectoryHandle* d, const char* filename){
    if(d == NULL || filename == NULL) return NULL;

    // directory non vuota
    FirstDirectoryBlock* fdb = d->dcb;
    DiskDriver* disk = d->sfs->disk;
    if(fdb->num_entries > 0) {

        FileHandle* fh = (FileHandle*) malloc(sizeof(FileHandle));
        if(fh == NULL) return NULL;

        fh->sfs = d->sfs;
        fh->directory = fdb;
        fh->current_block = NULL;
        fh->pos_in_file = 0;

        FirstFileBlock* f = (FirstFileBlock*) malloc(sizeof(FirstFileBloc));
        if(f == NULL) {
            free(f);
            return NULL;
        }

        // estrarre primo directory block
        DirectoryBlock* dir = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
        if(dir == NULL) {
            free(f);
            free(dir);
            return NULL;
        }

        if(DiskDriver_readBlock(disk, (void*) dir, fdb->header.blocks[0], sizeof(DirectoryBlock)) == -1){
            free(fh);
            free(f);
            free(dir);
            return NULL;
        }

        int len = ((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));
        int found = 0;

        // cerco il file
        while (dir != NULL && !found){
            for( int i=0; i< len; i++){
                if(dir->file_block[i] > 0 && (DiskDriver_readBlock(disk,f,dir->file_blocks[i],sizeof(FirstFileBlock)) != -1)){
                    if(strncmp(f->fcb.name,filename,128) == 0){
                        fh->fcb = f;
                        found = 1;
                        break;
                    }
                }
            }
        }

        if(found) {
            free(dir);
            return fh;
        } else {
            free(fh);
            free(f);
            free(dir);
            return NULL;
        }
        // directory vuota
    } else {
        return NULL
    }
    return NULL;
}

//Lorenzo
// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f);

//Stefano
// writes in the file, at current position for size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes written
int SimpleFS_write(FileHandle* f, void* data, int size){

    FirstFileBlock* ffb = f->fcb;
    DiskDriver* disk = f->sfs->disk;
    int space = BLOCK_SIZE - sizeof(int)- sizeof(int);
    int written = 0;
    int pos = f->pos_in_file;
    int write = size;

    DiskDriver* disk = f->sfs->disk;

    FileBlock* temp = (FileBlock*) malloc(sizeof(FileBlock));
    if(temp == NULL) return -1;

    // indice blocco a cui accedere
    int file_index;
    int index_block = pos - 90*space;
    if(index_block < 0) {
        index_block = 0;
        file_index = pos / (90*space);
        pos = pos - file_index*space;
    } else {
        index_block = index_block / (90*space);
        file_index = ((pos - 90*space)-index_block*(90*space))/space;
        pos = pos - (pos - 90*space)-index_block*(90*space);
    }

    FirstBlockIndex index = ffb->index;

    // vado alla posizione giusta
    for(int i=0; i<index_block; i++){
        if(DiskDriver_readBlock(disk, (void*)&index, index.post, sizeof(BlockIndex)) == -1){
            return -1;
        }
    }

    // scrivere al primo blocco
    if(pos < space) {
        // estrarre file block
        if(DiskDriver_readBlock(disk,(void*) temp, index.blocks[file_index],sizeof(FileBlock) == -1){
            free(temp);
            return -1;
        }
        // basta questo blocco
        if(write <= space-pos) {
            memcpy(temp->data + pos, (char*)data, write);
            written += write;
            if(pos+written > ffb->fcb.written_bytes){
                ffb->fcb.written_bytes = pos + written;
            }
            free(temp);
            return written;
        }
        // non basta questo blocco
        else {
            memcpy(temp->data + pos, (char*) data, space - pos);
            written += space - pos;
            size = size - written;
        }
    } else {
        free(temp);
        return -1;
    }

    FileBlock* corrente = temp;
    int position;

    while(written < size && temp != NULL) {
        if(ffb->fcb.block_in_disk == temp->data) {
            position = create_next_file_block_first(corrente, temp, disk);
        } else {
            position = create_next_file_block(corrente, temp, disk);
        }
        if(position == -1) {
            free(temp);
            return -1;
        }

        if(write <= space - pos) {
            memcpy(temp->data, (char*)data + written, write);
            written += write;
            if(f->pos_in_file+written > ffb->fcb.written_bytes){
                ffb->fcb.written_bytes = f->pos_in_file+written;
            }
            if(DiskDriver_writeBlock(disk, temp, position, sizeof(FileBlock)) == -1){
                free(temp);
                return -1;
            }
            free(temp);
            return written;
        } else {
            memcpy(temp->data, (char*)data+written, space);
            written += space;
            write = size-written;
            if(DiskDriver_writeBlock(disk, temp,position,sizeof(FileBlock)) == -1){
                free(temp);
                return -1;
            }
        }
        corrente = temp;
    }
    free(temp);
    return written;
}

//Lorenzo
// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

//Stefano
// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos) {
    FirstFileBlock* ffb = f->fcb;
    if(pos > ffb->fcb.written_bytes) {
        return -1;
    }
    f->pos_in_file = pos;

    return pos;
}

//Lorenzo
// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

//Stefano
// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname) {
    if(d == NULL || dirname == NULL) return -1;

    DiskDriver* disk = d->sfs->disk;
    FirstDirectoryBlock* fdb = d->dcb;

    // creare blocco nuovo
    int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
    if(new_block == -1) return -1;

    FirstDirectoryBlock* dir = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
    if(dir == NULL) return -1;

    dir->fcb.block_in_disk = new_block;
    dir->fcb.directory_block = fdb->fcb.block_in_disk;
    dir->fcb.is_dir = 1;
    strncpy(dir->fcb.name, dirname , 128);
    dir->fcb.size_in_blocks = 0;
    dir->fcb.size_in_bytes = 0;
    dir->fcb.written_bytes = 0;
    dir->num_entries = 0;

    FirstBlockIndex fbi;
    fbi.pre = -1;
    fbi.post = -1;
    for( int i=0; i<90; i++) {
        fbi.blocks[i] = -1;
    }

    DirectoryBlock block;
    block.index = new_block;
    block.pos = 0;

    for( int i=0;i<((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));i++){
        block.file_blocks[i] = 0;
    }

    int free_block = DiskDriver_getFreeBlock(disk, new_block+1);
    if(free_block == -1){
        return -1;
    }

    fbi.blocks[0] = free_block;

    dir->header = fbi; // settare FirstBlockIndex

    //scrivere su disco
    if(DiskDriver_writeBlock(disk,dir,new_block,sizeof(FirstDirectoryBlock)) == -1){
        return -1;
    }

    if(DiskDriver_writeBlock(disk,dir,free_block,sizeof(DirectoryBlock)) == -1){
        return -1;
    }


    free(dir);
    return 0;
    
}

//Lorenzo
// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename);


int create_next_file_block(FileBlock* corrente, FileBlock* new, DiskDriver* disk){
	int index_corrente = corrente->pos;
	
	if(index_corrente + 1 == 126){
		BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
        if(DiskDriver_readBlock(disk,index,corrente->num, sizeof(BlockIndex)) == -1){
            free(index);
            index = NULL;
        }
		if(index == NULL) return -1;

		int new_block = DiskDriver_getFreeBlock(disk, index->blocks[index_corrente]);
		if(new_block == -1){
			free(index);
			return -1;
		}
		
		int block = DiskDriver_getFreeBlock(disk, new_index + 1);
		if(block == -1){
			free(index);
			return -1;
		}
		
		int index_block = corrente->num;
		
		index->next = new_block;

		BlockIndex new_index;
        new_index.pre = index_block;
        new_index.post = -1;
        for(int i=0; i<126; i++) new_index.blocks[i] = -1;
		new_index.blocks[0] = block;
		if(DiskDriver_writeBlock(disk, &new_index, new_block, sizeof(BlockIndex)) == -1){
			free(index);
			return -1;
		}

		new->num = new_block;
		new->pos = 0;
		
		free(index);
		
		return block;
		
	}
	else{
		
		BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
        if(DiskDriver_readBlock(disk,index,corrente->num, sizeof(BlockIndex)) == -1){
            free(index);
            index = NULL;
        }
		if(index == NULL) return -1;
		
		int index_block = corrente -> num; 

		new->num = index_block;
		new->pos = index_corrente + 1;
		
		int block = DiskDriver_getFreeBlock(disk, index->blocks[index_corrente]);
		if(block == -1){
			free(index);
			return -1;
		}
		
		index->blocks[new->pos] = block;
		
		free(index);
		
		return block;
	}

}



int create_next_file_block_first(FileBlock* corrente, FileBlock* new, DiskDriver* disk) {
    int index_corrente = corrente->pos;

    if(index_corrente + 1 == MAX_BLOCKS_FIRST){
        FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
        if(DiskDriver_readBlock(disk, index, corrente->num, sizeof(FirstBlockIndex)) == -1) {
            free(index);
            index = NULL;
        }

        if(index == NULL) return -1;
    
        int new_block = DiskDriver_getFreeBlock(disk, index->blocks[index_corrente]);
        if(new_block == -1){
            free(index);
            return -1;
        }

        int block = DiskDriver_getFreeBlock(disk, new_block+1);
        if(block == -1){
            free(index);
            return -1;
        }

        int index_block = corrente->num ;
        index->next = new_block;

        BlockIndex new_index;
        index->pre = index_block;
        index->post = -1;
        for( int i=0; i<126;i++) index->blocks[i] = -1;

        new_index.blocks[0] = block;
        if(DiskDriver_writeBlock(disk, &new_index, new_index, sizeof(BlockIndex)) == -1){
            free(index);
            return -1;
        }

        new->num = new_index;
        new->pos = 0;
        free(index);
        return block;
    } else {
        FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
        if(DiskDriver_readBlock(disk, index, corrente->num, sizeof(FirstBlockIndex)) == -1) {
            free(index);
            index = NULL;
        }
        if(index == NULL) return -1;

        int index_block = corrente->num;

        new->num=index_block;
        new->pos=index_corrente +1;

        int block = DiskDriver_getFreeBlock(disk, index->blocks[index_corrente]);
        if(block == -1) {
            free(index);
            return -1;
        }

        index->blocks[new->pos] = block;

        free(index);
        return block;
    }

    
}