#ifndef LIFEGAME_H
#define LIFEGAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BITS_PER_WORD 64
#define PADDING 2
#define MAX_MEMORY_BYTES (64 * 1024) /* 64 KiB embedded memory budget */

typedef enum
{
    BOUNDARY_EDGE,
    BOUNDARY_TORUS,
    BOUNDARY_MIRROR,
    BOUNDARY_RIM
} Boundary;

typedef struct
{
    uint16_t width;
    uint16_t height;
    size_t words;       /* 64-bit words per active row */
    size_t stride;      /* words + PADDING (1 word left, 1 word right) */
    uint64_t last_mask; /* Valid bits mask for the last word in each row */
    uint64_t *curr;     /* Current generation grid */
    uint64_t *next;     /* Shadow grid for next generation */
} World;

/* Lifecycle */
bool world_init(World *w, uint16_t width, uint16_t height);
void world_free(World *w);

/* Cell helpers, to either get the state or set a value :) */
bool get_cell_state(const World *w, uint16_t x, uint16_t y);
void set_cell_state(World *w, uint16_t x, uint16_t y, bool alive);

/* Simulates one generation */
void next_generation(World *w, Boundary boundary);

/* File and terminal functions */
bool world_load_file(World *w, const char *filename);
bool world_save_file(const World *w, const char *filename);
void world_load_glider(World *w);
void world_print(const World *w, char *row_buf);

#endif /* LIFEGAME_H */