/*
 *      uarray2b.c
 *      by Jia Wen Goh (jgoh01) & Sean Ong (song02), 10/6/2017
 *
 *      - Unboxed 2D array that is polymorphic in data storage
 *      - Implements block-major mapping, with user-specified blocksize
 */

#include "assert.h"
#include "mem.h"
#include "uarray.h"
#include "uarray2.h"
#include "uarray2b.h"
#include <math.h>
#include <stdio.h>

#define T UArray2b_T

/*---------------------------------------------------------------
 |             Private Helpers, No Client Access                |
 *--------------------------------------------------------------*/

/* Initialization Functions */
void UArray2b_init(T uarray2b, int width, int height, int size, int blocksize);
void blocks_init(T uarray2b);
void cell_init(int col, int row, UArray2_T uarray2, void *elem, void *cl);

/* Memory Freeing Function */
void blocks_free(int col, int row, UArray2_T blocks, void *elem, void *cl);

/* Complete struct for UArray2b representation */
struct T {
        int width, height;
        int size;
        int blocksize;
        UArray2_T blocks;
};

/*---------------------------------------------------------------
 |             Constructors / Destructors                       |
 *--------------------------------------------------------------*/
/* [Name]:       UArray2b_new
 * [Purpose]:    Allocates memory for a 2D blocked array with user-specified
 *               dimensions and blocksize
 * [Parameters]: 4 ints (width, height, size of each data elem in bytes),
 *               blocksize)
 * [Return]:     Opaque representation of a UArray2b
 */
T UArray2b_new (int width, int height, int size, int blocksize)
{
        T uarray2b;
        NEW(uarray2b);
        UArray2b_init(uarray2b, width, height, size, blocksize);

        return uarray2b;
}

/* [Name]:       UArray2b_new_64K_block
 * [Purpose]:    Allocates memory for a 2D blocked array with user-specified
 *               dimensions and blocksize that can fit within a 64kb cache, if
 *               possible
 * [Parameters]: 4 ints (width, height, size of each data elem in bytes),
 *               blocksize)
 * [Return]:     Opaque representation of a UArray2b
 */
T UArray2b_new_64K_block(int width, int height, int size)
{
        T   uarray2b;
        int blocksize = 0;
        int num_cells = 64000 / size;

        if (num_cells < 1) {
                blocksize = 1;
        } else if (sqrt(num_cells) > width || sqrt(num_cells) > height) {
                if (height < width) {
                        blocksize = height;
                } else {
                        blocksize = width;
                }
        } else {
                blocksize = (int)sqrt(num_cells);
        }

        NEW(uarray2b);
        UArray2b_init(uarray2b, width, height, size, blocksize);

        return uarray2b;
}

/* [Name]:       UArray2b_init
 * [Purpose]:    Initializes the metadata in the UArray2b representation
 * [Parameters]: 1 T (uarray2b), 4 ints (width, height, size of data element,
 *               blocksize)
 * [Return]:     void
 */
void UArray2b_init(T uarray2b, int width, int height, int size, int blocksize)
{
        assert(width > 0 && height > 0 && size > 0 && blocksize > 0);
        assert(blocksize <= width && blocksize <= height);

        uarray2b->width     = width;
        uarray2b->height    = height;
        uarray2b->size      = size;
        uarray2b->blocksize = blocksize;

        blocks_init(uarray2b);
}

/* [Name]:       blocks_init
 * [Purpose]:    Initializes each block within the UArray2b
 * [Parameters]: 1 T (uarray2b)
 * [Return]:     void
 */
void blocks_init(T uarray2b)
{
        int blocks_h = (int)ceil((double)uarray2b->height /
                                 (double)uarray2b->blocksize);
        int blocks_w = (int)ceil((double)uarray2b->width /
                                 (double)uarray2b->blocksize);

        uarray2b->blocks = UArray2_new(blocks_w, blocks_h, sizeof(UArray_T));
        UArray2_map_row_major(uarray2b->blocks, cell_init, uarray2b);
}

/* [Name]:       cell_init
 * [Purpose]:    Initializes each block within the UArray2b as a UArray_T
 * [Parameters]: 2 ints (col and row coordinates of the current block),
 *               UArray2_T (uarray2 representation of all blocks),
 *               2 void* (block at (col, row), closure)
 * [Return]:
 */
void cell_init(int col, int row, UArray2_T uarray2, void *elem, void *cl)
{
        (void) uarray2;
        (void) col;
        (void) row;

        T uarray2b     = (T) cl;
        UArray_T *buff = (UArray_T *)elem;
        int blocksize  = uarray2b->blocksize;
        int size       = uarray2b->size;

        *buff = UArray_new(blocksize * blocksize, size);

        assert(buff != NULL);
}

/* [Name]:       UArray2b_free
 * [Purpose]:    Frees the heap allocated memory of uarray2b
 * [Parameters]: 1 T* (uarray2b)
 * [Return]:     void
 */
void UArray2b_free(T *uarray2b)
{
        assert(uarray2b != NULL && *uarray2b != NULL);

        UArray2_map_row_major((*uarray2b)->blocks, blocks_free, NULL);
        UArray2_free(&((*uarray2b)->blocks));

        FREE(*uarray2b);
}

/* [Name]:       blocks_free
 * [Purpose]:    Frees the cells in each block of UArray2b
 * [Parameters]: 2 ints (col and row coordinates of the current block),
 *               UArray2_T (uarray2 representation of all blocks),
 *               2 void* (block at (col, row), closure)
 * [Return]:     void
 */
void blocks_free(int col, int row, UArray2_T blocks, void *elem, void *cl)
{
        (void) cl;
        (void) col;
        (void) row;
        (void) blocks;

        UArray_free(elem);
}

/*---------------------------------------------------------------
 |             UArray2b Metadata Functions                      |
 *--------------------------------------------------------------*/
/* [Name]:       UArray2b_width
 * [Purpose]:    Returns width of uarray2b
 * [Parameters]: 1 T (uarray2b)
 * [Return]:     Width of uarray2b
 */
int UArray2b_width(T uarray2b)
{
        assert(uarray2b != NULL);
        return uarray2b->width;
}

/* [Name]:       UArray2b_height
 * [Purpose]:    Returns height of uarray2b
 * [Parameters]: 1 T (uarray2b)
 * [Return]:     Height of uarray2b
 */
int UArray2b_height (T uarray2b)
{
        assert(uarray2b != NULL);
        return uarray2b->height;
}

/* [Name]:       UArray2b_size
 * [Purpose]:    Returns the size of each data element of uarray2b
 * [Parameters]: 1 T (uarray2b)
 * [Return]:     Size of each data element in uarray2b
 */
int UArray2b_size (T uarray2b)
{
        assert(uarray2b != NULL);
        return uarray2b->size;
}

/* [Name]:       UArray2b_blocksize
 * [Purpose]:    Returns blocksize of uarray2b
 * [Parameters]: 1 T (uarray2b)
 * [Return]:     Blocksize of uarray2b
 */
int UArray2b_blocksize(T uarray2b)
{
        assert(uarray2b != NULL);
        return uarray2b->blocksize;
}

/*---------------------------------------------------------------
 |                      Access Functions                        |
 *--------------------------------------------------------------*/
/* [Name]:       UArray2b_at
 * [Purpose]:    Returns the element at the given (col, row)
 * [Parameters]: 1 T (uarray2b), 2 ints (col and row)
 * [Return]:     void* pointing to element at given index
 */
void *UArray2b_at(T uarray2b, int col, int row)
{
        assert(uarray2b != NULL);
        assert(col < uarray2b->width && row < uarray2b->height);

        UArray2_T blocks = uarray2b->blocks;
        int blocksize = uarray2b->blocksize;
        UArray_T *curr_block = UArray2_at(blocks, col / blocksize,
                                          row / blocksize);
        return UArray_at(*curr_block,
                         (blocksize * (row % blocksize)) + (col % blocksize));
}

/* [Name]:       UArray2b_map
 * [Purpose]:    Map function for UArray2b that does block-major mapping
 * [Parameters]: 1 T (uarray2b), 1 apply function, 1 void* (closure)
 * [Return]:     void
 */
void UArray2b_map(T uarray2b, void apply(int col, int row, T uarray2b,
                                        void *elem, void *cl), void *cl)
{
        UArray2_T uarray2 = uarray2b->blocks;
        int blocksize  = uarray2b->blocksize;
        int height     = uarray2b->height;
        int width      = uarray2b->width;
        int uarray2_w  = UArray2_width(uarray2);
        int uarray2_h  = UArray2_height(uarray2);

        int blk_h = 0;
        int blk_w = 0;

        for (int blk_row = 0; blk_row < uarray2_h; blk_row++) {
                if (blk_row == uarray2_h - 1 && (height % blocksize) != 0) {
                        blk_h = height % blocksize;
                } else {
                        blk_h = blocksize;
                }

                for (int blk_col = 0; blk_col < uarray2_w; blk_col++) {
                        if (blk_col == uarray2_w - 1 &&
                            (width % blocksize) != 0) {
                                blk_w = width % blocksize;
                        } else {
                                blk_w = blocksize;
                        }

                        for (int y = 0; y < blk_h; y++) {
                                for (int x = 0; x < blk_w; x++) {
                                        int cell_col = blk_col * blocksize + x;
                                        int cell_row = blk_row * blocksize + y;
                                        apply(cell_col, cell_row, uarray2b,
                                              UArray2b_at(uarray2b,
                                              cell_col, cell_row), cl);
                                }
                        }
                }
        }
}