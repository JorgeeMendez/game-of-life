#include "lifegame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    uint64_t un;
    uint64_t deux;
    uint64_t quatre;
} v;

static inline void additionner_voisins(uint64_t p, v *c)
{
    uint64_t r1 = c->un & p;
    c->un = c->un ^ p;

    uint64_t r2 = c->deux & r1;
    c->deux = c->deux ^ r1;

    c->quatre |= r2;
}

static inline uint64_t comparer8voisins(const uint64_t *p, size_t stride)
{
    v c = {0, 0, 0};

    uint64_t top = *(p - stride);
    uint64_t bottom = *(p + stride);

    uint64_t top_left = (*(p - 1 - stride) >> 63) | (top << 1);
    uint64_t top_right = (top >> 1) | (*(p + 1 - stride) << 63);

    uint64_t bottom_left = (*(p - 1 + stride) >> 63) | (bottom << 1);
    uint64_t bottom_right = (bottom >> 1) | (*(p + 1 + stride) << 63);

    uint64_t left = (*(p - 1) >> 63) | (*p << 1);
    uint64_t right = (*p >> 1) | (*(p + 1) << 63);

    additionner_voisins(top, &c);
    additionner_voisins(bottom, &c);
    additionner_voisins(top_left, &c);
    additionner_voisins(top_right, &c);
    additionner_voisins(bottom_left, &c);
    additionner_voisins(bottom_right, &c);
    additionner_voisins(left, &c);
    additionner_voisins(right, &c);

    uint64_t next = c.deux & ~c.quatre & (c.un | *p);
    return next;
}

bool world_init(World *w, uint16_t width, uint16_t height)
{
    if (width == 0 || height == 0)
    {
        fprintf(stderr, "Error: width and height must be > 0.\n");
        return false;
    }

    w->width = width;
    w->height = height;
    w->words = (width + BITS_PER_WORD - 1) / BITS_PER_WORD;
    w->stride = w->words + PADDING;

    uint16_t rem = width % BITS_PER_WORD;
    w->last_mask = (rem == 0) ? ~0ULL : ((1ULL << rem) - 1ULL);

    size_t total_words = (size_t)(height + PADDING) * w->stride;
    size_t total_bytes = 2 * total_words * sizeof(uint64_t);

    if (total_bytes > MAX_MEMORY_BYTES)
    {
        fprintf(stderr, "Error: %ux%u needs %zu bytes (max %d bytes / 64 KiB).\n",
                width, height, total_bytes, MAX_MEMORY_BYTES);
        return false;
    }

    w->curr = calloc(total_words, sizeof(uint64_t));
    w->next = calloc(total_words, sizeof(uint64_t));
    if (!w->curr || !w->next)
    {
        fprintf(stderr, "Error: memory allocation failed.\n");
        world_free(w);
        return false;
    }
    return true;
}

void world_free(World *w)
{
    free(w->curr);
    free(w->next);
    w->curr = NULL;
    w->next = NULL;
}

bool get_cell_state(const World *w, uint16_t x, uint16_t y)
{
    if (x >= w->width || y >= w->height)
        return false;
    size_t idx = (size_t)(y + 1) * w->stride + (x / BITS_PER_WORD) + 1;
    return (w->curr[idx] >> (x % BITS_PER_WORD)) & 1ULL;
}

void set_cell_state(World *w, uint16_t x, uint16_t y, bool alive)
{
    if (x >= w->width || y >= w->height)
        return;
    size_t idx = (size_t)(y + 1) * w->stride + (x / BITS_PER_WORD) + 1;
    uint64_t bit = 1ULL << (x % BITS_PER_WORD);
    if (alive)
        w->curr[idx] |= bit;
    else
        w->curr[idx] &= ~bit;
}

static void apply_boundaries(World *w, Boundary b)
{
    size_t W = w->words;
    size_t S = w->stride;
    uint16_t rem = w->width % BITS_PER_WORD;
    uint16_t last_bit = (rem == 0) ? 63 : (rem - 1);

    /* Left and right halos for each active row */
    for (uint16_t y = 1; y <= w->height; y++)
    {
        uint64_t *row = &w->curr[(size_t)y * S];
        uint64_t left_bit = 0, right_bit = 0;

        if (b == BOUNDARY_RIM)
        {
            left_bit = 1;
            right_bit = 1;
        }
        else if (b == BOUNDARY_TORUS)
        {
            left_bit = (row[W] >> last_bit) & 1ULL;
            right_bit = row[1] & 1ULL;
        }
        else if (b == BOUNDARY_MIRROR)
        {
            left_bit = row[1] & 1ULL;
            right_bit = (row[W] >> last_bit) & 1ULL;
        }

        row[0] = left_bit << 63;
        row[W] &= w->last_mask;
        if (rem == 0)
        {
            row[W + 1] = right_bit;
        }
        else
        {
            row[W] |= (right_bit << rem);
            row[W + 1] = 0;
        }
    }

    /* Top and bottom halos (also copies corner bits) */
    uint64_t *top = &w->curr[0];
    uint64_t *bottom = &w->curr[(size_t)(w->height + 1) * S];
    uint64_t *first = &w->curr[S];
    uint64_t *last = &w->curr[(size_t)w->height * S];

    if (b == BOUNDARY_EDGE)
    {
        memset(top, 0, S * sizeof(uint64_t));
        memset(bottom, 0, S * sizeof(uint64_t));
    }
    else if (b == BOUNDARY_RIM)
    {
        memset(top, 0xFF, S * sizeof(uint64_t));
        memset(bottom, 0xFF, S * sizeof(uint64_t));
    }
    else if (b == BOUNDARY_TORUS)
    {
        memcpy(top, last, S * sizeof(uint64_t));
        memcpy(bottom, first, S * sizeof(uint64_t));
    }
    else if (b == BOUNDARY_MIRROR)
    {
        memcpy(top, first, S * sizeof(uint64_t));
        memcpy(bottom, last, S * sizeof(uint64_t));
    }
}

void next_generation(World *w, Boundary boundary)
{
    apply_boundaries(w, boundary);

    for (uint16_t y = 1; y <= w->height; y++)
    {
        size_t row = (size_t)y * w->stride;
        for (size_t x = 1; x <= w->words; x++)
        {
            w->next[row + x] = comparer8voisins(&w->curr[row + x], w->stride);
        }
        w->next[row + w->words] &= w->last_mask;
    }

    /* Swap buffers */
    uint64_t *tmp = w->curr;
    w->curr = w->next;
    w->next = tmp;
}

bool world_load_file(World *w, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        perror("fopen input");
        return false;
    }

    uint16_t x = 0, y = 0;
    int ch;
    while ((ch = fgetc(f)) != EOF && y < w->height)
    {
        if (ch == '\n')
        {
            y++;
            x = 0;
        }
        else if (ch != '\r')
        {
            if (x < w->width)
                set_cell_state(w, x, y, ch == '*');
            x++;
        }
    }
    fclose(f);
    return true;
}

bool world_save_file(const World *w, const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f)
    {
        perror("fopen output");
        return false;
    }

    for (uint16_t y = 0; y < w->height; y++)
    {
        for (uint16_t x = 0; x < w->width; x++)
        {
            fputc(get_cell_state(w, x, y) ? '*' : '.', f);
        }
        fputc('\n', f);
    }
    fclose(f);
    return true;
}

void world_load_glider(World *w)
{
    set_cell_state(w, 2, 1, true);
    set_cell_state(w, 3, 2, true);
    set_cell_state(w, 1, 3, true);
    set_cell_state(w, 2, 3, true);
    set_cell_state(w, 3, 3, true);
}

void world_print(const World *w, char *row_buf)
{
    printf("\033[H");
    for (uint16_t y = 1; y <= w->height; y++)
    {
        uint16_t col = 0;
        const uint64_t *row = &w->curr[(size_t)y * w->stride];
        for (size_t x = 1; x <= w->words; x++)
        {
            uint64_t word = row[x];

            int b = 0;
            while (b < BITS_PER_WORD && col < w->width)
            {
                row_buf[col++] = ((word >> b) & 1ULL) ? '*' : '.';
                b++;
            }
        }
        row_buf[w->width] = '\n';
        fwrite(row_buf, 1, w->width + 1, stdout);
    }
    fflush(stdout);
}