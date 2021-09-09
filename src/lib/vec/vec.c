#include "vec.h"

const int MAX_VEC_ELEM_SIZE = sizeof(void *);

Vec *new_vec_with_capacity(uint64 elem_size, uint64 capacity) {
    if (elem_size == 0) {
        perror("You shouldn't use vec with elements without size!\n");
        exit(1);
    }
    if (elem_size > MAX_VEC_ELEM_SIZE) {
        fprintf(stderr, "You shouldn't use vec with elements with size greater than %d\n", MAX_VEC_ELEM_SIZE);
        exit(1);
    }
    Vec *vec = malloc(sizeof(Vec));
    vec->_arr = malloc(sizeof(void *) * capacity);
    vec->_elem_size = elem_size;
    vec->_capacity = capacity;
    vec->length = 0;
    vec->drop = vec_drop;
    vec->get = vec_get;
    vec->take = vec_take;
    vec->push = vec_push;
    vec->pop = vec_pop;
    vec->shift = vec_shift;
    vec->print = vec_print;
    vec->fmt = vec_fmt;
    vec->first = vec_first;
    vec->last = vec_last;
    vec->take_arr = vec_take_arr;
    return vec;
}

Vec *new_vec(uint64 elem_size) {
    return new_vec_with_capacity(elem_size, INITIAL_VEC_CAPACITY);
}

void resize_vec(Vec *vec) {
    if (vec->length + 1 > vec->_capacity) {
        uint64 capacity_by_initial = vec->_capacity / INITIAL_VEC_CAPACITY;
        uint64 new_capacity =
                ((capacity_by_initial ? capacity_by_initial : 1) * INITIAL_VEC_CAPACITY) << 1;
        void **new_arr = realloc(vec->_arr, new_capacity * sizeof(void *));
        if (new_arr == NULL) {
            perror("vec resizing failed!\n");
            free(vec->_arr);
            free(vec);
            exit(1);
        }
        vec->_capacity = new_capacity;
        vec->_arr = new_arr;
    }
}

void vec_push(Vec *vec, void *elem) {
    resize_vec(vec);
    vec->_arr[vec->length] = elem;
    vec->length += 1;
}

void *vec_get(Vec *vec, uint64 idx) {
    if (idx <= vec->length - 1) {
        return vec->_arr[idx];
    } else {
        return NULL;
    }
}

void vec_drop(Vec *vec) {
    if (vec->_arr != NULL) {
        free(vec->_arr);
    }
    free(vec);
}

void *vec_pop(Vec *vec) {
    if (vec->length) {
        void *elem = vec->_arr[vec->length - 1];
        vec->_arr[vec->length - 1] = NULL;
        vec->length -= 1;
        return elem;
    }
    return NULL;
}

void *vec_shift(Vec *vec) {
    if (vec->length) {
        void *elem = vec->_arr[0];
        int i;
        for (i = 0; i < vec->length - 1; i++) {
            vec->_arr[i] = vec->_arr[i + 1];
        }
        vec->length -= 1;
        return elem;
    }
    return NULL;
}

char *array_fmt(uint64 size, void **array, char *(*fmt_elem)(void *) ) {
    uint64 str_length = 2;
    char **elems_str = malloc(sizeof(char *) * size);
    int i;
    for (i = 0; i < size; i++) {
        char *elem_str = fmt_elem(array[i]);
        elems_str[i] = elem_str;
        str_length += strlen(elem_str) + 2;
    }
    char *str = malloc(sizeof(char) * (str_length + 1));
    str[0] = '[';
    str[1] = '\0';
    for (i = 0; i < size; i++) {
        strcat(str, elems_str[i]);
        free(elems_str[i]);
        if (i < size - 1)
            strcat(str, ", ");
    }
    strcat(str, "]");
    free(elems_str);
    return str;
}

char *vec_fmt(Vec *vec, char *(*fmt_elem)(void *) ) {
    return array_fmt(vec->length, vec->_arr, fmt_elem);
}

void vec_print(Vec *vec, char *(*fmt_elem)(void *) ) {
    char *str = vec_fmt(vec, fmt_elem);
    printf("%s\n", str);
    free(str);
}

void *vec_take(Vec *vec, uint64 idx) {
    if (idx < vec->length) {
        void *elem = vec->_arr[idx];
        int i = idx;
        for (; i < vec->length - 1; i++) {
            vec->_arr[i] = vec->_arr[i + 1];
        }
        vec->_arr[i] = NULL;
        vec->length -= 1;
        return elem;
    }
    return NULL;
}

void *vec_first(Vec *vec) {
    return vec->length ? vec->_arr[0] : NULL;
}
void *vec_last(Vec *vec) {
    return vec->length ? vec->_arr[vec->length - 1] : NULL;
}

void **vec_take_arr(Vec *vec) {
    void **arr = vec->_arr;
    vec->_arr = NULL;
    free(vec);
    return arr;
}