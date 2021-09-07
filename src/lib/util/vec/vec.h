#ifndef VEC_H
#define VEC_H

#include <stdio.h>
#include <stdlib.h>

#define INITIAL_VEC_CAPACITY 64

typedef long unsigned int uint64;

typedef struct vec {
    void **_arr;
    uint64 _elem_size;
    uint64 _capacity;
    uint64 length;

    void (*push)(struct vec *, void *);

    void *(*get)(struct vec *, uint64);
    void *(*take)(struct vec*, uint64);
    void (*drop)(struct vec *);

    void *(*pop)(struct vec *);

    void *(*pop_first)(struct vec *);

    void (*print)(struct vec *, char *(*)(void *));

    void **(*take_arr)(struct vec *);
} Vec;

Vec *new_vec_with_capacity(uint64 elem_size, uint64 capacity);

Vec *new_vec(uint64 elem_size);

void vec_push(Vec *vec, void *elem);

void *vec_get(Vec *vec, uint64 idx);
void* vec_take(Vec *vec, uint64 idx);

void vec_drop(Vec *vec);

void *vec_pop(Vec *vec);

void *vec_pop_first(Vec *vec);

void vec_print(Vec *vec, char *(*fmt_elem)(void *));

void **vec_take_arr(Vec *vec);

#endif