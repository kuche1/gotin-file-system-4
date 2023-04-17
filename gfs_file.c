
int gfs_sync_file(struct file *file){
    FILE *f = storage.disks[file->location.disk_idx].location;
    if(fseek(f, file->location.offset, SEEK_SET)){
        return ERR_FSEEK;
    }

    if(FWRITE(file->name, FILE_NAME_SIZE, f) != FILE_NAME_SIZE){
        return ERR_FWRITE;
    }
    if(FWRITE(&file->first_block, 1, f) != 1){
        return ERR_FWRITE;
    }

    return 0;
}

int gfs_find_unallocated_file(struct file **file){
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *candidate = &storage.files[fi];
        if(candidate->first_block.offset == BLOCK_NEXT_FREE){
            *file = candidate;
            return 0;
        }
    }
    return ERR_NO_UNALLOCATED_FILE;
}

struct file *gfs_find_file(char file_name[FILE_NAME_SIZE]){
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *file = &storage.files[fi];
        if(file->first_block.offset >= 0){
            if(memcmp(file_name, file->name, sizeof(*file_name) * FILE_NAME_SIZE) == 0){
                return file;
            }
        }
    }
    return NULL;
}

int gfs_create_file(char file_name[FILE_NAME_SIZE]){
    int err;

    if(gfs_find_file(file_name)){
#ifdef GFS_DEBUG
        printf("gfs: err: could not create file since file with same name already exists `");
        print_str(FILE_NAME_SIZE, file_name);
        printf("`\n");
#endif
        return ERR_FILE_WITH_SAME_NAME_ALREADY_EXISTS;
    }
    
    // find unallocated file
    struct file *file;
    if((err = gfs_find_unallocated_file(&file))){
#ifdef GFS_DEBUG
        printf("gfs: err: problem with finding unallocated file\n");
#endif
        return err;
    }

    // TODO we could just skip this part and assign BLOCK_NEXT_NONE
    //     but let's do it for test coverage's case

    // find unallocated disk block
    struct block block;
    if((err = gfs_find_unallocated_block(&block))){
#ifdef GFS_DEBUG
        printf("gfs: err: problem with finding unallocated block\n");
        return err;
#endif
    }
    // tell the block that it's now allocated
    block.info.next.offset = BLOCK_NEXT_NONE;
    // tell file it's allocated and set info
    memcpy(file->name, file_name, sizeof(*file->name) * FILE_NAME_SIZE);
    file->first_block = block.info.location;

    // sync
    if((err = gfs_sync_block(&block))){ // TODO should we sync the block first or the file? think about this
        return err;
    }
    if((err = gfs_sync_file(file))){
        return err;
    }

    return 0;
}

// TODO think about the syncing here
int gfs_delete_file(char file_name[FILE_NAME_SIZE]){
    int err;

    struct file *file = gfs_find_file(file_name);
    if(!file){
#ifdef GFS_DEBUG
        printf("gfs: err: cannot delete file as it doesn't exist `");
        print_str(FILE_NAME_SIZE, file_name);
        printf("`\n");
#endif
        return ERR_FILE_DOESNT_EXIST;
    }

    struct storage_location first_block = file->first_block;

    file->first_block.offset = BLOCK_NEXT_FREE; // mark file as deallocated
    if((err = gfs_sync_file(file))){ // sync
        return err;
    }

    if(first_block.offset == BLOCK_NEXT_NONE){ // file doesn't consist of any blocks
        return 0;
    }

    struct block block;
    if((err = gfs_read_block(&block, file->first_block))){ // TODO gfs_read_block_info
        return err;
    }

    while(1){
        struct storage_location next = block.info.next;

        // deallocate
        block.info.next.offset = BLOCK_NEXT_FREE;
        // sync
        if((err = gfs_sync_block(&block))){
            return err;
        }

        if(next.offset == BLOCK_NEXT_NONE){ // no next block
            break;
        }

        // read next and write to `block`
        if((err = gfs_read_block(&block, next))){
            return err;
        }
    }

    return 0;
}
