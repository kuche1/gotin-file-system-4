
int free_blocks_on_disk_init(struct free_blocks_on_disk *fb, int size){
    fb->start = 0;
    fb->end = 0;
    fb->size = size;
    fb->offsets = NULL; // will be set in `MALLOC_AND_SET` anyways, but let's set it here also just in case

    if(!MALLOC_AND_SET(fb->offsets, size)){ // assume all blocks could potentially be free
        return ERR_MALLOC;
    }

    return 0;
}

void free_blocks_on_disk_deinit(struct free_blocks_on_disk *fb){
    DEMALLOC(fb->offsets);
}

void free_blocks_on_disk_append(struct free_blocks_on_disk *fb, disk_offset_t item){
    fb->offsets[fb->end] = item;
    fb->end = (fb->end + 1) % fb->size;
}
