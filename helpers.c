
#include <stdio.h>

#include "helpers.h"

void print_str(int len, char *str){
    for(int i=0; i<len; ++i){
        printf("%c", str[i]);
    }
}
