
// TODO
//
// use `int32_t` instead of `int` for structs that are written-read from disk
//
// create a function that reads the whole disk and checks for nonsensical data
//
// what I did with `gfs_file.h` is wrong and needs to be done properly

#ifndef _H_GFS_H_
#define _H_GFS_H_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define FILE_NAME_SIZE 5 // TODO this needs to be changed for a dynamic folder name
#define NUMBER_OF_FILES 30 // TODO this is super bad, but it will have to do for now

#define FOLDER_NAME_SIZE FILE_NAME_SIZE

#define BLOCKSIZE_INFO (sizeof(struct storage_location))
#define BLOCKSIZE_DATA 512 // seems reasonable, might need to change this in the future, or might make it a customizable value
#define BLOCKSIZE_INFO_DATA (BLOCKSIZE_INFO + BLOCKSIZE_DATA)

////////////////////////// enum

// TODO use proper enum for this instead of int
// and make sure compiler checks type
enum{
    // generic
    ERR_NONE = 0, // no error
    ERR_MALLOC,
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

    // TODO intead of using a section for files,
    //     we could allocate a new file that contains all data for all files
    int num_files;
    struct file *files;
    struct storage_location file_section; // wil be needed for writing num_files later on
};

////////////////////////// function

// init, deinit
int gfs_init(int disks, char **locations);
void gfs_deinit(void);
// formatting and syncing
int gfs_format(void);
int gfs_sync(void);

////////////////////////// block

#include "gfs_block.h"

////////////////////////// file

#include "gfs_file.h"

////////////////////////// disk

#include "gfs_disk.h"

////////////////////////// folder

#include "gfs_folder.h"

////////////////////////// end

#endif
