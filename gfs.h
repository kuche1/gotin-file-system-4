
// TODO
//
// use `int32_t` instead of `int` for structs that are written-read from disk
//
// create a function that reads the whole disk and checks for nonsensical data

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define FILE_NAME_SIZE 5
#define NUMBER_OF_FILES 30 // TODO this is super bad, but it will have to do for now

#define BLOCKSIZE_INFO (sizeof(struct storage_location))
#define BLOCKSIZE_DATA 512 // seems reasonable, might need to change this in the future, or might make it a customizable value
#define BLOCKSIZE_INFO_DATA (BLOCKSIZE_INFO + BLOCKSIZE_DATA)

////////////////////////// enum

enum{
    // generic
    ERR_MALLOC = 1,
    ERR_FOPEN,
    ERR_FSEEK,
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

// TODO put this into a separate enum and don't merge with the offset
enum{
    BLOCK_NEXT_NONE = -1, // means that this is the last block
    BLOCK_NEXT_FREE = -2, // means that this block is not even allocated for anything // TODO rename
};

////////////////////////// general stuff

typedef long int disk_offset_t; // offset for `fseek`
// what in the actual fuck this was supposed to be limited to 2GiB but in my tests it works perfectly fine with 8GiB
// TODO don't use regular `int` since this will be written to disk

struct storage_location{
    int8_t disk_idx; // which index is the disk // up to `2**8 == 256` disks
    disk_offset_t offset; // at which offset on the disk is this block located
};

struct storage{
    int num_disks;
    int last_allocated_disk; // which disk did we last allocate from
    struct disk *disks;

    int num_files;
    struct file *files;
    struct storage_location file_section; // wil be needed for writing num_files later on
};

////////////////////////// disk

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

////////////////////////// block

struct block_info{
    struct storage_location location;
    struct storage_location next;
};

struct block{
    struct block_info info;
    char data[BLOCKSIZE_DATA];
};

////////////////////////// file

// TODO intead of using a section for files,
//     we could allocate a new file that contains all data for all files
struct file{
    char name[FILE_NAME_SIZE];
    struct storage_location location; // location of file metadata
    struct storage_location first_block; // location of first block
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
// read block
int gfs_read_block(struct block *block, struct storage_location location);
// find unallocated file, block
int gfs_find_unallocated_block(struct block *block);
int gfs_find_unallocated_file(struct file **file); // pointer to pointer is sufficient since all files are loaded int omemory (as of right now)
// find file by properties
struct file *gfs_find_file(char file_name[FILE_NAME_SIZE]); // TODO not ptr
// file creation, deletion
int gfs_create_file(char file_name[FILE_NAME_SIZE]);
int gfs_delete_file(char file_name[FILE_NAME_SIZE]);
