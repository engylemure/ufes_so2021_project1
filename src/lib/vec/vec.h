#ifndef VEC_H
#define VEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * author: Jordao Rodrigues Oliveira Rosario
 * Vector implementation to more easily handle with resisable arrays
 * it uses the Rust Vec implementation as an inspiration.
 */
#define INITIAL_VEC_CAPACITY 64

typedef long unsigned int uint64;


/**
 * Vec
 * description: struct that stores an internall array, it's capacity and 
 * the used capacity or actual length used and some pointer to functions
 * since the most close to a generic way to handle different types is by using
 * 'void*' elements I've used this type for the internal array.
 */
typedef struct vec {
    void **_arr;
    uint64 _elem_size;
    uint64 _capacity;
    uint64 length;
} Vec;

Vec *new_vec_with_capacity(uint64 elem_size, uint64 capacity);

Vec *new_vec(uint64 elem_size);

// function to push an element to the end of the vector
void vec_push(Vec *vec, void *elem);
// function to get an element by it's index and add some internal checks that
// would prevent us to cause some out of bound access it return's NULL case it isn't on 
// on the bound's of the arr;
void *vec_get(Vec *vec, uint64 idx);
// function to remove an element for the idx moving all element's on the 
// right part of the idx to it's new position it return's NULL case the idx is out of
// bound
void *vec_take(Vec *vec, uint64 idx);

// function to deallocate this struct
void vec_drop(Vec *vec);
// function to pop an element of the end of the array it return's NULL 
// in the case it's empty
void *vec_pop(Vec *vec);

// function to remove the first element of the start of the array
// moving all element's after the first position to it's new position
void *vec_shift(Vec *vec);
// function to get an reference to the first element of the array
// without removing it 
void *vec_first(Vec *vec);
// function to get an reference to the last element of the array
// without removing it
void *vec_last(Vec *vec);
// function to print the array into a human-readable structure
// it uses a function to format each of the element's
void vec_print(Vec *vec, char *(*fmt_elem)(void *) );
// function to format and return a string of this array into a human-readable structure
// it uses a function to format each of the element's
char *vec_fmt(Vec *vec, char *(*fmt_elem)(void *) );
char *array_fmt(uint64 size, void **array, char *(*fmt_elem)(void *) );
// function to take the internal array of this struct
// and deallocate the struct itself;
void **vec_take_arr(Vec *vec);

#endif