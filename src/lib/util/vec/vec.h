#ifndef VEC_H
#define VEC_H

#include <stdio.h>
#include <stdlib.h>

#define INITIAL_VEC_CAPACITY 64

typedef struct vec {
    void **_arr;
    unsigned int _elem_size;
    unsigned int _capacity;
    unsigned int length;

    void (*push)(struct vec *, void *);

    void *(*get)(struct vec *, unsigned int);

    void (*drop)(struct vec *);

    void *(*pop)(struct vec *);

    void *(*pop_first)(struct vec *);

    void (*print)(struct vec *, char *(*)(void *));

    void **(*take_arr)(struct vec *);
} Vec;

Vec *new_vec_with_capacity(unsigned int elem_size, unsigned int capacity);

Vec *new_vec(unsigned int elem_size);

void vec_push(Vec *vec, void *elem);

void *vec_get(Vec *vec, unsigned int idx);

void vec_drop(Vec *vec);

void *vec_pop(Vec *vec);

void *vec_pop_first(Vec *vec);

void vec_print(Vec *vec, char *(*fmt_elem)(void *));

void **vec_take_arr(Vec *vec);

#endif