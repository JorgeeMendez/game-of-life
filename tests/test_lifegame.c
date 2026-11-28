#include "lifegame.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void make_file(const char *path, const char *txt)
{
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fputs(txt, f);
    assert(fclose(f) == 0);
}

static void check_against_scalar(uint16_t width, uint16_t height, Boundary b)
{
    World w;
    assert(world_init(&w, width, height));

    for (uint16_t y = 0; y < height; y++)
        for (uint16_t x = 0; x < width; x++)
            set_cell_state(&w, x, y, ((x * 31 + y * 13) % 3) == 0);

    bool *ref = malloc((size_t)width * height * sizeof(bool));
    assert(ref != NULL);

    for (uint16_t y = 0; y < height; y++)
        for (uint16_t x = 0; x < width; x++)
            ref[(size_t)y * width + x] = get_next_state(&w, x, y, b);

    next_generation(&w, b);

    for (uint16_t y = 0; y < height; y++)
        for (uint16_t x = 0; x < width; x++)
            assert(get_cell_state(&w, x, y) == ref[(size_t)y * width + x]);

    free(ref);
    world_free(&w);
}

int main(void)
{
    /* Blinker */
    {
        World w;
        assert(world_init(&w, 5, 5));
        set_cell_state(&w, 1, 2, true);
        set_cell_state(&w, 2, 2, true);
        set_cell_state(&w, 3, 2, true);

        next_generation(&w, BOUNDARY_EDGE);
        assert(!get_cell_state(&w, 1, 2) && !get_cell_state(&w, 3, 2));
        assert(get_cell_state(&w, 2, 1) && get_cell_state(&w, 2, 2) && get_cell_state(&w, 2, 3));

        next_generation(&w, BOUNDARY_EDGE);
        assert(get_cell_state(&w, 1, 2) && get_cell_state(&w, 2, 2) && get_cell_state(&w, 3, 2));
        world_free(&w);
    }

    /* Block */
    {
        World w;
        assert(world_init(&w, 6, 6));
        set_cell_state(&w, 2, 2, true);
        set_cell_state(&w, 3, 2, true);
        set_cell_state(&w, 2, 3, true);
        set_cell_state(&w, 3, 3, true);

        for (int i = 0; i < 4; i++)
        {
            next_generation(&w, BOUNDARY_EDGE);
            assert(get_cell_state(&w, 2, 2) && get_cell_state(&w, 3, 2));
            assert(get_cell_state(&w, 2, 3) && get_cell_state(&w, 3, 3));
        }
        world_free(&w);
    }

    /* Glider wrapping on 5x5 torus (period 20) */
    {
        World w;
        assert(world_init(&w, 5, 5));
        world_load_glider(&w);
        for (int i = 0; i < 20; i++)
            next_generation(&w, BOUNDARY_TORUS);

        assert(get_cell_state(&w, 2, 1));
        assert(get_cell_state(&w, 3, 2));
        assert(get_cell_state(&w, 1, 3));
        assert(get_cell_state(&w, 2, 3));
        assert(get_cell_state(&w, 3, 3));
        world_free(&w);
    }

    /* Widths, height=1, and all 4 boundaries */
    {
        uint16_t widths[] = {1, 63, 64, 65, 127, 128, 129};
        Boundary bounds[] = {BOUNDARY_EDGE, BOUNDARY_TORUS, BOUNDARY_MIRROR, BOUNDARY_RIM};

        for (int b = 0; b < 4; b++)
        {
            check_against_scalar(1, 1, bounds[b]);
            check_against_scalar(65, 1, bounds[b]);
            for (size_t i = 0; i < sizeof(widths) / sizeof(widths[0]); i++)
                check_against_scalar(widths[i], 4, bounds[b]);
        }
    }

    /* 64 KiB budget */
    {
        World w;
        assert(world_init(&w, 320, 240));
        world_free(&w);
        assert(!world_init(&w, 512, 512));
        assert(!world_init(&w, 0, 10));
    }

    /* File I/O */
    {
        World w;
        assert(world_init(&w, 5, 3));

        make_file("tests/t_empty.txt", "");
        assert(world_load_file(&w, "tests/t_empty.txt"));

        make_file("tests/t_crlf.txt", "..*..\r\n...*.\r\n.***.\r\n");
        assert(world_load_file(&w, "tests/t_crlf.txt"));
        assert(get_cell_state(&w, 2, 0) && get_cell_state(&w, 3, 1));
        assert(world_save_file(&w, "tests/t_out.txt"));

        make_file("tests/t_bad.txt", "..Z..\n");
        assert(!world_load_file(&w, "tests/t_bad.txt"));

        make_file("tests/t_big.txt", "......\n");
        assert(!world_load_file(&w, "tests/t_big.txt"));

        assert(!world_load_file(&w, "tests/nope_missing.txt"));

        remove("tests/t_empty.txt");
        remove("tests/t_crlf.txt");
        remove("tests/t_out.txt");
        remove("tests/t_bad.txt");
        remove("tests/t_big.txt");
        world_free(&w);
    }

    /* CLI error checks */
    assert(system("./lifegame --width -5 > /dev/null 2>&1") != 0);
    assert(system("./lifegame --height 0 > /dev/null 2>&1") != 0);
    assert(system("./lifegame --gens abc > /dev/null 2>&1") != 0);
    assert(system("./lifegame --boundary bogus > /dev/null 2>&1") != 0);
    assert(system("./lifegame --width > /dev/null 2>&1") != 0);
    assert(system("./lifegame --nope > /dev/null 2>&1") != 0);

    printf("All tests passed!\n");
    return 0;
}