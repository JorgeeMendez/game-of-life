#define _POSIX_C_SOURCE 199309L
#include "lifegame.h"
#include <errno.h>
#include <limits.h>
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

static bool parse_num(const char *s, long long min_v, long long max_v, long long *out)
{
    if (!s || s[0] == '\0' || s[0] == '-' || s[0] == '+')
        return false;

    errno = 0;
    char *end = NULL;
    long long val = strtoll(s, &end, 10);
    if (errno == ERANGE || end == s || *end != '\0')
        return false;
    if (val < min_v || val > max_v)
        return false;

    *out = val;
    return true;
}

int main(int argc, char *argv[])
{
    const uint16_t def_width = 40, def_height = 20, def_hz = 60;
    const uint32_t def_gens = 500;
    const Boundary def_boundary = BOUNDARY_TORUS;

    uint16_t width = def_width, height = def_height, target_hz = def_hz;
    uint32_t gens = def_gens;
    Boundary boundary = def_boundary;
    const char *in_file = NULL, *out_file = NULL;
    bool bad_args = false;

    for (int i = 1; i < argc; i++)
    {
        long long tmp = 0;

        if (!strcmp(argv[i], "--width"))
        {
            if (i + 1 >= argc || !parse_num(argv[++i], 1, UINT16_MAX, &tmp))
            {
                fprintf(stderr, "Invalid/missing --width (keeping default %u).\n", def_width);
                width = def_width;
                bad_args = true;
            }
            else
                width = (uint16_t)tmp;
        }
        else if (!strcmp(argv[i], "--height"))
        {
            if (i + 1 >= argc || !parse_num(argv[++i], 1, UINT16_MAX, &tmp))
            {
                fprintf(stderr, "Invalid/missing --height (keeping default %u).\n", def_height);
                height = def_height;
                bad_args = true;
            }
            else
                height = (uint16_t)tmp;
        }
        else if (!strcmp(argv[i], "--gens"))
        {
            if (i + 1 >= argc || !parse_num(argv[++i], 0, UINT32_MAX, &tmp))
            {
                fprintf(stderr, "Invalid/missing --gens (keeping default %u).\n", def_gens);
                gens = def_gens;
                bad_args = true;
            }
            else
                gens = (uint32_t)tmp;
        }
        else if (!strcmp(argv[i], "--target-hz"))
        {
            if (i + 1 >= argc || !parse_num(argv[++i], 0, UINT16_MAX, &tmp))
            {
                fprintf(stderr, "Invalid/missing --target-hz (keeping default %u).\n", def_hz);
                target_hz = def_hz;
                bad_args = true;
            }
            else
                target_hz = (uint16_t)tmp;
        }
        else if (!strcmp(argv[i], "--in"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing value for --in.\n");
                in_file = NULL;
                bad_args = true;
            }
            else
                in_file = argv[++i];
        }
        else if (!strcmp(argv[i], "--out"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Missing value for --out.\n");
                out_file = NULL;
                bad_args = true;
            }
            else
                out_file = argv[++i];
        }
        else if (!strcmp(argv[i], "--boundary"))
        {
            if (i + 1 >= argc || !parse_boundary(argv[++i], &boundary))
            {
                fprintf(stderr, "Invalid/missing --boundary (use edge|torus|mirror|rim; keeping default torus).\n");
                boundary = def_boundary;
                bad_args = true;
            }
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            bad_args = true;
        }
    }

    if (bad_args)
        return 1;

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

    char *row_buf = malloc((size_t)width + 1);
    if (!row_buf)
    {
        world_free(&w);
        return 1;
    }

    double total_us = 0.0, worst_us = 0.0;
    double frame_ns = (target_hz > 0) ? (1e9 / target_hz) : 0.0;
    int status = 0;

    printf("\033[2J\033[?25l"); /* Clear screen, hide cursor */

    for (uint32_t g = 0; g < gens; g++)
    {
        struct timespec t0, t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        next_generation(&w, boundary);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        double sim_ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                        (double)(t1.tv_nsec - t0.tv_nsec);
        double us = sim_ns / 1e3;

        total_us += us;
        if (us > worst_us)
            worst_us = us;

        world_print(&w, row_buf);

        if (frame_ns > 0.0)
        {
            clock_gettime(CLOCK_MONOTONIC, &t2);
            double spent_ns = (double)(t2.tv_sec - t0.tv_sec) * 1e9 +
                              (double)(t2.tv_nsec - t0.tv_nsec);
            if (spent_ns < frame_ns)
            {
                double rem = frame_ns - spent_ns;
                struct timespec req = {
                    (time_t)(rem / 1e9),
                    (long)((long long)rem % 1000000000LL)};
                nanosleep(&req, NULL);
            }
        }
    }

    printf("\033[?25h"); /* Show cursor */
    fflush(stdout);

    if (out_file && !world_save_file(&w, out_file))
        status = 1;

    double avg_us = (gens > 0) ? (total_us / gens) : 0.0;
    double jitter_us = worst_us - avg_us;

    printf("\n--- Timing (%u gens, %ux%u) ---\n", gens, width, height);
    printf("Average : %.2f us (%.4f ms)\n", avg_us, avg_us / 1000.0);
    printf("Worst   : %.2f us (%.4f ms)\n", worst_us, worst_us / 1000.0);
    printf("Jitter  : %.2f us (%.4f ms)\n", jitter_us, jitter_us / 1000.0);

    free(row_buf);
    world_free(&w);
    return status;
}