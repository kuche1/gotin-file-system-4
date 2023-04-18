
struct block_info{
    struct storage_location location;
    struct storage_location next;
};

struct block{
    struct block_info info;
    char data[BLOCKSIZE_DATA];
};

// sync
int gfs_sync_block(struct block *block); // TODO split into `sync_info` and `sync_data`

// read from disk
int gfs_read_block(struct block *block, struct storage_location location);

// find unallocated
int gfs_find_unallocated_block(struct block *block);
// returned block is set to `BLOCK_NEXT_NONE` and needs to be synced by caller

int gfs_deallocate_block(struct block *block); // TODO unfinished
// syncs and updates the free blocks arr on it's own
