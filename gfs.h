
// TODO
// use `int32_t` instead of `int` for structs that are written-read from disk

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define BLOCKSIZE_DATA 512 // seems reasonable, might need to change this in the future, or might make it a customizable value
#define BLOCKSIZE_INFO_DATA (BLOCKSIZE_DATA + sizeof(block_offset_t))

////////////////////////// enum

enum{
    ERR_MALLOC = 1,
    ERR_FOPEN,
    ERR_FREAD,
    ERR_FWRITE,
};

enum{
    BLOCK_NEXT_NONE = -1, // means that this is the last block
    BLOCK_NEXT_FREE = -2, // means that this block is not even allocated for anything
};

////////////////////////// struct

struct storage{
    int num_disks;
    struct disk *disks;

    // circular buffer
    int free_blocks_start; // at which idx the next free block is located
    int free_blocks_end; // at which idx the last free block is located
    struct block **free_blocks;
};

struct disk{
    FILE *location;
    int num_blocks;
    struct block *blocks;
};

typedef long int block_offset_t; // offset for `fseek` // what in the actual fuck this was supposed to be limited to 2GiB but in my tests it works perfectly fine with 8GiB

struct block_info{
    FILE *location;
    block_offset_t offset;
    block_offset_t next_block; // update to this type would require update to `BLOCKSIZE_INC_INFO`
};

struct block_data{
    char data[BLOCKSIZE_DATA]; // TODO delete?
};

struct block{
    struct block_info info;
    struct block_data *data; // TODO no need?
};

// struct file{
//     char name[10];
//     block_offset_t first_block;
// }

////////////////////////// function

int gfs_init(int disks, char **locations);
void gfs_deinit(void);
int gfs_format(void);
int gfs_sync(void);
