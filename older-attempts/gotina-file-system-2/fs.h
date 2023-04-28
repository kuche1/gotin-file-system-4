
#define BLOCKSIZE 512

// meta information about the filesystem
struct superblock{
    int num_inodes;
    int num_blocks;
    int size_blocks;
};

struct inode{
    int size;
    int first_block;
    char name[8];
};

struct disk_block{
    int next_block_num;
    char data[BLOCKSIZE];
};

void create_fs(); // initialize new filesystem
void mount_fs(); // load a filesystem
void sync_fs(); // write the filesystem

// return filenumber
int allocate_file(char name[8]);
void set_filesize(int filenum, int size);
void write_byte(int filenum, int pos, char *data);
// TODO delete
// TODO read_byte

void print_fs(); // print info about the filesystem
