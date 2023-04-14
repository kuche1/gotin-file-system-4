
// TODO
// use `int32_t` instead of `int` for structs that are written-read from disk

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define FILE_NAME_SIZE 5
#define NUMBER_OF_FILES 30 // TODO this is super bad, but it will have to do for now

#define BLOCKSIZE_DATA 512 // seems reasonable, might need to change this in the future, or might make it a customizable value
#define BLOCKSIZE_INFO_DATA (BLOCKSIZE_DATA + sizeof(struct block_location))

////////////////////////// enum

enum{
    // generic
    ERR_MALLOC = 1,
    ERR_FOPEN,
    ERR_FREAD,
    ERR_FWRITE,
    // generic, unusual
    ERR_UNREACHABLE,
    // disk resource allocation
    ERR_NO_UNALLOCATED_FILE,
    ERR_NO_UNALLOCATED_BLOCK,
    ERR_FILE_WITH_SAME_NAME_ALREADY_EXISTS,
    // disk resource deletion
    ERR_FILE_DOESNT_EXIST,
};

enum{
    BLOCK_NEXT_NONE = -1, // means that this is the last block
    BLOCK_NEXT_FREE = -2, // means that this block is not even allocated for anything // TODO rename
};

////////////////////////// struct

struct storage{
    int num_disks;
    struct disk *disks;

    // circular buffer
    int free_blocks_start; // at which idx the next free block is located
    int free_blocks_end; // at which idx the last free block is located
    int free_blocks_size; // needed for `end = (end+1) % size`, and also for `start`
    struct block **free_blocks;

    int num_files;
    struct file *files;
};

struct disk{
    FILE *location;
    int num_blocks;
    struct block *blocks;
};

typedef long int disk_offset_t; // offset for `fseek` // what in the actual fuck this was supposed to be limited to 2GiB but in my tests it works perfectly fine with 8GiB
// TODO don't use regular `int` since this will be written to disk

struct block_location{
    int8_t disk_idx; // which index is the disk // up to `2**8 == 256` disks
    disk_offset_t offset; // at which offset on the disk is this block located
};

struct block_info{
    struct block_location location;
    struct block_location next;
};

// TODO delete?
struct block_data{
    char data[BLOCKSIZE_DATA];
};

struct block{
    struct block_info info;
    struct block_data *data; // TODO no need?
};

struct file{
    char name[FILE_NAME_SIZE];
    // int32_t first_block_disk_idx;
    // disk_offset_t first_block_offset; // value is `BLOCK_NEXT_FREE` when file is unallocated
    struct block_location first_block;
};

////////////////////////// function

// init, deinit
int gfs_init(int disks, char **locations);
void gfs_deinit(void);
// formatting and syncing
int gfs_format(void);
int gfs_sync(void);
// specialised syncing
int gfs_sync_block(struct block *block);
int gfs_sync_file(struct file *file);
// utilty for files
struct file *gfs_find_file_by_name(char file_name[FILE_NAME_SIZE]); // TODO rename to `find_file`
// file creation
int gfs_create_file(char file_name[FILE_NAME_SIZE]);
