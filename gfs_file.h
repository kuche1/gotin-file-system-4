
struct file{
    char name[FILE_NAME_SIZE];
    struct storage_location location; // location of file metadata
    struct storage_location first_block; // location of first block
};

// sync
int gfs_sync_file(struct file *file); // TODO split into `sync_info` and `sync_data`

// find unallocated
int gfs_find_unallocated_file(struct file **file); // pointer to pointer is sufficient since all files are loaded int omemory (as of right now)

// find by name
struct file *gfs_find_file(char file_name[FILE_NAME_SIZE]); // pointer is sufficient since all files are loaded in memory (as of right now)
// returns NULL if file cannot be found

// file creation
int gfs_create_file(char file_name[FILE_NAME_SIZE]);

// file deletion
int gfs_delete_file(char file_name[FILE_NAME_SIZE]);
