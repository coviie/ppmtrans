/*
 *      a2plain.c
 *      by Jia Wen Goh (jgoh01) & Sean Ong (song02), 10/6/2017
 *
 *      - Implements the method suite for plain (i.e. non-blocked) 2D arrays
 */

#include <stdlib.h>
#include <a2plain.h>
#include "uarray2.h"

typedef A2Methods_UArray2 A2;

/*---------------------------------------------------------------
 |             Constructors / Destructors                       |
 *--------------------------------------------------------------*/
/* [Name]:       new
 * [Purpose]:    Allocates memory for a 2D array with user-specified
 *               dimensions
 * [Parameters]: 3 ints (width, height, size of each data elem in bytes)
 * [Return]:     Opaque representation of a 2D array
 */
static A2 new(int width, int height, int size)
{
        return UArray2_new(width, height, size);
}

/* [Name]:       new_with_blocksize
 * [Purpose]:    Allocates memory for a 2D array with user-specified
 *               dimensions, ignoring the blocksize (because this is the plain
 *               method suite)
 * [Parameters]: 4 ints (width, height, size of each data elem in bytes,
 *               blocksize)
 * [Return]:     Opaque representation of a 2D array
 */
static A2 new_with_blocksize(int width, int height, int size, int blocksize)
{
        (void)blocksize;
        return UArray2_new(width, height, size);
}

/* [Name]:       a2free
 * [Purpose]:    Frees the heap allocated memory of 2D array
 * [Parameters]: 1 A2* (array2p)
 * [Return]:     void
 */
static void a2free(A2 *array2p)
{
        UArray2_free((UArray2_T *) array2p);
}

/*---------------------------------------------------------------
 |                       Metadata Functions                     |
 *--------------------------------------------------------------*/
/* [Name]:       width
 * [Purpose]:    Returns width of 2D array
 * [Parameters]: 1 A2 (array2)
 * [Return]:     Width of array2
 */
static int width(A2 array2)
{
        return UArray2_width(array2);
}

/* [Name]:       height
 * [Purpose]:    Returns height of 2D array
 * [Parameters]: 1 A2 (array2)
 * [Return]:     Height of array2
 */
static int height(A2 array2)
{
        return UArray2_height(array2);
}

/* [Name]:       size
 * [Purpose]:    Returns size of each data element within the 2D array
 * [Parameters]: 1 A2 (array2)
 * [Return]:     Size of array2
 */
static int size(A2 array2)
{
        return UArray2_size(array2);
}

/* [Name]:       blocksize
 * [Purpose]:    Returns default value, because this is the plain methods suite
 * [Parameters]: 1 A2 (array2)
 * [Return]:     1
 */
static int blocksize(A2 array2)
{
        (void) array2;
        return 1;
}

/*---------------------------------------------------------------
 |                      Access Functions                        |
 *--------------------------------------------------------------*/
/* [Name]:       at
 * [Purpose]:    Returns the element at the given (i, j)
 * [Parameters]: 1 A2 (array2), 2 ints (i and j)
 * [Return]:     A2Methods_Object* pointing to element at given index
 */
static A2Methods_Object *at(A2 array2, int i, int j)
{
        return UArray2_at(array2, i, j);
}

/* Private definition for apply function */
typedef void applyfun(int i, int j, UArray2_T array2, void *elem, void *cl);

/* [Name]:       map_col_major
 * [Purpose]:    Map function for A2 that does col-major mapping
 * [Parameters]: 1 A2 (array2), 1 apply function, 1 void* (closure)
 * [Return]:     void
 */
static void map_col_major(A2 array2, A2Methods_applyfun apply, void *cl)
{
        UArray2_map_col_major(array2, (applyfun *) apply, cl);
}

/* [Name]:       map_row_major
 * [Purpose]:    Map function for A2 that does row-major mapping
 * [Parameters]: 1 A2 (array2), 1 apply function, 1 void* (closure)
 * [Return]:     void
 */
static void map_row_major(A2 array2, A2Methods_applyfun apply, void *cl)
{
        UArray2_map_row_major(array2, (applyfun *) apply, cl);
}

/* Private struct definition for small map closure */
struct small_closure {
        A2Methods_smallapplyfun *apply;
        void *cl;
};

/* [Name]:       apply_small
 * [Purpose]:    Wrapper function for small map functions
 * [Parameters]: 2 ints (i, j), 1 UArray2_T (array2), 2 void* (elem & closure)
 * [Return]:     void
 */
static void apply_small(int i, int j, UArray2_T array2, void *elem, void *vcl)
{
        struct small_closure *cl = vcl;
        (void)i;
        (void)j;
        (void)array2;
        cl->apply(elem, cl->cl);
}

/* [Name]:       small_map_col_major
 * [Purpose]:    Small map function for A2 that does col-major mapping
 * [Parameters]: 1 A2 (array2), 1 small apply function, 1 void* (closure)
 * [Return]:     void
 */
static void small_map_col_major(A2 a2, A2Methods_smallapplyfun apply, void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_col_major(a2, apply_small, &mycl);
}

/* [Name]:       small_map_row_major
 * [Purpose]:    Small map function for A2 that does row-major mapping
 * [Parameters]: 1 A2 (array2), 1 small apply function, 1 void* (closure)
 * [Return]:     void
 */
static void small_map_row_major(A2 a2, A2Methods_smallapplyfun apply, void *cl)
{
        struct small_closure mycl = { apply, cl };
        UArray2_map_row_major(a2, apply_small, &mycl);
}

/* Private struct containing pointers to the functions */
static struct A2Methods_T uarray2_methods_plain_struct = {
        new,
        new_with_blocksize,
        a2free,
        width,
        height,
        size,
        blocksize,
        at,
        map_row_major,
        map_col_major,
        NULL,                   // map_block_major
        map_row_major,          // map_default
        small_map_row_major,
        small_map_col_major,
        NULL,                   // small_map_block_major
        small_map_row_major,    // small_map_default
};

/* Payoff: exported pointer to the struct */
A2Methods_T uarray2_methods_plain = &uarray2_methods_plain_struct;