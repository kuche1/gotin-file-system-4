
#include <stdlib.h>

#define MALLOC_AND_SET(ptr, count) ((ptr) = malloc(sizeof(*(ptr)) * count))

#define DEMALLOC(ptr) {\
    free(ptr);\
    (ptr) = NULL;\
}

// just like the regular `fclose`, but does not crash if `NULL` is passed
#define FCLOSE(file) {\
    if(file){\
        fclose(file);\
    }\
}

// TODO fseek that works with big values
// and ftell that works with big values
// then, the metadata needs to be updated

// #define FWRITE_CHECK(item, file) (fwrite(item, sizeof(*item), 1, file) != 1)

#define FWRITE(item_ptr, count, file) fwrite(item_ptr, sizeof(*(item_ptr)), count, file)

#define FREAD(item_ptr, count, file) fread(item_ptr, sizeof(*(item_ptr)), count, file)
