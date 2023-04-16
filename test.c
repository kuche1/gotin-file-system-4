
#include "gfs.h"

int main(void){
    int err = 0;

    char *disks[] = {"test_disk_1", "test_disk_2"};
    int num_disks = sizeof(disks) / sizeof(*disks);

    if((err = gfs_init(num_disks, disks))){
        return err;
    }

    // if((err = gfs_format())){
    //     gfs_deinit();
    //     return err;
    // }

    char file_name[FILE_NAME_SIZE] = {'g', 'a', 'y', 's', 'f'}; // used to be `gaysex`

    if((err = gfs_create_file(file_name))){
        printf("could not create file\n");
    }else{
        printf("created file\n");
    }

    // if((err = gfs_delete_file(file_name))){
    //     printf("could not delete file\n");
    // }else{
    //     printf("deleted file\n");
    // }

    // printf("Press enter\n");
    // getchar();

    gfs_deinit();

    return 0;
}
