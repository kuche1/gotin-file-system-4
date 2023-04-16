
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
    storage.free_blocks = NULL;
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
        disk->blocks = NULL;

        char *path_to_disk = locations[di];
        FILE *f = fopen(path_to_disk, "r+"); // if file does not exist, it will return `NULL` instead of creating it
        if(!f){
            err = ERR_FOPEN;
            goto err;
        }
        disk->location = f;

        storage.num_disks += 1;
    }

    FILE *master_disk = storage.disks[0].location;

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
            printf("loaded 1 file: ");
            print_str(FILE_NAME_SIZE, file->name);
            printf("\n");
        }
#endif
    }

    // set block indexes
    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        
        disk_offset_t disk_current = ftell(disk->location);
        fseek(disk->location, 0, SEEK_END);
        disk_offset_t disk_size = ftell(disk->location); // TODO bad solution, limited to 2GiB; tested with 3GiB, the return value is as if 2GiB; ? can return negative value ?
        fseek(disk->location, disk_current, SEEK_SET);
#ifdef GFS_DEBUG
        printf("gfs: size of device %d: %li\n", di, disk_size);
#endif

        disk_offset_t remaining_space = disk_size - disk_current;

        int blocks = remaining_space / BLOCKSIZE_INFO_DATA;

        if(!MALLOC_AND_SET(disk->blocks, blocks)){
            err = ERR_MALLOC;
            goto err;
        }

        // disk_offset_t block_offset = 0; // TODO we could use this kind of mechanism for the `ftell` limit
        for(int bi=0; bi<blocks; bi++){
            struct block *block = &(disk->blocks[bi]);
            struct block_info *block_info = &(block->info);
            block_info->location.disk_idx = di;
            block_info->location.offset = ftell(disk->location); // TODO again, 2GiB limit
            // block_info->offset = block_offset;
            // block_offset += BLOCKSIZE_INC_INFO;

            FILE *file = storage.disks[block_info->location.disk_idx].location;

            if(FREAD(&block_info->next, 1, file) != 1){
                err = ERR_FREAD;
                goto err;
            }
            fseek(file, BLOCKSIZE_DATA, SEEK_CUR);

            disk->num_blocks += 1;
        }
    }

    // allocate enough memory for free blocks arr; assume that 100% of blocks could be free
    int total_blocks = 0;
    for(int di=0; di<storage.num_disks; ++di){
        total_blocks += storage.disks[di].num_blocks;
    }

    // allocate free block arr
    if(!MALLOC_AND_SET(storage.free_blocks, total_blocks)){
        err = ERR_MALLOC;
        goto err;
    }
    storage.free_blocks_size = total_blocks;

    // get number of free blocks and assign in order of disk1->disk2->disk3->...->disk1
    storage.free_blocks_start = 0;
    storage.free_blocks_end = 0;
    for(int bi=0;; ++bi){
        int at_least_one_disk_available = 0;
        for(int di=0; di<storage.num_disks; ++di){
            struct disk *disk = &(storage.disks[di]);
            if(bi >= disk->num_blocks){
                break;
            }
            at_least_one_disk_available = 1;
            struct block *block = &(disk->blocks[bi]);
            struct block_info *block_info = &(block->info);
            if(block_info->next.offset == BLOCK_NEXT_FREE){
                storage.free_blocks[storage.free_blocks_end] = block;
                storage.free_blocks_end += 1;
            }
        }
        if(!at_least_one_disk_available){
            break;
        }
    }

    // TODO we might be able to free some mem here, need to think about it tomorrow

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
        FCLOSE(disk->location); // fclose(disk->location);
        DEMALLOC(disk->blocks);
    }
    DEMALLOC(storage.files);
    DEMALLOC(storage.disks);
    DEMALLOC(storage.free_blocks);
}

int gfs_format(void){
#ifdef GFS_DEBUG
    printf("gfs: format start\n");
#endif

    storage.num_files = NUMBER_OF_FILES;
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *file = &(storage.files[fi]);
        file->first_block.offset = BLOCK_NEXT_FREE; // signify that the file is not allocated
    }

    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        for(int bi=0; bi<disk->num_blocks; ++bi){
            struct block *block = &(disk->blocks[bi]);
            struct block_info *block_info = &(block->info);
            block_info->next.offset = BLOCK_NEXT_FREE; // signify that the block is not allocated
        }
    }

#ifdef GFS_DEBUG
    printf("gfs: format end (sync pending)\n");
#endif
    return gfs_sync();
}

int gfs_sync(void){
#ifdef GFS_DEBUG
    printf("gfs: sync start\n");
#endif

    if(storage.num_disks == 0){
        return 0;
    }

    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        rewind(disk->location);
    }

    FILE *master_disk = storage.disks[0].location;

    // write number of files
    if(fwrite(&storage.num_files, sizeof(storage.num_files), 1, master_disk) != 1){
        return ERR_FWRITE;
    }
    // and write files meradata
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *file = &(storage.files[fi]);
        if(FWRITE(file->name, FILE_NAME_SIZE, master_disk) != FILE_NAME_SIZE){
            return ERR_FWRITE;
        }
        if(FWRITE(&file->first_block, 1, master_disk) != 1){
            return ERR_FWRITE;
        }
    }

    // TODO this sucks (it only works currentsy since the number of files is hardcoded)
    // some size should be skipped (size for metadata should be predetermined)

    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);

        for(int bi=0; bi<disk->num_blocks; ++bi){
            struct block *block = &(disk->blocks[bi]);
            struct block_info *block_info = &(block->info);

            FILE *file = storage.disks[block_info->location.disk_idx].location;

            if(FWRITE(&block_info->next, 1, file) != 1){
                return ERR_FWRITE;
            }
            fseek(file, BLOCKSIZE_DATA, SEEK_CUR);
        }
    }

#ifdef GFS_DEBUG
    printf("gfs: sync end\n");
#endif
    return 0;
}

int gfs_sync_block(struct block *block){
    // struct disk *disk = storage.disks[block->info.disk_idx];
    // TODO
    block = block;
    return 0;
}

int gfs_sync_file(struct file *file){
    // TODO
    file = file;
    return 0;
}

// struct block *gfs_find_block(struct block_location *location){
//     storage.disks[location.block_idx]
// }

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
    if(gfs_find_file(file_name)){
#ifdef GFS_DEBUG
        printf("gfs: err: could not create file since file with same name already exists\n");
#endif
        return ERR_FILE_WITH_SAME_NAME_ALREADY_EXISTS;
    }
    // find unallocated file
    struct file *file = NULL;
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *candidate = &storage.files[fi];
        if(candidate->first_block.offset == BLOCK_NEXT_FREE){
            file = candidate;
            break;
        }
    }
    if(!file){
#ifdef GFS_DEBUG
        printf("gfs: err: can't find any unallocated files\n");
#endif
        return ERR_NO_UNALLOCATED_FILE;
    }
    // find unallocated disk block // TODO we could just skip this part and assign BLOCK_NEXT_NONE
    if(storage.free_blocks_start == storage.free_blocks_end){
#ifdef GFS_DEBUG
        printf("gfs: err: can't find any unallocated blocks\n");
#endif
        return ERR_NO_UNALLOCATED_BLOCK;
    }
    struct block *block = storage.free_blocks[storage.free_blocks_start];
    storage.free_blocks_start += 1;
    storage.free_blocks_start = storage.free_blocks_start % storage.free_blocks_size;
    // tell block it's allocated
    block->info.next.offset = BLOCK_NEXT_NONE;
    // tell file it's allocated and set info
    memcpy(file->name, file_name, sizeof(*file->name) * FILE_NAME_SIZE);
    file->first_block = block->info.location;
    // sync
    int err;
    if((err = gfs_sync_block(block))){ // TODO should we sync the block first or the file? think about this
        return err;
    }
    if((err = gfs_sync_file(file))){
        return err;
    }
    return 0;
}

// int gfs_delete_file(char file_name[FILE_NAME_SIZE]){ // TODO think about the syncing here
//     struct file *file = gfs_find_file(file_name);
//     if(!file){
//         return ERR_FILE_DOESNT_EXIST;
//     }

//     struct block *block = gfs_find_block(file->first_block);
//     file->first_block.offset = BLOCK_NEXT_FREE; // deallocate file
//     gfs_sync_file(file); // TODO check for error
    
//     while(block){
//         struct block *next = gfs_find_block(block->info.next);
//         block->info.location.offset = BLOCK_NEXT_FREE;
//         gfs_sync_block(block); // TODO check for error
//         block = next;
//     }
// }
