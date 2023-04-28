
#ifndef _H_GFS_H_
#define _H_GFS_H_

// all in bytes
#define FILE_LEN_SIZE 1
#define FILE_DELETED_SIZE 1
#define FILE_LEN_NAME_SIZE 1
#define FILE_LEN_DATA_SIZE 1

// TODO might be able to can make this obsolete
#define FILE_METADATA_SIZE ( \
    FILE_LEN_SIZE + \
    FILE_DELETED_SIZE + \
    FILE_LEN_NAME_SIZE + \
    FILE_LEN_DATA_SIZE \
)

#pragma pack(1)
typedef struct{
    unsigned int len     :8*FILE_LEN_SIZE;
    unsigned int deleted :8*FILE_DELETED_SIZE;
    unsigned int len_name:8*FILE_LEN_NAME_SIZE;
    unsigned int len_data:8*FILE_LEN_DATA_SIZE;
}file_metadata_t;

//#pragma pack(1)
typedef struct{
    file_metadata_t metadata;
    char name[512]; // TODO moje da ne e to4no
    char data[512];
}file_data_t;

void file_calc_len(file_data_t *file);

#endif
