
struct folder{
    char name[FOLDER_NAME_SIZE];

    int num_files;
    struct file *files;

    int num_folders;
    struct folder *folders;
};
