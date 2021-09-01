#ifndef LIB_STRING_UTIL_H
#define LIB_STRING_UTIL_H

#include "../vec/vec.h"
#include <stdbool.h>

#define BUFFER_MAX_SIZE 1024

/*
    Code for more easily handle string manipulation.
    author: Jordao Rosario <jordao.rosario01@gmail.com>
*/

char *str_trim(char *str);

char *fmt_str(void *data);

bool str_equals(char *self, char *other);

#endif