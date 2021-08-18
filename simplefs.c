#include "bitmap.h"
#include "disk_driver.h"
#include "simplefs.h"

//Stefano
// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {
    if(fs == NULL || disk == NULL) return NULL;

    fs->disk = disk;

    // controllare che il blocco sia disponibile

    FirstDirectoryBlock* dcb = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
    if(dcb == NULL){
        return NULL
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

    // prendere dal disco il blocco libero

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
    fcb->fcb.directory_block = d->dcb->fcd.block_in_disk;
    fcb->fcb.block_in_disk = ; // inserire blocco libero del disco
    strncpy(fcb->fcb.name,filename,128);
    fcb->fcb.size_in_bytes = BLOCK_SIZE;
    fcb->fcb.size_in_blocks = 1;
    fcb->fcb.is_dir = 0;

    FileBlock* file = (FileBlock*) malloc(sizeof(FileBlock));
    if(file == NULL) {
        free(file);
        return NULL;
    }
    
    // settare num
    file->pos = 0;
    file->num = ; // inserire blocco libero del disco
    int len = BLOCK_SIZE -sizeof(int) - sizeof(int);
    for(int i=0; i<len ; i++) {
        file->data[i] = -1;
    }

    // scrivere su disco


    FileHandle* fh = (FileHandle*) malloc(sizeof(FileHandle));
    fh->sfs = d->sfs;
    fh->directory = d->dcb;
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
    if(d->dcb->num_entries > 0) {

        FileHandle* fh = (FileHandle*) malloc(sizeof(FileHandle));
        if(fh == NULL) return NULL;

        fh->sfs = d->sfs;
        fh->directory = d->dcb;
        fh->current_block = NULL;
        fh->pos_in_file = 0;

        FirstFileBlock* f = (FirstFileBlock*) malloc(sizeof(FirstFileBloc));
        if(f == NULL) {
            free(f);
            return NULL;
        }

        DirectoryBlock* dir = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
        if(dir == NULL) {
            free(f);
            free(dir);
            return NULL;
        }

        int len = ((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));

        while (dir != NULL ){
            for( int i=0; i< len; i++){
                if(dir->file_block[i] > 0 ){
                    if(strncmp(f->fcb.name,filename,128) == 0){
                        fh->fcb = f;
                        break;
                    }
                }
            }
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
int SimpleFS_write(FileHandle* f, void* data, int size);

//Lorenzo
// writes in the file, at current position size bytes stored in data
// overwriting and allocating new space if necessary
// returns the number of bytes read
int SimpleFS_read(FileHandle* f, void* data, int size);

//Stefano
// returns the number of bytes read (moving the current pointer to pos)
// returns pos on success
// -1 on error (file too short)
int SimpleFS_seek(FileHandle* f, int pos);

//Lorenzo
// seeks for a directory in d. If dirname is equal to ".." it goes one level up
// 0 on success, negative value on error
// it does side effect on the provided handle
 int SimpleFS_changeDir(DirectoryHandle* d, char* dirname);

//Stefano
// creates a new directory in the current one (stored in fs->current_directory_block)
// 0 on success
// -1 on error
int SimpleFS_mkDir(DirectoryHandle* d, char* dirname);

//Lorenzo
// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(SimpleFS* fs, char* filename);
