
#include <string.h>

#include "helpers.h"

#include "gfs.h"

struct storage storage;

int gfs_init(int disks, char **locations){
#if GFS_DEBUG
    printf("gfs: init start\n");
#endif
    int err = 0;

    if(disks == 0){
        err = ERR_UNREACHABLE;
        goto err;
    }

    storage.num_disks = 0;
    storage.last_allocated_disk = disks - 1; // start at first disk (this will be increased by 1 then `%`ed)
    storage.disks = NULL;
    storage.num_files = 0;
    storage.files = NULL;

    if(!MALLOC_AND_SET(storage.disks, disks)){
        err = ERR_MALLOC;
        goto err;
    }

    // set block indexes
    for(int di=0; di<disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        
        disk->location = NULL;
        disk->num_blocks = 0;

        char *path_to_disk = locations[di];
        FILE *f = fopen(path_to_disk, "r+"); // if file does not exist, it will return `NULL` instead of creating it
        if(!f){
            err = ERR_FOPEN;
            goto err;
        }
        disk->location = f;

        storage.num_disks += 1;
    }

    int master_disk_idx = 0;
    FILE *master_disk = storage.disks[master_disk_idx].location;

    // save location
    storage.file_section.disk_idx = master_disk_idx;
    storage.file_section.offset = ftell(master_disk);
    // read number of files
    if(FREAD(&storage.num_files, 1, master_disk) != 1){
        err = ERR_FREAD;
        goto err;
    }
    storage.num_files = NUMBER_OF_FILES; // TODO hack remove
    // alloc mem
    if(!MALLOC_AND_SET(storage.files, storage.num_files)){
        err = ERR_MALLOC;
        goto err;
    }
    // read data for each file
    for(int fi=0; fi<storage.num_files; fi++){
        struct file *file = &(storage.files[fi]);

        // save location
        file->location.disk_idx = master_disk_idx;
        file->location.offset = ftell(master_disk);

        if(FREAD(file->name, FILE_NAME_SIZE, master_disk) != FILE_NAME_SIZE){
            err = ERR_FREAD;
            goto err;
        }
        if(FREAD(&file->first_block, 1, master_disk) != 1){
            err = ERR_FREAD;
            goto err;
        }
#ifdef GFS_DEBUG
        if(file->first_block.offset >= 0){
            printf("gfs: loaded 1 file: ");
            print_str(FILE_NAME_SIZE, file->name);
            printf("\n");
        }
#endif
    }

    // set block indexes
    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);

        // save location
        disk->block_section = ftell(disk->location);

        // calc disk space
        disk_offset_t disk_current = ftell(disk->location);
        if(fseek(disk->location, 0, SEEK_END)){
            err = ERR_FSEEK;
            goto err;
        }
        disk_offset_t disk_size = ftell(disk->location);
        if(fseek(disk->location, disk_current, SEEK_SET)){
            err = ERR_FSEEK;
            goto err;
        }
#ifdef GFS_DEBUG
        printf("gfs: size of device %d: %li\n", di, disk_size);
#endif

        disk_offset_t remaining_space = disk_size - disk_current;

        int blocks = remaining_space / BLOCKSIZE_INFO_DATA;

        // init free blocks
        disk->free_blocks.start = 0;
        disk->free_blocks.end = 0;
        disk->free_blocks.size = blocks;

        if(!MALLOC_AND_SET(disk->free_blocks.offsets, blocks)){ // assume all blocks could potentially be free
            err = ERR_MALLOC;
            goto err;
        }

        for(int bi=0; bi<blocks; bi++){

            struct block_info block_info;
            // block_info.location.disk_idx = di; // not needed
            block_info.location.offset = ftell(disk->location);

            if(FREAD(&block_info.next, 1, disk->location) != 1){
                err = ERR_FREAD;
                goto err;
            }
            if(fseek(disk->location, BLOCKSIZE_DATA, SEEK_CUR)){
                err = ERR_FSEEK;
                goto err;
            }

            if(block_info.next.offset == BLOCK_NEXT_FREE){
                disk->free_blocks.offsets[disk->free_blocks.end] = block_info.location.offset;
                disk->free_blocks.end += 1;
            }

            disk->num_blocks += 1;
        }
    }

#if GFS_DEBUG
    printf("gfs: init end\n");
#endif

    return 0;
err:
    gfs_deinit();
    return err;
}

void gfs_deinit(void){
    for(int i=0; i<storage.num_disks; ++i){
        struct disk *disk = &(storage.disks[i]);
        FCLOSE(disk->location);
        DEMALLOC(disk->free_blocks.offsets);
    }
    DEMALLOC(storage.files);
    DEMALLOC(storage.disks);
}

// TODO free blocks need to be refreshed after calling this
int gfs_format(void){
#ifdef GFS_DEBUG
    printf("gfs: format start\n");
#endif

    int err;

    storage.num_files = NUMBER_OF_FILES;
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *file = &(storage.files[fi]);
        file->first_block.offset = BLOCK_NEXT_FREE; // signify that the file is not allocated
        if((err = gfs_sync_file(file))){
            return err;
        }
    }

    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        for(int bi=0; bi<disk->num_blocks; ++bi){
            struct block block;

            block.info.location.disk_idx = di;
            block.info.location.offset = disk->block_section + (bi * BLOCKSIZE_INFO_DATA);

            // block.info.next.disk_idx = undefined;
            block.info.next.offset = BLOCK_NEXT_FREE;

            if((err = gfs_sync_block(&block))){
                return err;
            }
        }
    }

#ifdef GFS_DEBUG
    printf("gfs: format end\n");
#endif
    return 0;
}

// TODO split into `sync_info` and `sync_data`
int gfs_sync_block(struct block *block){
    FILE *f = storage.disks[block->info.location.disk_idx].location;
    if(fseek(f, block->info.location.offset, SEEK_SET)){
        return ERR_FSEEK;
    }
    
    if(FWRITE(&block->info.next, 1, f) != 1){
        return ERR_FWRITE;
    }
    if(FWRITE(block->data, BLOCKSIZE_DATA, f) != BLOCKSIZE_DATA){
        return ERR_FWRITE;
    }

    return 0;
}

// TODO split into `sync_info` and `sync_data`
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

int gfs_read_block(struct block *block, struct storage_location location){
    FILE *f = storage.disks[location.disk_idx].location;
    if(fseek(f, location.offset, SEEK_SET)){
        return ERR_FSEEK;
    }

    if(FREAD(&block->info.next, 1, f) != 1){
        return ERR_FREAD;
    }
    if(FREAD(block->data, BLOCKSIZE_DATA, f) != BLOCKSIZE_DATA){
        return ERR_FREAD;
    }

    block->info.location = location;

    return 0;
}

// returned block needs to be changed from `UNALLOCATED` and synced by caller
int gfs_find_unallocated_block(struct block *block){
    int err;

    int disk_idx_last = storage.last_allocated_disk;
    int disk_idx = (disk_idx_last + 1) % storage.num_disks;

    while(1){
        struct disk *disk = &storage.disks[disk_idx];

        if(disk->free_blocks.start != disk->free_blocks.end){ // there is a free block
            break;
        }

        if(disk_idx == disk_idx_last){
            return ERR_NO_UNALLOCATED_BLOCK;
        }

        disk_idx = (disk_idx+1) % storage.num_disks;
    }

    storage.last_allocated_disk = disk_idx;
    struct disk *disk = &storage.disks[disk_idx];

    int block_idx = disk->free_blocks.start;
    disk->free_blocks.start = (disk->free_blocks.start + 1) % disk->free_blocks.size;
    disk_offset_t block_offset = disk->free_blocks.offsets[block_idx];

    struct storage_location location = {
        .disk_idx = disk_idx,
        .offset = block_offset,
    };
    if((err = gfs_read_block(block, location))){
        return err;
    }

    block->info.next.offset = BLOCK_NEXT_NONE; // should not be needed (as the caller should overwrite this), doing it just in case

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
