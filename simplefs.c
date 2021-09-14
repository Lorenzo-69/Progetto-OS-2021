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
	FirstDirectoryBlock* first_directory_block = (FirstDirectoryBlock*)calloc(1,sizeof(FirstDirectoryBlock));
	if(first_directory_block == NULL){
		fprintf(stderr, "Errore creazione FirstDIrectoryBlock.\n");
		free(first_directory_block);
		return NULL;
	}	
	
	int res = DiskDriver_readBlock(disk,first_directory_block,0, sizeof(FirstDirectoryBlock));
	if(res == 0){ 
		free(first_directory_block);
		return NULL;
	};				
	
	DirectoryHandle* directory_handle = (DirectoryHandle*)malloc(sizeof(DirectoryHandle));
	if(directory_handle == NULL){
		fprintf(stderr, "Errore creazione directory_handle.\n");
		free(first_directory_block);
		return NULL;
	}

	if(fs->disk->header->first_free_block == 0){
		SimpleFS_format(fs);
	}
	
	directory_handle->sfs = fs;
	directory_handle->dcb = first_directory_block;
	directory_handle->directory = NULL;
	directory_handle->current_block = NULL;
	directory_handle->pos_in_dir = 0;
	directory_handle->pos_in_block = 0;

	
	return directory_handle;
}

//Lorenzo
// creates the inital structures, the top level directory
// has name "/" and its control block is in the first position
// it also clears the bitmap of occupied blocks on the disk
// the current_directory_block is cached in the SimpleFS struct
// and set to the top level directory
void SimpleFS_format(SimpleFS* fs){
    if(fs == NULL || fs->disk == NULL) {
        fprintf(stderr,"Errore: input SimpleFS_format");
        return;
    }
    //pulizia disco
    fs->disk->header->free_blocks = fs->disk->header->num_blocks;
    fs->disk->header->first_free_block = 0;
    fs->disk->bitmap_data = (char *) memset((void*)fs->disk->bitmap_data,0,BLOCK_SIZE);

	int size = fs->disk->header->bitmap_entries;
    FirstDirectoryBlock  fdb = {0};
	fdb.fcb.is_dir = 1;
    fdb.fcb.size_in_blocks = 0;
    fdb.fcb.size_in_bytes = 0;
    strcpy(fdb.fcb.name,"/");
    fdb.fcb.directory_block = -1;
    fdb.fcb.block_in_disk = 0;
	memset(fs->disk->bitmap_data,'\0',size);



    fdb.num_entries = 0;
    fdb.header.pre = -1;
    fdb.header.post = -1;
    for (int i=0; i<87; i++){
        fdb.header.blocks[i] = -1;
    }

    DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
    db->index = 0;
    db->pos = 0;
    for (int i=0; i<(BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int); i++){
        db->file_blocks[i] = 0;
    }
    
    int block = DiskDriver_getFreeBlock(fs->disk,1);

    if(block == -1){
        fprintf(stderr,"Errore: getfreeblock SImpleFS_format");
        return;
    }    
    fdb.header.blocks[0] = block;

	if(DiskDriver_writeBlock(fs->disk,&fdb,0,sizeof(FirstDirectoryBlock)) != -1){
		if(DiskDriver_writeBlock(fs->disk,db,block,sizeof(DirectoryBlock)) != -1){
			free(db);
			return;
		} else {
			fprintf(stderr,"Errore: writeBlock 2 SimpleFS_format");
			free(db);
			return;
		}
	} else {
		fprintf(stderr,"Errore: writeBlock 1 SimpleFS_format");
		free(db);
		return;
	}
}

//Stefano
// creates an empty file in the directory d
// returns null on error (file existing, no free blocks)
// an empty file consists only of a block of type FirstBlock
FileHandle* SimpleFS_createFile(DirectoryHandle* d, const char* filename) {
    
    if(d == NULL || filename == NULL) {
        fprintf(stderr,"Errore: Input non validi SimpleFS_createFile\n");
        return NULL;
    }

    SimpleFS* fs = d->sfs;
    DiskDriver* disk = fs->disk;
    FirstDirectoryBlock* fdb = d->dcb;

    if(disk->header->free_blocks < 1){
        fprintf(stderr,"Errore: non ci sono blocchi liberi SimpleFS_createFile\n");
        return NULL;
    }

    // prendere dal disco il blocco libero
    int new_block = DiskDriver_getFreeBlock(fs->disk,fs->disk->header->first_free_block);
    if(new_block == -1) {
        fprintf(stderr,"Errore: getfreeblock 1 error SimpleFS_createFile");
        return NULL;
    }


    // creare primo blocco del file
    FirstFileBlock* fcb = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
    if(fcb == NULL) {
		free(fcb);
        return NULL;
    }
    fcb->header.pre = -1;
    fcb->header.post = -1;
    fcb->num = 0;

    for(int i=0; i<126; i++) {
        fcb->header.blocks[i] = -1;   // tutti i blocchi vuoti a -1
    }

    // settare FileControlBlock del FirstDirectoryBlock
    fcb->fcb.directory_block = fdb->fcb.block_in_disk;
    fcb->fcb.block_in_disk = new_block; // inserire blocco libero del disco
    strncpy(fcb->fcb.name,filename,128);
    fcb->fcb.written_bytes = 0;
    fcb->fcb.size_in_bytes = BLOCK_SIZE;
    fcb->fcb.size_in_blocks = 1;
    fcb->fcb.is_dir = 0;

     // creare blocco successivo
	int free_block = DiskDriver_getFreeBlock(disk,new_block+1);
	if(free_block == -1){
		fprintf(stderr,"Errore: getFreeBlock SImpleFS_createFile\n");
		free(fcb);
		return NULL;
	}
    fcb->header.blocks[0] = free_block;
	if(fcb->header.blocks[0] == -1){
		fprintf(stderr,"Errore: getfreeblock 2 SimpleFS_createFile");
        free(fcb);
        return NULL;
	}

    FileBlock* file = (FileBlock*) malloc(sizeof(FileBlock));
    if(file == NULL) {
        printf("\n file == null");
		free(fcb);
        free(file);
        return NULL;
    }
    
    // settare num
    file->pos = 0;
    file->num = new_block; // inserire blocco libero del disco
    int len = BLOCK_SIZE -sizeof(int) - sizeof(int);
    for(int i=0; i<len ; i++){
        file->data[i] = -1;
    }

	FileHandle* fh = (FileHandle*) malloc(sizeof(FileHandle));
	// scrivere su disco il file
	if(DiskDriver_writeBlock(disk,fcb,new_block,sizeof(FirstFileBlock)) != -1){
		if(DiskDriver_writeBlock(disk,file,free_block,sizeof(FileBlock)) != -1){
			if(AssignDirectory(disk, fdb , new_block, free_block) != -1){
    			fh->sfs = fs;
    			fh->directory = fdb;
    			fh->current_block = NULL;
    			fh->pos_in_file = 0;
    			fh->fcb = fcb;

    			d->dcb->num_entries++;

				free(fcb);
    			free(file);
				FileHandle* fh2 = fh;
				free(fh);
     			return fh2;
			}else{
				fprintf(stderr, "Errore: impossibile assegnare la directory SimpleFS_createFile.\n");
				free(fcb);
				free(fh);
				free(file);
				DiskDriver_freeBlock(disk,new_block);
				DiskDriver_freeBlock(disk,fcb->header.blocks[0]);
				return NULL;
			}
		}else {
			fprintf(stderr,"Errore: writeblock 2 SimpleFS_createFile");
        	free(fcb);
        	free(file);
			free(fh);
        	return NULL;
		}
	}else {
		fprintf(stderr,"Errore: writeblock 1 SimpleFS_createFIle");
        free(fcb);
        free(file);
		free(fh);
        return NULL;
	}
}

//Lorenzo
// reads in the (preallocated) blocks array, the name of all files in a directory 
int SimpleFS_readDir(char** names, DirectoryHandle* d){

    if(names == NULL || d == NULL){
		fprintf(stderr, "Errore: Input non validi SImpleFS_readDir.\n");
		return -1;
	}
	
	DiskDriver* disk = d->sfs->disk;
	FirstDirectoryBlock *fdb = d->dcb;
	
	if(fdb->num_entries <= 0){
		fprintf(stderr, "Errore directory vuota SimpleFS_readDir.\n");
		return -1;
	}
	
	int i, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int), num_tot = 0;
	FirstFileBlock ffb_to_check; 
	
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL) return -1;
	
	if(DiskDriver_readBlock(disk,db,d->dcb->header.blocks[0],sizeof(DirectoryBlock)) != -1){
		for (i=0; i<dim; i++){	
			if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &ffb_to_check, db->file_blocks[i], sizeof(FirstFileBlock)) != -1){ 
				strncpy(names[num_tot],ffb_to_check.fcb.name, 128);
				num_tot++;
			}
		}
	
		if (fdb->num_entries > i){	
		
			if(fdb->fcb.block_in_disk == db->index)
				db = next_block_directory(db, disk,0);
			else
				db = next_block_directory(db, disk,1);

			while (db != NULL){	 
				for (i=0; i<dim; i++){	 
					if (db->file_blocks[i]> 0 && DiskDriver_readBlock(disk, &ffb_to_check, db->file_blocks[i], sizeof(FirstFileBlock)) != -1){ 
						strncpy(names[num_tot],ffb_to_check.fcb.name, 128);
						num_tot++;
					}
				}
				if(fdb->fcb.block_in_disk == db->index)
					db = next_block_directory(db, disk,0);
				else
					db = next_block_directory(db, disk,1);
			}
		}
		free(db);
		return 0;
	} else{
		fprintf(stderr,"Errore impossibile leggere directory block.\n");
		free(db);
		return -1;
	}
	
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


        if(DiskDriver_readBlock(disk, dir, fdb->header.blocks[0],sizeof(DirectoryBlock)) == -1){
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
                if(dir->file_blocks[i] > 0) {
					if(DiskDriver_readBlock(disk,f,dir->file_blocks[i],sizeof(FirstFileBlock)) != -1) {
						if(strncmp(f->fcb.name,filename,128) == 0){
                        	fh->fcb = f;
                        	found = 1;
                        	break;
                    	}
					}
				}
                    
            }
			if(fdb->fcb.block_in_disk == dir->index)
				dir = next_block_directory(dir, disk,0);
			else
				dir = next_block_directory(dir, disk,1);
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
    int index_block = pos - 87*space;
    if(index_block < 0) {
        index_block = 0;
        file_index = pos / (87*space);
        pos = pos - file_index*space;
    } else {
        index_block = index_block / (87*space);
        file_index = ((pos - 87*space)-index_block*(87*space))/space;
        pos = pos - (pos - 87*space)-index_block*(87*space);
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
			if(DiskDriver_updateBlock(disk,(void*)temp, index.blocks[file_index],sizeof(FileBlock)) == -1){ //R. Aggiorno file block su disco
				fprintf(stderr,"Errore impossibile aggiornare file block 1.\n");
				free(temp);
				return -1;
			}
			if(DiskDriver_updateBlock(disk, ffb, ffb->fcb.block_in_disk,sizeof(FirstFileBlock)) == -1){ //R. Aggiorno il first file block
				fprintf(stderr,"Errore impossibile aggiornare ffb.\n");
				free(temp);
				return -1;
			}
            free(temp);
            return written;
        }
        // non basta questo blocco
        else {
            memcpy(temp->data + pos, (char*) data, space - pos);
            written += space - pos;
            size = size - written;
			if(DiskDriver_updateBlock(disk,(void*)temp, index.blocks[file_index],sizeof(FileBlock)) == -1){ //R. Aggiorno file block su disco
				fprintf(stderr,"Errore impossibile updateBlock.\n");
				free(temp);
				return -1;
			}
        }
    } else {
        free(temp);
        return -1;
    }

    FileBlock* corrente = temp;
    int position;

    while(written < size && temp != NULL) {
        if(ffb->fcb.block_in_disk == temp->data[0]) {
            position = next_file_block(corrente, temp, disk,0);
        } else {
            position = next_file_block(corrente, temp, disk,1);
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
			if(DiskDriver_updateBlock(disk, ffb, ffb->fcb.block_in_disk, sizeof(FirstFileBlock)) == -1){ //R. Aggiorno il first file block
				fprintf(stderr,"Errore impossibile updateBlock.\n");
				free(temp);
				return -1;
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
    FirstFileBlock* first_file = f->fcb;
	DiskDriver* my_disk = f->sfs->disk;
	int i;
	
	int off = f->pos_in_file;															
	int written_bytes = first_file->fcb.written_bytes;												
	
	if(size+off > written_bytes){																
		fprintf(stderr,"Error in SimpleFS_read: could not read, size+off > written_bytes.\n");
		return -1;
	}
	
	int bytes_read = 0;
	int to_read = size;
	int space_file_block = BLOCK_SIZE - sizeof(int) - sizeof(int);
	
	FileBlock* file_block_tmp = (FileBlock*)malloc(sizeof(FileBlock));
	if(file_block_tmp == NULL){
		fprintf(stderr,"Error in SimpleFS_read: malloc file_block_tmp.\n");
		return -1;
	}
	
	int file_index_pos;
	int index_block_ref = off - 126*space_file_block;
	if(index_block_ref < 0){
		index_block_ref = 0;
		file_index_pos = off / (87*space_file_block);
		off = off - file_index_pos * space_file_block;
	}
	else{
		index_block_ref = index_block_ref / (87*space_file_block);
		file_index_pos = ((off - 126*space_file_block)-index_block_ref*(87*space_file_block))/space_file_block;
		off = off - (off - 126*space_file_block)-index_block_ref*(87*space_file_block);
	}
		
	FirstBlockIndex index = first_file->header;
		
	for(i=0;i<index_block_ref;i++){
		if(DiskDriver_readBlock(my_disk, (void*)&index, index.post, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Errore readBlock.\n");
			return -1;
		}
	}
		
	if(DiskDriver_readBlock(my_disk, (void*)file_block_tmp, index.blocks[file_index_pos], sizeof(FileBlock)) == -1){
		fprintf(stderr,"Errore impossibile leggere file block in read.\n");
		return -1;
	}
	
	if(off < space_file_block && to_read <= (space_file_block-off)){	
		memcpy(data, file_block_tmp->data + off, to_read);							
		bytes_read += to_read;
		to_read = size - bytes_read;
		f->pos_in_file += bytes_read;
		free(file_block_tmp);
		return bytes_read;
	}

	else if(off < space_file_block && to_read > (space_file_block-off)){
		memcpy(data, file_block_tmp->data + off, space_file_block-off);																	
		bytes_read += space_file_block-off;
		to_read = size - bytes_read;
	}

	else{
		return -1;
	}

	
	while(bytes_read < size && file_block_tmp != NULL){
		if(first_file->fcb.block_in_disk == file_block_tmp->num)
			file_block_tmp =  next_block_file(file_block_tmp,my_disk,0);
		else
			file_block_tmp =  next_block_file(file_block_tmp,my_disk,1);
		
		if(file_block_tmp == NULL){
			f->pos_in_file += bytes_read;
			free(file_block_tmp);
			return bytes_read;
		}
		else if(to_read <= space_file_block){											
			memcpy(data+bytes_read, file_block_tmp->data, to_read);													
			bytes_read += to_read;
			to_read = size - bytes_read;
			f->pos_in_file += bytes_read;
			free(file_block_tmp);
			return bytes_read;
		}
		else{										
			memcpy(data+bytes_read, file_block_tmp->data, space_file_block);																	
			bytes_read += space_file_block;
			to_read = size - bytes_read;
		}
	}

	free(file_block_tmp);
	f->pos_in_file += bytes_read;
	return bytes_read;
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
         fprintf(stderr,"Errore: input non valido SimpleFS_changeDir\n");
         return -1;
     }
     if(strcmp(dirname,"..") == 0){
		if(d->dcb->fcb.block_in_disk == 0){
			return -1;
		}
		free(d->dcb);
        int parent = d->directory->fcb.directory_block;
		d->pos_in_block = 0;
		d->dcb = d->directory;
		if(parent == -1) {
			d->directory = NULL;
			return 0;
		}
        FirstDirectoryBlock * new_parent = malloc(sizeof(FirstDirectoryBlock));


		int ret = DiskDriver_readBlock(d->sfs->disk,new_parent,parent,sizeof(FirstDirectoryBlock));
        if(ret == -1){
            fprintf(stderr,"errore lettura blocco padre (..)");
			free(new_parent);
			d->directory = NULL;
            return -1;
        }
		d->directory = new_parent;
		free(new_parent);
		return 0;
     }
	 else if(d->dcb->num_entries < 0) {
		fprintf(stderr,"Errore: directory vuota\n");
		return -1;
	 }
     else{
         /*TODO
         cercare nella directory corrente la directory con nome dirname e aggiornare directory-handler
         */
		FirstDirectoryBlock* fdb = d->dcb;
		DiskDriver* disk = d->sfs->disk;

        FirstDirectoryBlock* fdb_ctrl = (FirstDirectoryBlock*) malloc(sizeof(FirstDirectoryBlock));
		if(fdb_ctrl == NULL){
			free(fdb_ctrl);
			return -1;
		}
        DirectoryBlock* db = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
        
		if(DiskDriver_readBlock(disk,db,fdb->header.blocks[0],sizeof(DirectoryBlock)) == -1){
			free(db);
			free(fdb_ctrl);
			return -1;
		}

		int dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
		while(db != NULL){
			for(int i=0; i<dim; i++){
				if(db->file_blocks[i] > 0){
					if(DiskDriver_readBlock(disk,fdb_ctrl,db->file_blocks[i],sizeof(FirstDirectoryBlock)) != -1){
						if(strncmp(fdb_ctrl->fcb.name,dirname,128) == 0){
							d->pos_in_block = 0;
							d->directory = fdb;
							d->dcb = fdb_ctrl;
							free(db);
							return 0;
						}
					}
				}
			}
			if(fdb->fcb.block_in_disk == db->index)
				db = next_block_directory(db, disk,0);
			else
				db = next_block_directory(db,disk,1);
		}
		free(db);
		free(fdb_ctrl);
		return -1;
	}
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
    for( int i=0; i<87; i++) {
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

    if(DiskDriver_writeBlock(disk,&block,free_block, sizeof(DirectoryBlock)) == -1){
        return -1;
    }

	if(AssignDirectory(disk,fdb,new_block,free_block) == -1){
		fprintf(stderr, "Errore in mkdir impossibile assegnare directory.\n");
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

	if(d == NULL || filename == NULL) return -1;

    // directory non vuota
    FirstDirectoryBlock* fdb = d->dcb;
	if(fdb->num_entries > 0) {
    	FirstFileBlock* f = (FirstFileBlock*) malloc(sizeof(FirstFileBlock));
    	if(f == NULL) {
        	free(f);
			free(fdb);
        	return -1;
    	}

    	// estrarre primo directory block
    	DirectoryBlock* dir = (DirectoryBlock*) malloc(sizeof(DirectoryBlock));
    	if(dir == NULL) {
    	    free(f);
			free(fdb);
    	    free(dir);
	        return -1;
    	}


    	if(DiskDriver_readBlock(d->sfs->disk, dir, fdb->header.blocks[0],sizeof(DirectoryBlock)) == -1){
    	    free(f);
			free(fdb);
    	    free(dir);
    	    return -1;
    	}

    	int len = ((BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int));


    	// cerco il file
    	while (dir != NULL){
    	    for( int i=0; i< len; i++){
    	        if(dir->file_blocks[i] > 0) {
					if(DiskDriver_readBlock(d->sfs->disk,f,dir->file_blocks[i],sizeof(FirstFileBlock)) != -1) {
						if(strncmp(f->fcb.name,filename,128) == 0){
                	       	DiskDriver_freeBlock(d->sfs->disk,dir->file_blocks[i]);
							for(int p=0;p<87;p++){
								if(f->header.blocks[p] != -1){
									DiskDriver_freeBlock(d->sfs->disk,f->header.blocks[p]);
								}
							}
                       		break;
                   		}
					}
				}
                   
        	}
			if(fdb->fcb.block_in_disk == dir->index)
				dir = next_block_directory(dir, d->sfs->disk,0);
			else
				dir = next_block_directory(dir, d->sfs->disk,1);
        }
		free(f);
		free(dir);
		free(fdb);
		return 0;
	}else{
		free(fdb);
		return -1;
	}

}

int next_file_block(FileBlock* corrente, FileBlock* new, DiskDriver* disk, int flag){
	int index_corrente = corrente->pos;

	if(flag == 1){
		BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(index_corrente + 1 == 126){
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
	}else{
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


}

FileBlock* next_block_file(FileBlock* file,DiskDriver* disk, int flag){

	if(flag == 1){
		BlockIndex* index = (BlockIndex*)malloc(sizeof(BlockIndex));
		if(DiskDriver_readBlock(disk, index, file->num, sizeof(BlockIndex)) == -1){
			fprintf(stderr,"Error in get_block_index_file.\n");
			free(index);
			free(file);
			return NULL;
		}	
	
		int current_position = file->pos;
	 
		if((current_position + 1) == 126){
			if(index->post == -1){
				fprintf(stderr,"Error in get next block file\n");
				free(index);
				free(file);
				return NULL;
			}
			BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
			if(DiskDriver_readBlock(disk, next, index->post, sizeof(BlockIndex)) == -1){
				fprintf(stderr,"Errore nella get next block file\n");
				free(index);
				free(next);
				free(file);
				return NULL;
			}
			FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
			if(DiskDriver_readBlock(disk, next_file, next->blocks[0], sizeof(FileBlock)) == -1){
				fprintf(stderr,"Errore nella get next block file\n");
				free(index);
				free(next);
				free(next_file);
				free(file);
				return NULL;
			}
			free(index);
			free(next);
			free(file);
		
			return next_file;
		}
		else{
		FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
		if(DiskDriver_readBlock(disk, next_file, index->blocks[current_position + 1], sizeof(FileBlock)) == -1){
			fprintf(stderr,"Errore nella get next block file\n");
			free(next_file);
			free(index);
			free(file);
			return NULL;
		}
		free(index);
		free(file);

		return next_file;
		}
	}else{
		FirstBlockIndex* index = (FirstBlockIndex*)malloc(sizeof(FirstBlockIndex));
		if(DiskDriver_readBlock(disk, index, file->num, sizeof(FirstBlockIndex)) == -1){
			fprintf(stderr,"Error in get_block_index_file.\n");
			free(index);
			free(file);
			return NULL;
		}
	
		int current_position = file->pos;
	 
		if((current_position + 1) == 87){
			if(index->post == -1){
				fprintf(stderr,"Error in get next block file\n");
				free(index);
				free(file);
				return NULL;
			}
			BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
			if(DiskDriver_readBlock(disk, next, index->post, sizeof(BlockIndex)) == -1){
				fprintf(stderr,"Errore nella get next block file\n");
				free(index);
				free(next);
				free(file);
				return NULL;
			}
			FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
			if(DiskDriver_readBlock(disk, next_file, next->blocks[0], sizeof(FileBlock)) == -1){
				fprintf(stderr,"Errore nella get next block file\n");
				free(index);
				free(next);
				free(next_file);
				free(file);
				return NULL;
			}
			free(index);
			free(next);
			free(file);
		
			return next_file;
		}
		else{
			FileBlock* next_file = (FileBlock*)malloc(sizeof(FileBlock));
			if(DiskDriver_readBlock(disk, next_file, index->blocks[current_position + 1], sizeof(FileBlock)) == -1){
				fprintf(stderr,"Errore nella get next block file\n");
				free(next_file);
				free(index);
				free(file);
				return NULL;
			}
			free(index);
			free(file);

			return next_file;
		}
	}

}

DirectoryBlock* next_block_directory(DirectoryBlock* directory,DiskDriver* disk,int flag){
	int current_position = directory->pos;
	DirectoryBlock* next_directory = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(flag == 1){
		BlockIndex* index = (BlockIndex*) malloc(sizeof(BlockIndex));
		if (DiskDriver_readBlock(disk, index, directory->index, sizeof(BlockIndex)) == -1) return NULL;
		if((current_position +1) != 120){
			if(DiskDriver_readBlock(disk, next_directory, index->blocks[current_position + 1], sizeof(DirectoryBlock)) == -1){
				fprintf(stderr,"Errore: readBlock directory next_block_directory\n");
				free(next_directory);
				free(directory);
				free(index);
				return NULL;
			}
		free(index);
		free(directory);

		}else{
			if(index->post == -1){
				fprintf(stderr,"Errore blocco successivo non esiste next_block_directory\n");
				free(index);
				free(directory);
				return NULL;
			}
			BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
			if(DiskDriver_readBlock(disk, next, index->post, sizeof(BlockIndex)) == -1){
				fprintf(stderr,"Errore readBlock next_block_directory\n");
				free(next);
				free(index);
				free(directory);
				return NULL;
			}
			if(DiskDriver_readBlock(disk, next_directory, next->blocks[0], sizeof(DirectoryBlock)) == -1){
				fprintf(stderr,"Errore readBlock next_block_directory\n");
				free(next_directory);
				free(index);
				free(next);
				free(directory);
				return NULL;
			}
			free(index);
			free(next);
			free(directory);
		}
	}else{
		FirstBlockIndex* index = (FirstBlockIndex*) malloc(sizeof(FirstBlockIndex));
		if(DiskDriver_readBlock(disk, index, directory->index, sizeof(FirstBlockIndex)) == -1){
			return NULL;
		}
		if((current_position + 1) == 87){
			if(index->post == -1){
				fprintf(stderr,"Errore blocco successivo non esiste next_block_directory\n");
				free(index);
				free(directory);
				return NULL;
			}
			BlockIndex* next = (BlockIndex*)malloc(sizeof(BlockIndex));
			if(DiskDriver_readBlock(disk, next, index->post, sizeof(BlockIndex)) == -1){
				fprintf(stderr,"Errore readBlock next_block_directory\n");
				free(next);
				free(index);
				free(directory);
				return NULL;
			}
			if(DiskDriver_readBlock(disk, next_directory, next->blocks[0], sizeof(DirectoryBlock)) == -1){
				fprintf(stderr,"Errore readBlock next_block_directory\n");
				free(next_directory);
				free(index);
				free(next);
				free(directory);
				return NULL;
			}
			free(index);
			free(next);
			free(directory);
			}
		else{
			if(DiskDriver_readBlock(disk, next_directory, index->blocks[current_position + 1], sizeof(DirectoryBlock)) == -1){
				free(next_directory);
				free(index);
				free(directory);
				return NULL;
			}
			free(index);
			free(directory);
		}
	}
	return next_directory;
}

int AssignDirectory(DiskDriver* disk, FirstDirectoryBlock* fdb, int ffb, int fb){
	
	if(disk == NULL || fdb == NULL || ffb == -1 || fb == -1){
		fprintf(stderr, "Errore: Input non valido AssignDirectory.\n");
		DiskDriver_freeBlock(disk, ffb);
		DiskDriver_freeBlock(disk, fb);
		return -1;
	}
	int i, block_pos, dim = (BLOCK_SIZE-sizeof(int)-sizeof(int))/sizeof(int);
	
	DirectoryBlock* db = (DirectoryBlock*)malloc(sizeof(DirectoryBlock));
	if(db == NULL) return -1;
	if(DiskDriver_readBlock(disk, db, fdb->header.blocks[0],sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Errore: readBlock AssignDirectory.\n");
		free(db);
		DiskDriver_freeBlock(disk, ffb);
		DiskDriver_freeBlock(disk, fb);
		return -1;
	}

	for(i=0; i<dim; i++){
		if(db->file_blocks[i] == 0){  
			block_pos = i;
			break;
		}
	}
	
	db->file_blocks[block_pos] = ffb;
	
	fdb->num_entries++;

	int pos_db;
	if(DiskDriver_updateBlock(disk, fdb, fdb->fcb.block_in_disk, sizeof(FirstDirectoryBlock)) != -1){
		db->index = fdb->fcb.block_in_disk;
		if(fdb->fcb.block_in_disk != db->index){
			BlockIndex index;
			if(DiskDriver_readBlock(disk, &index, db->index, sizeof(BlockIndex)) == -1){
				fprintf(stderr,"Errore: impossibile leggere l'indice del blocco AssignDirectory.\n");
				DiskDriver_freeBlock(disk, ffb);
				DiskDriver_freeBlock(disk, fb);
				free(db);
				return -1;
			}
			pos_db = index.blocks[db->pos];
		}else{
			FirstBlockIndex index;
			if(DiskDriver_readBlock(disk, &index, db->index, sizeof(FirstBlockIndex)) == -1){
				fprintf(stderr,"Errore: impossibile leggere l'indice del blocco AssignDirectory.\n");
				DiskDriver_freeBlock(disk, ffb);
				DiskDriver_freeBlock(disk, fb);
				free(db);
				return -1;
			}
			pos_db = index.blocks[db->pos];
		}
	}else {
		fprintf(stderr,"Errore: updateBlock fdb AssignDirectory.\n");
		DiskDriver_freeBlock(disk, ffb);
		DiskDriver_freeBlock(disk, fb);
		free(db);
		return -1;
	}

	if(DiskDriver_updateBlock(disk, db, pos_db, sizeof(DirectoryBlock)) == -1){
		fprintf(stderr,"Errore: updateBlock db AssignDirectory.\n");
		DiskDriver_freeBlock(disk, ffb);
		DiskDriver_freeBlock(disk, fb);
		free(db);
		return -1;
	}
	
	free(db);
	return 0;		
}
