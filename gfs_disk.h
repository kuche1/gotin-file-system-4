
struct free_blocks_on_disk{
    // circular buffer
    int start; // at which index is the next free block located
    int end; // at which index is the last free block located
    int size; // needed for `start = (start+1) % size` and `end`
    disk_offset_t *offsets; // no need to use `storage_location` since the disk is already known
};

struct disk{
    FILE *location;
    disk_offset_t block_section;
    
    int num_blocks; // TODO not needed? (used for `format` as of right now)
    struct free_blocks_on_disk free_blocks;
};

void free_blocks_on_disk_append(struct free_blocks_on_disk *fb, disk_offset_t item);
