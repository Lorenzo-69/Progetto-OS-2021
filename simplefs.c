#include "disk_driver.h"
#include "simplefs.h"
#include <string.h>
#include <stdio.h>

//Stefano
// initializes a file system on an already made disk
// returns a handle to the top level directory stored in the first block
DirectoryHandle* SimpleFS_init(SimpleFS* fs, DiskDriver* disk) {
    if(fs == NULL || disk == NULL) return NULL;
	
	fs->disk = disk;
	FirstDirectoryBlock* fdb = (FirstDirectoryBlock*)malloc(sizeof(FirstDirectoryBlock));
	if(fdb == NULL){
		fprintf(stderr, "Errore creazione FirstDIrectoryBlock SimpleFs_Init.\n");
		return NULL;
	}	
	
	int res = DiskDriver_readBlock(disk,fdb,0, sizeof(FirstDirectoryBlock));
	if(res == 0){ 
		fprintf(stderr,"Errore: readBlock del FirstDirectoryBlock SImpleFS_init\n");
		free(fdb);
		return NULL;
	}			
	
	DirectoryHandle* dir = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));
	if(dir == NULL){
		fprintf(stderr, "Errore creazione directory_handle SimpleFS_init.\n");
		return NULL;
	}

	if(fs->disk->header->first_free_block == 0){
		SimpleFS_format(fs);
	}
	
	dir->sfs = fs;
	dir->dcb = fdb;
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
void SimpleFS_format(SimpleFS* fs){
    if(fs == NULL || fs->disk == NULL) {
        fprintf(stderr,"errore input format");
        return;
    }
    //pulizia disco
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    fs->disk->header->first_free_block = 0;
    fs->disk->bitmap_data = (char *) memset((void*)fs->disk->bitmap_data,0,BLOCK_SIZE);

    FirstDirectoryBlock * fdb = (FirstDirectoryBlock *)malloc(sizeof(FirstDirectoryBlock));
    fdb->num_entries = 0;
    fdb->header.pre = -1;
    fdb->header.post = -1;
    for (int i=0; i<90; i++){
        fdb->header.blocks[i] = -1;
    }
    fdb->fcb.is_dir = 1;
    fdb->fcb.size_in_blocks = 0;
    fdb->fcb.size_in_bytes = 0;
    strcpy(fdb->fcb.name,"/");
    fdb->fcb.directory_block = -1;
    fdb->fcb.block_in_disk = 0;

    DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
    db->index = 0;
    db->pos = 0;
    for (int i=0; i<(BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int); i++){
        db->file_blocks[i] = -1;
    }
    
    int block = DiskDriver_getFreeBlock(fs->disk,1);

    if(block == -1){
        fprintf(stderr,"errore getFreeBlock");
        return;
    }    
    fdb->header.blocks[0] = block;

    int ret = DiskDriver_writeBlock(fs->disk,db,block,sizeof(DirectoryBlock));
    if(ret == -1){
        fprintf(stderr,"errore writeblock");
        return;
    }

    ret = DiskDriver_writeBlock(fs->disk,fdb,0,sizeof(FirstDirectoryBlock));
     if(ret == -1){
        fprintf(stderr,"errore writeblock");
        return;
    }
    return;
}

//Stefano
// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {
    
    if(d == NULL || filename == NULL) {
        printf("\nfilename == null");
        return NULL;
    }

    int ret;
    SimpleFS* fs = d->sfs;
    DiskDriver* disk = fs->disk;
    FirstDirectoryBlock* fdb = d->dcb;

    if(disk->header->free_blocks < 1){
        printf("\n non ci sono blocchi liberi");
        return NULL;
    }

    if(fs == NULL || disk == NULL || fdb == NULL) {
        printf("\n fs == null");
        return NULL;
    }

    // prendere dal disco il blocco libero
    int new_block = DiskDriver_getFreeBlock(disk,disk->header->first_free_block);
    if(new_block == -1) {
        printf("\n getfreeblock 1 error");
        return NULL;
    }


    // creare primo blocco del file
    FirstFileBlock* fcb = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
    if(fcb == NULL) {
        printf("\n fcb == null");
        return NULL;
    }
    fcb->header.pre = -1;
    fcb->header.post = -1;
    fcb->num = 0;

    for(int i=0; i<120; i++) {
        fcb->header.blocks[i] = -1;   // tutti i blocchi vuoti a -1
    }

    // settare FileControlBlock del FirstDirectoryBlock
    fcb->fcb.directory_block = fdb->fcb.block_in_disk;
    fcb->fcb.block_in_disk = new_block; // inserire blocco libero del disco
    strncpy(fcb->fcb.name,filename,128);
    fcb->fcb.written_bytes = 0;
    printf("\n %s",fcb->fcb.name);
    fcb->fcb.size_in_bytes = BLOCK_SIZE;
    fcb->fcb.size_in_blocks = 1;
    fcb->fcb.is_dir = 0;

     // creare blocco successivo
    int free_block = DiskDriver_getFreeBlock(disk,new_block+1);
    if(free_block == -1){
        printf("\n getfreeblock 2 error");
        free(fcb);
        return NULL;
    }

    fcb->header.blocks[0] = free_block;

    FileBlock* file = (FileBlock*) malloc(sizeof(FileBlock));
    if(file == NULL) {
        printf("\n file == null");
        free(file);
        return NULL;
    }
    
    // settare num
    file->pos = 0;
    file->num = new_block; // inserire blocco libero del disco
    int len = BLOCK_SIZE -sizeof(int) - sizeof(int);
    // memset(file->data, -1 , len);
    for(int i=0; i<len ; i++){
        file->data[i] = -1;
    }

    // scrivere su disco il file
    ret = DiskDriver_writeBlock(disk,fcb,new_block,sizeof(FirstFileBlock));
    if(ret == -1){
        printf("\n writeblock 1 error");
        free(fcb);
        free(file);
        return NULL;
    }

    ret = DiskDriver_writeBlock(disk,file,free_block,sizeof(FileBlock));
    if(ret == -1){
        printf("\n writeblock 2 error");
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

    d->dcb->num_entries++;

    free(file);
    return fh;
}

//Lorenzo
// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d){

    if(d==NULL){
        fprintf(stderr,"directory handle non valido");
        return -1;
    }
    FirstDirectoryBlock* fdb = d->dcb;
    if(fdb->num_entries == 0){
        fprintf(stderr,"directory vuota");
        return 0;
    }

    if(fdb->num_entries == -1){
        fprintf(stderr,"errore");
        return -1;
    }


    if(names == NULL){
        /*names = (char**)malloc(sizeof(char*)* 128); //da capire grandezza names
        for(int i=0; i<fdb->num_entries; i++){
            names[i] = (char*)malloc(sizeof(char)*128);
        }*/
        printf("errore lista nomi non allocata\n");
        return -1;
    }

    int idx_names = 0;
    DirectoryBlock * db = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
    if (db == NULL) return -1;

    FirstFileBlock* ffb = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
    if (ffb == NULL) return -1;

    int ret = DiskDriver_readBlock(d->sfs->disk,db,d->dcb->header.blocks[0], sizeof(DirectoryBlock));
    if (ret == -1){
        return -1;
    }
    int dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);

    for (int i = 0; i<dim; i++) {
        //printf("\n primo for");
        //printf("\n file_blocks %d", db->file_blocks[i]);
        if(db->file_blocks[i] != -1) {
            //printf("\n primo if");
            if(DiskDriver_readBlock(d->sfs->disk, ffb, db->file_blocks[i],sizeof(FirstFileBlock)) != -1){
                //printf("\n name %s", ffb->fcb.name);
                //printf("\n secondo if");
                //printf("\n %s",names[idx_names]);
                strncpy(names[idx_names],ffb->fcb.name,128);
                //printf("\n %s", ffb->fcb.name);
                //idx_names++;
            }
        }
    }

    if(fdb->num_entries > 1) {
        for(int i = 1; i<fdb->num_entries; i++) {
            if(d->dcb->header.blocks[i] > 0) {
                if(DiskDriver_readBlock(d->sfs->disk,db,d->dcb->header.blocks[i],sizeof(DirectoryBlock)) != -1){
                    for(int j=0; j<dim; j++){
                        if(db->file_blocks[j] > 0) {
                            if(DiskDriver_readBlock(d->sfs->disk,ffb,db->file_blocks[j],sizeof(FirstFileBlock)) != -1){
                            strncpy(names[idx_names],ffb->fcb.name,128);
                            printf("\n %s", ffb->fcb.name);
                            //idx_names++;
                            }
                        } 
                    }
                } 
            }
        }
    }
    

    /*creare directory block con malloc senza inizializzare campi
    Diskdriver_ReadBlock con  (d->sfs->disk, directory block, d->dcb->header.blocks[0])
    for su (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int) e controllo se directory block->fileblocks[i] != -1
    dichiaro first file block
    readBlock con (d->sfs->disk, first file block, directory block->fileblocks[i])
    strcpy di first file block -> fcb.name in names[indice]
    prendo fdb->numentries e controllo se > 1
    se si controllare i successivi e fare stessi controlli
    */
    /*int ret;
    if(fdb->num_entries == 1){
        printf("\n ciao 3");
        idx_names = 0;
        DirectoryBlock * db = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
        ret = DiskDriver_readBlock(d->sfs->disk,db,d->dcb->header.blocks[0],sizeof(DirectoryBlock));
        if(ret==-1){
            fprintf(stderr,"errore readblock 1 in ReadDir");
            return -1;
        }
        printf("\n ciao 4");
        FirstFileBlock * ffb = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
        for(int i=0; i<(BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int); i++){
            if(db->file_blocks[i] == -1) continue;
            ret = DiskDriver_readBlock(d->sfs->disk,ffb,db->file_blocks[i],sizeof(FirstFileBlock));
            if(ret == -1){
                fprintf(stderr,"errore readblock 2 in ReadDir");
                return -1;
            }
            strncpy(names[idx_names],ffb->fcb.name,128);
            idx_names++;
        //il campo dati file_blocks all'interno del directory block corrisponde ai primi blocchi di ogni file nella directory
        }
        printf("\n ciao 5");
    }else{
        idx_names = 0;
        DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
        FirstFileBlock * ffb = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
        printf("\n ciao 6");
        for(int i=0; i<d->dcb->num_entries; i++){
            if(d->dcb->header.blocks[i] == -1) continue;
            printf("\n ciao readblokc");
            ret = DiskDriver_readBlock(d->sfs->disk,db,d->dcb->header.blocks[i],sizeof(DirectoryBlock));
            printf("la read torna %d\n",ret);
            if(ret==-1){
                fprintf(stderr,"errore readblock 3 in ReadDir");
                return -1;
            }
            printf("\n ciao 7");
            for(int j=0; j<(BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int); j++){
                if(db->file_blocks[j] == -1) continue;
                ret = DiskDriver_readBlock(d->sfs->disk,ffb,db->file_blocks[j],sizeof(FirstFileBlock));
                if(ret == -1){
                    fprintf(stderr,"errore readblock 4 in ReadDir");
                    return -1;
                }
                strncpy(names[idx_names],ffb->fcb.name,128);
                idx_names++;
            }
            printf("\n ciao 8");
        }
        printf("\n ciao 9");
    }
    printf("\n ciao finale");*/
    return 0;
} 


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

        FirstFileBlock* f = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
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

        if(DiskDriver_readBlock(disk, (void*) dir, fdb->header.blocks[0],sizeof(DirectoryBlock)) == -1){
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
                if(dir->file_blocks[i] > 0 && (DiskDriver_readBlock(disk,f,dir->file_blocks[i],sizeof(FirstFileBlock)) != -1)){
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
        return NULL;
    }
    return NULL;
}

//Lorenzo
// closes a file handle (destroyes it)
int SimpleFS_close(FileHandle* f){
    if(f==NULL){
        fprintf(stderr,"fileHandler non valido");
        return -1;
    }
    free(f->fcb);
    free(f);
    return 0;
}

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

    FirstBlockIndex index = ffb->header;

    // vado alla posizione giusta
    for(int i=0; i<index_block; i++){
        if(DiskDriver_readBlock(disk, (void*)&index, index.post, sizeof(FirstBlockIndex)) == -1){
            return -1;
        }
    }

    // scrivere al primo blocco
    if(pos < space) {
        // estrarre file block
        if(DiskDriver_readBlock(disk,(void*) temp, index.blocks[file_index],sizeof(FileBlock)) == -1){
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
        if(ffb->fcb.block_in_disk == temp->data[0]) {
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
            if(DiskDriver_writeBlock(disk, temp, position,sizeof(FileBlock)) == -1){
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
int SimpleFS_read(FileHandle* f, void* data, int size){
    if(f==NULL || size <=0){
        fprintf(stderr,"errore input non valido");
    }
    
    FileBlock * fb = (FileBlock*) malloc(sizeof(FileBlock));
    DiskDriver_readBlock(f->sfs->disk, fb, f->fcb->header.blocks[0],sizeof(FileBlock));
    if(size <= strlen(fb->data)){
        if(data == NULL){
            data = (char*) malloc(sizeof(char)* size);
        }
        strcpy(data,fb->data);
        return strlen(data);
    }else{
        if(data == NULL) data = (char*) malloc(sizeof(char)* size);
        int block_idx = 1;
        while(size >= strlen(fb->data)){
            strcat(data,fb->data);
            DiskDriver_readBlock(f->sfs->disk,fb,f->fcb->header.blocks[block_idx],sizeof(FileBlock));
            block_idx++;
        }
    }
    return strlen(data);
}

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
int SimpleFS_changeDir(DirectoryHandle* d, char* dirname){
     /*TODO
     se dirname == ".." cambio il directory handler passato in input con il directory handler di d->directory
     cambio il directory handler passato in input con il directory handler della directory dirname
     sfs rimane lo stesso, cambia d->directory, d->dcb, d->current_block, d->pos_in_dir, d->pos_in_block.
     */
     if(dirname == NULL || d == NULL){
         fprintf(stderr,"input changeDir non valido");
         return -1;
     }
     if(strcmp(dirname,"..") == 0){
         if(d->directory == NULL){
             fprintf(stderr,"errore non esiste directory padre");
             return -1;
         }
         FirstDirectoryBlock * new_parent = malloc(sizeof(FirstDirectoryBlock));
         FirstDirectoryBlock * new_fdb;
         DirectoryBlock * new_current_block = malloc(sizeof(DirectoryBlock));
         int pos_in_dir;
         int pos_in_block;

        if(DiskDriver_readBlock(d->sfs->disk,new_parent,d->directory->header.blocks[d->directory->fcb.directory_block],sizeof(FirstDirectoryBlock)) == -1){
            fprintf(stderr,"errore lettura blocco padre (..)");
            return -1;
        }
        new_fdb = d->directory;
        if(DiskDriver_readBlock(d->sfs->disk,new_current_block,new_fdb->header.blocks[1],sizeof(DirectoryBlock)) == -1){
            fprintf(stderr,"errore lettura blocco corrente (..)");
            return -1;
        }
        pos_in_dir = 1;
        pos_in_block = 1;
        d->dcb = new_fdb;
        d->directory = new_parent;
        d->current_block = new_current_block;
        d->pos_in_dir = pos_in_dir;
        d->pos_in_block = pos_in_block;
     }
     else{
         /*TODO
         cercare nella directory corrente la directory con nome dirname e aggiornare directory-handler
         */
         FirstDirectoryBlock* fdb_ctrl = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
         DirectoryBlock* db = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
         for(int i=0; d->dcb->num_entries; i++){
             DiskDriver_readBlock(d->sfs->disk,db,d->dcb->header.blocks[i],sizeof(DirectoryBlock));
             for(int j=0; j<(BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int); i++){
                 if(db->file_blocks[j] == -1) continue;
                 DiskDriver_readBlock(d->sfs->disk,fdb_ctrl,db->file_blocks[j],sizeof(FirstDirectoryBlock));
                 //controllo se nome corrisponde e se è una directory
                 if(strcmp(dirname,fdb_ctrl->fcb.name) == 0 && fdb_ctrl->fcb.is_dir == 1){
                     d->directory = d->dcb;
                     d->dcb = fdb_ctrl;
                     d->current_block = db;
                     d->pos_in_dir = db->pos;
                     d->pos_in_block = db->index;
                     return 0;
                    }
                }

            }
        }
        return 0;
 }

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

    if(DiskDriver_writeBlock(disk,dir,free_block, sizeof(FirstDirectoryBlock)) == -1){
        return -1;
    }


    free(dir);
    return 0;
    
}

//Lorenzo
// removes the file in the current directory
// returns -1 on failure 0 on success
// if a directory, it removes recursively all contained files
int SimpleFS_remove(DirectoryHandle* d, char* filename){
    if(d==NULL || filename == NULL) return -1;
    char** dir_list = NULL;
    int ret;
    ret = SimpleFS_readDir(dir_list,d);
    if(ret == -1) return -1;
    int check = 0;
    for(int i=0; i<128;i++){
        if(dir_list != NULL && strcmp(filename,dir_list[i]) == 0){
            check = 1;
            break;
        }
    }
    if(check == 0) return -1; //file non presente nella directory

    FirstDirectoryBlock * start = d->dcb;
    DirectoryBlock * current_db;
    FirstFileBlock * current_ffb;
    FileBlock * current_fb;

    for(int i=0; i<d->dcb->num_entries; i++){
        DiskDriver_readBlock(d->sfs->disk, current_db, start->header.blocks[i],sizeof(DirectoryBlock));
        for(int j=0; j<(BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int); j++){
            DiskDriver_readBlock(d->sfs->disk,current_ffb,current_db->file_blocks[j],sizeof(FirstFileBlock));
            if(strcmp(filename,current_ffb->fcb.name) == 0){ //check name and if dir
                if(current_ffb->fcb.is_dir == 0){ // se è un file
                    //eliminare file
                    for(int k=0; k<sizeof(current_ffb->header.blocks)/sizeof(int); k++){
                        DiskDriver_freeBlock(d->sfs->disk,current_ffb->header.blocks[k]);
                    }
                    DiskDriver_freeBlock(d->sfs->disk,current_db->file_blocks[j]);
                    return 0;
                }else if(current_ffb->fcb.is_dir == 1){ // se è una directory
                    //entrare dentro la directory e fare chiamata ricorsiva con il directory handler aggiornato
                    DirectoryHandle * d1 = d;
                    SimpleFS_changeDir(d1, filename);
                    SimpleFS_remove(d1,filename);
                }
                return 0;
            }
        }

    }


}


int create_next_file_block(FileBlock* corrente, FileBlock* new, DiskDriver* disk){
	int index_corrente = corrente->pos;
	
	if(index_corrente + 1 == 126){
		BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
        if(DiskDriver_readBlock(disk,index,corrente->num,sizeof(BlockIndex)) == -1){
            free(index);
            index = NULL;
        }
		if(index == NULL) return -1;

		int new_block = DiskDriver_getFreeBlock(disk, index->blocks[index_corrente]);
		if(new_block == -1){
			free(index);
			return -1;
		}
		
		int block = DiskDriver_getFreeBlock(disk, new_block + 1);
		if(block == -1){
			free(index);
			return -1;
		}
		
		int index_block = corrente->num;
		
		index->post = new_block;

		BlockIndex new_index;
        new_index.pre = index_block;
        new_index.post = -1;
        for(int i=0; i<126; i++) new_index.blocks[i] = -1;
		new_index.blocks[0] = block;
		if(DiskDriver_writeBlock(disk, &new_index, new_block,sizeof(BlockIndex)) == -1){
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
        if(DiskDriver_readBlock(disk,index,corrente->num,sizeof(BlockIndex)) == -1){
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

    if(index_corrente + 1 == 90){
        FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
        if(DiskDriver_readBlock(disk, index, corrente->num,sizeof(FirstBlockIndex)) == -1) {
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
        index->post = new_block;

        BlockIndex new_index;
        index->pre = index_block;
        index->post = -1;
        for( int i=0; i<126;i++) index->blocks[i] = -1;

        new_index.blocks[0] = block;
        if(DiskDriver_writeBlock(disk, &new_index, new_block,sizeof(BlockIndex)) == -1){
            free(index);
            return -1;
        }

        new->num = new_block;
        new->pos = 0;
        free(index);
        return block;
    } else {
        FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
        if(DiskDriver_readBlock(disk, index, corrente->num,sizeof(FirstBlockIndex)) == -1) {
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
