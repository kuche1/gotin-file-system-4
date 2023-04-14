
#include <string.h>

#include "helpers.h"

#include "gfs.h"

struct storage storage;

int gfs_init(int disks, char **locations){
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
        if(FREAD(&(file->first_block_disk_idx), 1, master_disk) != 1){
            err = ERR_FREAD;
            goto err;
        }
        if(FREAD(&(file->first_block_offset), 1, master_disk) != 1){
            err = ERR_FREAD;
            goto err;
        }
    }

    // set block indexes
    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        
        disk_offset_t disk_current = ftell(disk->location);
        fseek(disk->location, 0, SEEK_END);
        disk_offset_t disk_size = ftell(disk->location); // TODO bad solution, limited to 2GiB; tested with 3GiB, the return value is as if 2GiB; ? can return negative value ?
        fseek(disk->location, disk_current, SEEK_SET);
#ifdef GFS_DEBUG
        printf("size of device %d: %li\n", di, disk_size);
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
            block_info->disk_idx = di;
            block_info->offset = ftell(disk->location); // TODO again, 2GiB limit
            // block_info->offset = block_offset;
            // block_offset += BLOCKSIZE_INC_INFO;

            FILE *file = storage.disks[block_info->disk_idx].location;

            int read = fread(&(block_info->next_block), sizeof(block_info->next_block), 1, file);
            if(read != 1){
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
            if(block_info->next_block == BLOCK_NEXT_FREE){
                storage.free_blocks[storage.free_blocks_end] = block;
                storage.free_blocks_end += 1;
            }
        }
        if(!at_least_one_disk_available){
            break;
        }
    }

    // TODO we might be able to free some mem here, need to think about it tomorrow

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
    printf("formatting\n");
#endif

    storage.num_files = NUMBER_OF_FILES;
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *file = &(storage.files[fi]);
        file->first_block_offset = BLOCK_NEXT_FREE; // signify that file is not allocated
    }

    for(int di=0; di<storage.num_disks; ++di){
        struct disk *disk = &(storage.disks[di]);
        for(int bi=0; bi<disk->num_blocks; ++bi){
            struct block *block = &(disk->blocks[bi]);
            struct block_info *block_info = &(block->info);
            block_info->next_block = BLOCK_NEXT_NONE;
        }
    }
    return gfs_sync();
}

int gfs_sync(void){
#ifdef GFS_DEBUG
    printf("syncing\n");
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
        if(FWRITE(&file->first_block_disk_idx, 1, master_disk) != 1){
            return ERR_FWRITE;
        }
        if(FWRITE(&file->first_block_offset, 1, master_disk) != 1){
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

            FILE *file = storage.disks[block_info->disk_idx].location;

            int written = fwrite(&(block_info->next_block), sizeof(block_info->next_block), 1, file);
            if(written != 1){
                return ERR_FWRITE;
            }
            fseek(file, BLOCKSIZE_DATA, SEEK_CUR);
        }
    }
    return 0;
}

// int gfs_sync_block(struct block *block){
//     struct disk *disk = storage.disks[block->info.disk_idx];
// }

int gfs_create_file(char file_name[FILE_NAME_SIZE]){
    // find unallocated file
    struct file *file = NULL;
    for(int fi=0; fi<storage.num_files; ++fi){
        struct file *candidate = &storage.files[fi];
        if(candidate->first_block_offset == BLOCK_NEXT_FREE){
            file = candidate;
            break;
        }
    }
    if(!file){
        return ERR_NO_UNALLOCATED_FILE;
    }
    // find unallocated disk block
    if(storage.free_blocks_start == storage.free_blocks_end){
        return ERR_NO_UNALLOCATED_BLOCK;
    }
    struct block *block = storage.free_blocks[storage.free_blocks_start];
    storage.free_blocks_start += 1;
    storage.free_blocks_start = storage.free_blocks_start % storage.free_blocks_size;
    // tell block it's allocated
    block->info.next_block = BLOCK_NEXT_NONE;
    // tell file it's allocated and set info
    memcpy(file->name, file_name, sizeof(*file->name) * FILE_NAME_SIZE);
    file->first_block_disk_idx = block->info.disk_idx;
    file->first_block_offset = block->info.offset;
    // TODO now call a proper `sync_file` and `sync_block` fncs
    return 0;
}
