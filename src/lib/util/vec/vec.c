#include "vec.h"

const int MAX_VEC_ELEM_SIZE = sizeof(void*);

Vec *new_vec_with_size(uint64 elem_size, uint64 capacity) {
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
    vec->pop_first = vec_pop_first;
    vec->print = vec_print;
    vec->take_arr = vec_take_arr;
    return vec;
}

Vec *new_vec(uint64 elem_size) {
    return new_vec_with_size(elem_size, INITIAL_VEC_CAPACITY);
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

void *vec_pop_first(Vec *vec) {
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

void vec_print(Vec *vec, char *(*fmt_elem)(void *)) {
    printf("[");
    int i;
    for (i = 0; i < vec->length; i++) {
        char *str = fmt_elem(vec->_arr[i]);
        printf("%s", str);
        if (i != vec->length - 1) {
            printf(", ");
        }
        free(str);
    }
    printf("]\n");
}

void* vec_take(Vec *vec, uint64 idx) {
    if (idx < vec->length) {
        void* elem = vec->_arr[idx];
        int i;
        for (i = idx; i < vec->length - 1; i++) {
            vec->_arr[i] = vec->_arr[i+1];
        }
        vec->_arr[i] = NULL;
        vec->length -= 1;
        return elem;
    } 
    return NULL;
}

void **vec_take_arr(Vec *vec) {
    void **arr = vec->_arr;
    vec->_arr = NULL;
    free(vec);
    return arr;
}