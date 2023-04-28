#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"

struct superblock sb;
struct inode *inodes;
struct disk_block *dbs;

int find_empty_inode(){
    for(int i=0; i<sb.num_inodes; ++i){
        if(inodes[i].first_block == -1){
            return i;
        }
    }
    return -1;
}

int find_empty_block(){
    for(int i=0; i<sb.num_blocks; ++i){
        if(dbs[i].next_block_num == -1){
            return i;
        }
    }
    return -1;
}

void shorten_file(int bn){
    int nn = dbs[bn].next_block_num;
    if(nn >= 0){
        shorten_file(nn);
    }
    dbs[bn].next_block_num = -1;
}

int get_block_num(int file, int offset){
    int togo = offset;
    int bn = inodes[file].first_block;
    while(togo > 0){
        bn = dbs[bn].next_block_num;
        togo -= 1;
    }
    return bn;
}


void create_fs(){
    sb.num_inodes = 10;
    sb.num_blocks = 100;
    sb.size_blocks = sizeof(struct disk_block);

    inodes = malloc(sizeof(struct inode) * sb.num_inodes); // TODO check
    for(int i=0; i<sb.num_inodes; ++i){
        inodes[i].size = -1;
        inodes[i].first_block = -1;
        strcpy(inodes[i].name, "emptyfi");
    }

    dbs = malloc(sizeof(struct disk_block) * sb.num_blocks); // TODO check
    for(int i=0; i<sb.num_blocks; ++i){
        dbs[i].next_block_num = -1;
        // don't init the data, it doesn't matter
    }
}

void mount_fs(){
    FILE *file;

    file = fopen("fs_data", "r");

    // superblock
    fread(&sb, sizeof(struct superblock), 1, file);

    // inodes
    inodes = malloc(sizeof(struct inode) * sb.num_inodes);
    // for(int i=0; i<sb.num_inodes; ++i){
    //     fread(&(inodes[i]), sizeof(struct inode), 1, file);
    // }
    fread(inodes, sizeof(struct inode), sb.num_inodes, file);

    // db
    dbs = malloc(sizeof(struct disk_block) * sb.num_blocks);
    // for(int i=0; i<sb.num_blocks; ++i){
    //     fread(&(dbs[i]), sizeof(struct disk_block), 1, file);
    // }
    fread(dbs, sizeof(struct disk_block), sb.num_blocks, file);

    fclose(file);
}

void sync_fs(){
    FILE *file;

    file = fopen("fs_data", "w+");

    // superblock
    fwrite(&sb, sizeof(struct superblock), 1, file);

    // inodes
    // for(int i=0; i<sb.num_inodes; ++i){
    //     fwrite(&(inodes[i]), sizeof(struct inode), 1, file);
    // }
    fwrite(inodes, sizeof(struct inode), sb.num_inodes, file);

    // dbs
    // for(int i=0; i<sb.num_blocks; ++i){
    //     fwrite(&(dbs[i]), sizeof(struct disk_block), 1, file);
    // }
    fwrite(dbs, sizeof(struct disk_block), sb.num_blocks, file);

    fclose(file);
}

int allocate_file(char name[8]){
    // find empty inode
    int in = find_empty_inode();
    
    // find and claim a disk block
    int block = find_empty_block();

    // claim them
    inodes[in].first_block = block;
    dbs[block].next_block_num = -2;

    strcpy(inodes[in].name, name);
    
    // return file descriptor
    return in;
}

void set_filesize(int filenum, int size){
    // how many blocks should we have
    int num = (size+BLOCKSIZE-1) / BLOCKSIZE; // round up
    int bn = inodes[filenum].first_block;
    num -= 1;
    // grow the file if necessary
    while(num > 0){
        // check next block number
        int next_num = dbs[bn].next_block_num;
        if(next_num == -2){
            int empty = find_empty_block();
            dbs[bn].next_block_num = empty;
            dbs[empty].next_block_num = -2;
        }
        bn = dbs[bn].next_block_num;
        num -= 1;
    }

    // shorten if necessary
    shorten_file(bn);
    dbs[bn].next_block_num = -2;
}

void write_byte(int filenum, int pos, char *data){
    // calculate which block
    int relative_block = pos / BLOCKSIZE;

    // find the block number
    int bn = get_block_num(filenum, relative_block);

    // calculate the offset in the block
    int offset = pos % BLOCKSIZE;

    // write the data
    dbs[bn].data[offset] = *data;
}

void print_fs(){
    printf("Superblock info\n");
    printf("\tnum inodes %d\n", sb.num_inodes);
    printf("\tnum blocks %d\n", sb.num_blocks);
    printf("\tsize blocks %d\n", sb.size_blocks);

    printf("inodes\n");
    for(int i=0; i<sb.num_inodes; ++i){
        printf("\tsize:%d block:%d name:%s\n", inodes[i].size, inodes[i].first_block, inodes[i].name);
    }

    for(int i=0; i<sb.num_blocks; ++i){
        printf("\tblock-num:%d next-block:%d\n", i, dbs[i].next_block_num);
    }
}
