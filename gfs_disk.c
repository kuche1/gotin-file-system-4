
void free_blocks_on_disk_append(struct free_blocks_on_disk *fb, disk_offset_t item){
    fb->offsets[fb->end] = item;
    fb->end = (fb->end + 1) % fb->size;
}
