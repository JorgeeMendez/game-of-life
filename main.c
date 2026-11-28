#define _POSIX_C_SOURCE 199309L
#include "lifegame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool parse_boundary(const char *s, Boundary *b)
{
    if (strcmp(s, "edge") == 0)
        *b = BOUNDARY_EDGE;
    else if (strcmp(s, "torus") == 0)
        *b = BOUNDARY_TORUS;
    else if (strcmp(s, "mirror") == 0)
        *b = BOUNDARY_MIRROR;
    else if (strcmp(s, "rim") == 0)
        *b = BOUNDARY_RIM;
    else
        return false;
    return true;
}

int main(int argc, char *argv[])
{
    uint16_t width = 40, height = 20, target_hz = 60;
    uint32_t gens = 500;
    Boundary boundary = BOUNDARY_TORUS;
    const char *in_file = NULL, *out_file = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--width") && i + 1 < argc)
            width = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--height") && i + 1 < argc)
            height = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--gens") && i + 1 < argc)
            gens = (uint32_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--target-hz") && i + 1 < argc)
            target_hz = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--in") && i + 1 < argc)
            in_file = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc)
            out_file = argv[++i];
        else if (!strcmp(argv[i], "--boundary") && i + 1 < argc)
        {
            if (!parse_boundary(argv[++i], &boundary))
            {
                fprintf(stderr, "Invalid boundary (use edge|torus|mirror|rim).\n");
                return 1;
            }
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    World w;
    if (!world_init(&w, width, height))
        return 1;

    if (in_file)
    {
        if (!world_load_file(&w, in_file))
        {
            world_free(&w);
            return 1;
        }
    }
    else
    {
        world_load_glider(&w);
    }

    char *row_buf = malloc(width + 1);
    if (!row_buf)
    {
        world_free(&w);
        return 1;
    }

    double total_us = 0.0, worst_us = 0.0;
    double frame_ns = (target_hz > 0) ? (1e9 / target_hz) : 0.0;

    printf("\033[2J\033[?25l"); /* Clear screen, hide cursor */

    for (uint32_t g = 0; g < gens; g++)
    {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        next_generation(&w, boundary);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 + (double)(t1.tv_nsec - t0.tv_nsec);
        double us = ns / 1e3;

        total_us += us;
        if (us > worst_us)
            worst_us = us;

        world_print(&w, row_buf);

        if (frame_ns > 0.0 && ns < frame_ns)
        {
            double rem = frame_ns - ns;
            struct timespec req = {(time_t)(rem / 1e9), (long)((long long)rem % 1000000000LL)};
            nanosleep(&req, NULL);
        }
    }

    printf("\033[?25h"); /* Show cursor */

    if (out_file)
        world_save_file(&w, out_file);

    double avg_us = (gens > 0) ? (total_us / gens) : 0.0;
    double jitter_us = worst_us - avg_us;

    printf("\n--- Timing (%u gens, %ux%u) ---\n", gens, width, height);
    printf("Average : %.2f us (%.4f ms)\n", avg_us, avg_us / 1000.0);
    printf("Worst   : %.2f us (%.4f ms)\n", worst_us, worst_us / 1000.0);
    printf("Jitter  : %.2f us (%.4f ms)\n", jitter_us, jitter_us / 1000.0);

    free(row_buf);
    world_free(&w);
    return 0;
}