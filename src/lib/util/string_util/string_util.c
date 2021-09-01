#include "string_util.h"
#include <ctype.h>
#include <string.h>

char *fmt_str(void *data) {
    char *str = malloc(strlen(data) + 1);
    strcpy(str, data);
    return str;
}

int get_first_position(const char *str) {
    int i = 0;
    while (isspace(str[i])) {
        i += 1;
    }
    return (i);
}

int get_last_position(char *str) {
    int i = strlen(str) - 1;
    while (isspace(str[i])) {
        i -= 1;
    }
    return (i);
}

char *str_trim(char *str) {
    char *trim = NULL;
    int i, len, start, end;
    if (str != NULL) {
        start = get_first_position(str);
        end = get_last_position(str);
        len = (get_last_position(str) - get_first_position(str)) + 2; // the + 1 is for the termination of the string
        trim = malloc(sizeof(char) * len);
        i = 0;
        while (start <= end) {
            char c = (char) str[start];
            start++;
            trim[i++] = c;
        }
        trim[i] = '\0';
    }
    return (trim);
}

bool str_equals(char *self, char *other) {
    return strcmp(self, other) == 0;
}