/*
 *      ppmtrans.c
 *      by Jia Wen Goh (jgoh01) & Sean Ong (song02), 10/6/2017
 *
 *      - Reads in ppm data either from a file or standard input
 *      - Transforms the ppm image based on user-specified transformation
 *        type and magnitude
 *      - Optionally records the time taken for the transformation
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "cputiming.h"
#include "mem.h"
#include "pnm.h"

typedef A2Methods_UArray2  A2;
typedef A2Methods_applyfun applyfun;
typedef A2Methods_Object   object;
typedef A2Methods_mapfun   mapfun;

/* Struct with transformed image; to pass in as closure for map functions */
typedef struct result {
        A2Methods_T methods;
        A2          destination_map;
        int         magnitude;
} result;

/* Transform type constants */
static const int ROTATE    = 0;
static const int FLIP      = 1;
static const int TRANSPOSE = 2;

/* Flip direction constants */
static const int HORIZ = 1;
static const int VERT  = 2;

/* Macro for setting row/col/block methods */
#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (0)

/* Error Handling Functions */
static void usage        (const char *progname);
       void malloc_check (void *ptr);

/* File Processing Function */
Pnm_ppm process_file (char *filename, A2Methods_T methods);

/* Image Transformation Functions */
Pnm_ppm transform       (Pnm_ppm ppm, A2Methods_T methods, mapfun *map,
                         int transform_type, int magnitude, float* time);
void    transform_image (Pnm_ppm ppm, applyfun *apply,
                         mapfun *map, result *dest_info, float *time);
void    rotate_map      (int col, int row, A2 source, object *ptr, void *cl);
void    flip_map        (int col, int row, A2 source, object *ptr, void *cl);
void    transpose_map   (int col, int row, A2 source, object *ptr, void *cl);

/* Image Transformation Helper Functions */
A2        create_image   (Pnm_ppm ppm, A2Methods_T methods, int type,
                          int magnitude);
result    result_init    (A2Methods_T methods, int magnitude, A2 image);
applyfun* transform_init (applyfun *apply, int transform_type);
void      reassign       (Pnm_ppm ppm, A2 destination_map, A2Methods_T methods);

/* Timing Function */
void print_time (float *time, char *file, float pixel_count);

/*---------------------------------------------------------------
 |                              Main                            |
 *--------------------------------------------------------------*/
/* [Name]:       main
 * [Purpose]:    Parse command-line arguments & call transformation functions.
 * [Parameters]: 1 int (argc), 1 array of c-strings (argv[])
 * [Return]:     0, for exit success
 */
int main(int argc, char *argv[])
{
        Pnm_ppm  ppm            = NULL;
        char    *time_file_name = NULL;
        char    *filename       = NULL;
        float   *time           = NULL;
        int      transform_type = ROTATE;
        int      magnitude      = 0;
        int      i;

        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods);

        /* default to best map */
        mapfun *map = methods->map_default;
        assert(map);

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-row-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_row_major,
                                    "row-major");
                } else if (strcmp(argv[i], "-col-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_col_major,
                                    "column-major");
                } else if (strcmp(argv[i], "-block-major") == 0) {
                        SET_METHODS(uarray2_methods_blocked, map_block_major,
                                    "block-major");
                } else if (strcmp(argv[i], "-rotate") == 0) {
                        if (!(i + 1 < argc)) {      /* no rotate value */
                                usage(argv[0]);
                        }
                        char *endptr;
                        magnitude = strtol(argv[++i], &endptr, 10);
                        if (!(magnitude == 0 || magnitude == 90 ||
                              magnitude == 180 || magnitude == 270)) {
                                fprintf(stderr,
                                        "Rotation must be 0, 90, 180 or 270\n");
                                usage(argv[0]);
                        }
                        if (!(*endptr == '\0')) {    /* Not a number */
                                usage(argv[0]);
                        }
                        transform_type = ROTATE;
                } else if (strcmp(argv[i], "-flip") == 0) {
                        if (!(i + 1 < argc)) {      /* no flip direction */
                                usage(argv[0]);
                        }
                        char *endptr = argv[++i];
                        if (strcmp(endptr, "horizontal") == 0) {
                                magnitude = HORIZ;
                        } else if (strcmp(endptr, "vertical") == 0) {
                                magnitude = VERT;
                        } else {
                                fprintf(stderr, "Flip must be horizontal "
                                                "or vertical\n");
                                usage(argv[0]);
                        }
                        transform_type = FLIP;
                } else if (strcmp(argv[i], "-transpose") == 0) {
                        transform_type = TRANSPOSE;
                } else if (strcmp(argv[i], "-time") == 0) {
                        if (i == argc - 1) {
                                usage(argv[0]);
                        } else {
                                time_file_name = argv[++i];
                        }
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n", argv[0],
                                argv[i]);
                        usage(argv[0]);
                } else if (argc - i > 1) {
                        fprintf(stderr, "Too many arguments\n");
                        usage(argv[0]);
                } else {
                        filename = argv[i];
                }
        }

        if (time_file_name != NULL) {
                time = malloc(sizeof(float));
                malloc_check(time);
                *time = 0.0;
        }

        ppm = process_file(filename, methods);
        ppm = transform(ppm, methods, map, transform_type, magnitude, time);

        if (time_file_name != NULL) {
                print_time(time, time_file_name, ppm->width * ppm->height);
                free(time);
        }

        Pnm_ppmwrite(stdout, ppm);
        Pnm_ppmfree(&ppm);

        return 0;
}

/*---------------------------------------------------------------
 |                    Error Handling Functions                  |
 *--------------------------------------------------------------*/
/* [Name]:       usage
 * [Purpose]:    Print the proper usage of ppmtrans to stderr.
 * [Parameters]: 1 c-string (progname), guaranteed to be unchanged
 * [Return]:     void
 */
static void usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] [-flip <direction>] "
                        "[-transpose] [{row,col,block}-major] "
                        "[-time <timing_file>] [filename]\n",
                        progname);
        exit(1);
}

/* [Name]:       malloc_check
 * [Purpose]:    Exits the program and prints to stderr if malloc'd ptr == NULL
 * [Parameters]: 1 void* (ptr)
 * [Return]:     void
 */
void malloc_check(void *ptr)
{
        if (ptr == NULL) {
                fprintf(stderr, "Malloc error\n");
                exit(EXIT_FAILURE);
        }
}

/*---------------------------------------------------------------
 |                   File Processing Functions                  |
 *--------------------------------------------------------------*/
/* [Name]:       process_file
 * [Purpose]:    Read binary ppm data from a file or stdin into a Pnm_ppm.
 * [Parameters]: 1 c-string (filename), 1 A2Methods_T (methods)
 * [Return]:     Pnm_ppm containing binary ppm data
 */
Pnm_ppm process_file(char *filename, A2Methods_T methods)
{
        Pnm_ppm ppm = NULL;

        if (filename == NULL) {
                ppm = Pnm_ppmread(stdin, methods);
        } else {
                FILE* inputfp;

                inputfp = fopen(filename, "r");
                if (inputfp == NULL) {
                        fprintf(stderr, "File read error.\n");
                        exit(EXIT_FAILURE);
                }

                ppm = Pnm_ppmread(inputfp, methods);

                fclose(inputfp);
        }

        return ppm;
}

/*---------------------------------------------------------------
 |                    Transformation Functions                  |
 *--------------------------------------------------------------*/
/* [Name]:       transform
 * [Purpose]:    Transforms the given ppm file (rotate/flip/transpose).
 *               Records time taken for transformation, if needed.
 * [Parameters]: 1 Pnm_ppm (source ppm image), 1 A2Methods_T (methods),
 *               1 mapfun* (map), 2 ints (transform_type [see constants],
 *               transform magnitude), 1 float* (time)
 * [Return]:     Transformed image in a Pnm_ppm
 */
Pnm_ppm transform(Pnm_ppm ppm, A2Methods_T methods, mapfun *map,
                  int transform_type, int magnitude, float *time)
{
        A2 image = create_image(ppm, methods, transform_type, magnitude);
        applyfun *apply = NULL;
        result *dest_info = malloc(sizeof(struct result));
        malloc_check(dest_info);

        *dest_info = result_init(methods, magnitude, image);
        apply = transform_init(apply, transform_type);
        transform_image(ppm, apply, map, dest_info, time);
        reassign(ppm, dest_info->destination_map, methods);

        free(dest_info);
        return ppm;
}

/* [Name]:       rotate_map
 * [Purpose]:    Copies source pixel content into a pixel in the destination
 *               image, based on rotation algorithms -- (CALLED BY MAP)
 * [Parameters]: 2 ints (col/row index), 1 A2 (source), 1 object* (content at
 *               the col/row index), 1 void* (closure; will be information
 *               about the destination image)
 * [Return]:     void
 */
void rotate_map(int col, int row, A2 source, object *ptr, void *cl)
{
        result *info        = (result *) cl;
        A2Methods_T methods = info->methods;
        int rotation        = info->magnitude;
        A2 destination      = info->destination_map;

        int height = methods->height(source);
        int width  = methods->width(source);

        Pnm_rgb source_pixel = ((Pnm_rgb) ptr);
        Pnm_rgb dest_pixel;

        if (rotation == 0) {
                dest_pixel = methods->at(destination, col, row);
        } else if (rotation == 90) {
                dest_pixel = methods->at(destination, height - row - 1, col);
        } else if (rotation == 180) {
                dest_pixel = methods->at(destination, width - col - 1,
                                         height - row - 1);
        } else if (rotation == 270) {
                dest_pixel = methods->at(destination, row, width - col - 1);
        }

        *dest_pixel = *source_pixel;
}

/* [Name]:       flip_map
 * [Purpose]:    Copies source pixel content into a pixel in the destination
 *               image, based on flip algorithms -- (CALLED BY MAP)
 * [Parameters]: 2 ints (col/row index), 1 A2 (source), 1 object* (content at
 *               the col/row index), 1 void* (closure; will be information
 *               about the destination image)
 * [Return]:     void
 */
void flip_map(int col, int row, A2 source, object *ptr, void *cl)
{
        result *info        = (result *) cl;
        A2Methods_T methods = info->methods;
        int flip_type       = info->magnitude;
        A2 destination      = info->destination_map;

        int height = methods->height(source);
        int width  = methods->width(source);

        Pnm_rgb source_pixel = ((Pnm_rgb) ptr);
        Pnm_rgb dest_pixel;

        if (flip_type == HORIZ) {
                dest_pixel = methods->at(destination, width - col - 1, row);
        } else if (flip_type == VERT) {
                dest_pixel = methods->at(destination, col, height - row - 1);
        }

        *dest_pixel = *source_pixel;
}

/* [Name]:       transpose_map
 * [Purpose]:    Copies source pixel content into a pixel in the destination
 *               image, based on transpose algorithms -- (CALLED BY MAP)
 * [Parameters]: 2 ints (col/row index), 1 A2 (source), 1 object* (content at
 *               the col/row index), 1 void* (closure; will be information
 *               about the destination image)
 * [Return]:     void
 */
void transpose_map(int col, int row, A2 source, object *ptr, void *cl)
{
        (void) source;
        result *info        = (result *) cl;
        A2Methods_T methods = info->methods;
        A2 destination      = info->destination_map;

        Pnm_rgb source_pixel = ((Pnm_rgb) ptr);
        Pnm_rgb dest_pixel;

        dest_pixel = methods->at(destination, row, col);

        *dest_pixel = *source_pixel;
}

/*---------------------------------------------------------------
 |                Transformation Helper Functions               |
 *--------------------------------------------------------------*/
/* [Name]:       create_image
 * [Purpose]:    Creates an destination A2 based on (edited) dimensions
 *               (if rotated by 90/270 degrees, or transposed -
 *                width & height are swapped).
 * [Parameters]: 1 Pnm_ppm (source ppm), 1 A2Methods_T (methods),
 *               2 ints (transformation type and magnitude)
 * [Return]:     Empty destination A2
 */
A2 create_image(Pnm_ppm ppm, A2Methods_T methods, int type, int magnitude)
{
        int width  = ppm->width;
        int height = ppm->height;
        int size   = sizeof(struct Pnm_rgb);

        if ((type == ROTATE && (magnitude == 90 || magnitude == 270)) ||
             type == TRANSPOSE) {
                ppm->width  = height;
                ppm->height = width;
                return methods->new(height, width, size);
        } else {
                return methods->new(width, height, size);
        }
}

/* [Name]:       result_init
 * [Purpose]:    Initializes the result struct that contains the methods,
 *               magnitude and destination image.
 * [Parameters]: 1 A2Methods_T (methods), 1 int (magnitude), 1 A2 (image)
 * [Return]:     Initialized result
 */
result result_init(A2Methods_T methods, int magnitude, A2 image)
{
        result dest_info;

        dest_info.methods         = methods;
        dest_info.magnitude       = magnitude;
        dest_info.destination_map = image;

        return dest_info;
}

/* [Name]:       transform_init
 * [Purpose]:    Assigns the appropriate apply function to the applyfun*
 * [Parameters]: 1 applyfun* (apply), 1 int (transform_type)
 * [Return]:     Initialized applyfun*
 */
applyfun* transform_init(applyfun *apply, int transform_type)
{
        if (transform_type == ROTATE) {
                apply = rotate_map;
        } else if (transform_type == FLIP) {
                apply = flip_map;
        } else if (transform_type == TRANSPOSE) {
                apply = transpose_map;
        }

        return apply;
}

/* [Name]:       transform_image
 * [Purpose]:    Maps applyfun on the source image to create transformed image
 *               Records the duration taken for the applyfun in time, if needed
 * [Parameters]: 1 Pnm_ppm (ppm), 1 applyfun* (apply), 1 mapfun* (map),
 *               1 result* (dest_info), 1 float* (time)
 * [Return]:     void
 */
void transform_image(Pnm_ppm ppm, applyfun *apply, mapfun *map,
                     result *dest_info, float *time)
{
        CPUTime_T timer;
        if (time != NULL) {
                timer = CPUTime_New();
                CPUTime_Start(timer);
        }

        map(ppm->pixels, apply, dest_info);

        if (time != NULL) {
                *time = CPUTime_Stop(timer);
                CPUTime_Free(&timer);
        }
}

/* [Name]:       reassign
 * [Purpose]:    Reassigns the A2 in ppm to the transformed A2, and frees
 *               heap-allocated data of the previous A2.
 * [Parameters]: 1 Pnm_ppm (ppm), 1 A2 (transformed A2), 1 A2Methods_T methods
 * [Return]:     void
 */
void reassign(Pnm_ppm ppm, A2 destination_map, A2Methods_T methods)
{
        A2 buff = ppm->pixels;
        ppm->pixels = destination_map;
        methods->free(&buff);
}

/*---------------------------------------------------------------
 |                      Timing Functions                        |
 *--------------------------------------------------------------*/
/* [Name]:       print_time
 * [Purpose]:    Prints the timing the map function took onto the given
 *               file.
 * [Parameters]: 1 float* (time it took), 1 char* (file to print timings to),
 *               1 float (pixel count)
 * [Return]:     void
 */
void print_time(float *time, char *file, float pixel_count)
{
        FILE *fp = fopen(file, "w");
        if (fp == NULL) {
                fprintf(stderr, "Time file read error\n");
                exit(EXIT_FAILURE);
        }

        fprintf(fp, "TIMING\n"
                    "Total:\t\t%.0f nanoseconds\n"
                    "Per pixel:\t%.0f nanoseconds\n",
                    *time, *time / pixel_count);
        fclose(fp);
}