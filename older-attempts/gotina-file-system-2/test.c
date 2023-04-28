#include <stdio.h>
#include "fs.h"

int main(void){
    // create_fs();

    mount_fs();

    int file = 1;//allocate_file("another");

    set_filesize(file, 5000);
    
    char data = 'b';
    for(int i=0; i<49; ++i){
        write_byte(file, i*100, &data);
    }

    print_fs();

    sync_fs();
    printf("done\n");
    return 0;
}
