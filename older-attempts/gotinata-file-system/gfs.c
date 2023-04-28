
// gcc gfs.c -Werror -Wall -pedantic && sudo ./a.out

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <limits.h>

#include "gfs.h"

///////////////////////////////////// errors

enum{
    ERR_CANT_OPEN = 1,
    ERR_CANT_READ,
    ERR_CANT_WRITE,
    ERR_FILE_WITH_SAME_NAME_ALREADY_EXISTS,
};

///////////////////////////////////// print

void print_str(char *str, int len){
    for(int i=0; i<len; ++i){
        printf("%c", str[i]);
    }
}

///////////////////////////////////// device IO

FILE *open_safe(char *device, char *mode){
    FILE *f = fopen(device, mode);
    if(!f){
        exit(ERR_CANT_OPEN);
    }
    return f;
}

void write_safe(const void *ptr, size_t size, size_t nmemb, FILE *stream){
    int written = fwrite(ptr, size, nmemb, stream);
    if(written != nmemb){
        exit(ERR_CANT_WRITE);
    }
}

void read_safe(void *ptr, size_t size, size_t nmemb, FILE *stream){
    int read = fread(ptr, size, nmemb, stream);
    if(read != nmemb){
        exit(ERR_CANT_READ);
    }
}

///////////////////////////////////// device formatting

// TODO placeholder
void format(char *device){
    int zero = 0;

    FILE *f = open_safe(device, "w");
    for(int i=0; i<1024; ++i){
        write_safe(&zero, sizeof(zero), 1, f);
    }
    fclose(f);
}

///////////////////////////////////// files

file_data_t file_generate(char *name, char *data){ // TODO returning to stack is bad but we'll use it for testing
    file_data_t file;

    file.metadata.deleted = 0;

    file.metadata.len_name = strlen(name); // TODO add check?
    memcpy(file.name, name, file.metadata.len_name);

    file.metadata.len_data = strlen(data); // TODO add check?
    memcpy(file.data, data, file.metadata.len_data);

    file_calc_len(&file);

    return file;
}

void file_calc_len(file_data_t *file){
    file->metadata.len = FILE_METADATA_SIZE - FILE_LEN_SIZE + file->metadata.len_name + file->metadata.len_data;
}

// void file_create(char *device, file_data_t *file){
//     FILE *f = open_safe(device, "w");
//     write_safe(file, FILE_METADATA_SIZE, 1, f);
//     write_safe(file->name, sizeof(char), file->metadata.len_name, f);
//     write_safe(file->data, sizeof(char), file->metadata.len_data, f);
//     fclose(f);
// }

void file_append(file_data_t *file, FILE *device){
    write_safe(file, FILE_METADATA_SIZE, 1, device);
    write_safe(file->name, sizeof(char), file->metadata.len_name, device);
    write_safe(file->data, sizeof(char), file->metadata.len_data, device);
}

// void file_read(char *device, file_data_t *file){
//     FILE *f = open_safe(device, "r");
//     read_safe(file, FILE_METADATA_SIZE, 1, f); // reads the metadata
//     read_safe(file->name, sizeof(char), file->metadata.len_name, f);
//     read_safe(file->data, sizeof(char), file->metadata.len_data, f);
//     fclose(f);
// }

void file_read(file_data_t *file, FILE *device){
    read_safe(file, FILE_METADATA_SIZE, 1, device); // reads the metadata
    read_safe(file->name, sizeof(char), file->metadata.len_name, device);
    read_safe(file->data, sizeof(char), file->metadata.len_data, device);
}

///////////////////////////////////// test

int main(void){

    char *device = "/dev/sdc";
    FILE *f;
    file_data_t file;

    ////////////// raw IO

    char *write = "s2eex";

    printf("write: %s\n", write);

    f = open_safe(device, "w");
    write_safe(write, sizeof(*write), strlen(write), f);
    fclose(f);

    char read[200] = {0};

    f = open_safe(device, "r");
    read_safe(read, sizeof(*read), strlen(write), f);
    fclose(f);

    printf("read: %s\n", read);

    printf("\n");

    ////////////// file creation

    // format(device);

    // str = "asdfg";
    // file.metadata.len_name = strlen(str);
    // memcpy(file.name, str, file.metadata.len_name);
    // str = "aaaaaaseeeex";
    // file.metadata.len_data = strlen(str);
    // memcpy(file.data, str, file.metadata.len_data);
    // file_calc_len(&file);

    // printf("creating file: %s\n", file.name);
    // printf("len total: %d\n", file.metadata.len);

    // file_create(device, &file);

    // file_read(device, &file2);

    // printf("read file: ");
    // print_str(file2.name, file2.metadata.len_name);
    // printf("\n");

    // printf("name len: %d\n", file2.metadata.len_name);
    // printf("data: %s\n", file2.data);
    // printf("data len: %d\n", file2.metadata.len_data);
    // printf("len total: %d\n", file2.metadata.len);

    // printf("\n");

    ////////////// create multiple files

    format(device);

    f = open_safe(device, "w");
    file = file_generate("asdf", "sexSEX53333X");
    file_append(&file, f);
    file = file_generate("asdfdasdadfdsfds", "twrefvty54eyv543X");
    file_append(&file, f);
    fclose(f);

    f = open_safe(device, "r");
    while(1){
        file_read(&file, f);
        if(file.metadata.len == 0){
            break;
        }
        printf("read a file with name: ");
        print_str(file.name, file.metadata.len_name);
        printf("\n");
        printf("and content: ");
        print_str(file.data, file.metadata.len_data);
        printf("\n");
    }
    fclose(f);

    return 0;
}
