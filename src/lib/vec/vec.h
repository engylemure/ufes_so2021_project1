/**
 * author: Jordao Rodrigues Oliveira Rosario
 * Vector implementation to more easily handle with resisable arrays
 * it uses the Rust Vec implementation as an inspiration, and was created to 
 * provide a safer API to handle with arrays.
 */
#ifndef VEC_H
#define VEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



/**
 * @brief Initial capacity of our vector
 */
#define INITIAL_VEC_CAPACITY 64

typedef long unsigned int uint64;


/**
 * @brief struct that stores an internall array, it's capacity and 
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

/**
 * @brief function to create a new vector with the provided capacity.
 * @param elem_size size of it's elements, it's used to guarantee that the internal array
 * can hold the provided element's inside of it without causing an overflow.
 * @param capacity capacity for this vector
 * @return Vec* 
 */
Vec *new_vec_with_capacity(uint64 elem_size, uint64 capacity);

/**
 * @brief function to create a new vector
 * @param elem_size 
 * @return Vec* 
 */
Vec *new_vec(uint64 elem_size);

/**
 * @brief function to push an element to the end of the vector
 * @param vec vector
 * @param elem element to be pushed into the vector.
 */
void vec_push(Vec *vec, void *elem);

/**
 * @brief function to get an element by it's index and add some internal checks that
 * would prevent us to cause some out of bound access, in that case we return a NULL value.
 * @param vec 
 * @param idx 
 * @return void* 
 */
void *vec_get(Vec *vec, uint64 idx);
/**
 * @brief function to remove an element for the idx moving all element's on the 
 * right part of the idx to it's new position it return's NULL case the idx is out of
 * bound preventing.
 * @param vec 
 * @param idx 
 * @return void* 
 */
void *vec_take(Vec *vec, uint64 idx);

/**
 * @brief function to deallocate the Vec
 * @param vec 
 */
void vec_drop(Vec *vec);

/**
 * @brief function to pop an element of the end of the vector it return's NULL 
 * in the case it's empty
 * @param vec 
 * @return void* element removed from the end of the vector
 */
void *vec_pop(Vec *vec);

/**
 * @brief function to remove the first element of the start of the vector
 * moving all element's after the first position to it's new position it's a O(n-1) process
 * considering the length of the vector.
 * @param vec 
 * @return void* 
 */
void *vec_shift(Vec *vec);

/**
 * @brief function to get a reference to the first element of the vector
 * without removing it.
 * @param vec 
 * @return void* first element if it exists otherwise NULL
 */
void *vec_first(Vec *vec);

/**
 * @brief function to get a reference to the last element of the vector
 * without removing it.
 * @param vec 
 * @return void* last element if it exists otherwise NULL
 */
void *vec_last(Vec *vec);

/**
 * @brief function to print the vector into a human-readable structure 
 *  it uses a function to format each of the element's.
 * @param vec vector to be printed.
 * @param fmt_elem  function to format each of the elements.
 */
void vec_print(Vec *vec, char *(*fmt_elem)(void *) );

/**
 * @brief function to format and return a string of this vector into a human-readable structure
 * it uses a function to format each of the element's.
 * @param vec vector to be formatted
 * @param fmt_elem function to format each of the elements.
 * @return char* 
 */
char *vec_fmt(Vec *vec, char *(*fmt_elem)(void *) );

/**
 * @brief function to format and return a string of this array into a human-readable structure
 * it uses a function to format each of the element's.
 * @param size size to be used in the iteration of the array we are considering that this param should be equal
 * or lower than the length of the array otherwise it will cause a Undefined Behavior
 * @param array array to be used
 * @param fmt_elem function to format each of the elements
 * @return char* 
 */
char *array_fmt(uint64 size, void **array, char *(*fmt_elem)(void *) );

/**
 * @brief function to take the internal array of this vector
 * and deallocate the struct itself;
 * @param vec 
 * @return void** 
 */
void **vec_take_arr(Vec *vec);

#endif