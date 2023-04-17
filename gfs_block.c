
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
