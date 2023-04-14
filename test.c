
#include "gfs.h"

int main(void){
    int err = 0;

    char *disks[] = {"test_disk_1", "test_disk_2"};
    int num_disks = sizeof(disks) / sizeof(*disks);

    if((err = gfs_init(num_disks, disks))){
        return err;
    }

    if((err = gfs_format())){
        goto err;
    }

    // printf("Press enter\n");
    // getchar();

err:
    gfs_deinit();

    return 0;
}
