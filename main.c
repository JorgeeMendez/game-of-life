#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// STRUCTURES AND ENUMERATIONS
typedef enum Boundary : uint8_t
{
    EDGE,
    TORUS,
    MIRROR,
    RIM
} Boundary;

typedef struct AppState
{
    uint16_t height;
    uint16_t width;
    uint16_t gens;
    uint8_t targetHz;
    Boundary boundary;
    const char *input_file;
    const char *target_file;
} AppState;

// CONSTANTS THAT COULD BE USEFUL

#define TYPESIZE 64

void printUint(const uint64_t *bitChain, size_t *xLocal, size_t bitWidth)
{
    for (size_t i = 0; i < TYPESIZE && *xLocal < bitWidth; i++)
    {
        ((*bitChain >> i) & 1) ? printf("*") : printf(".");
        (*xLocal)++;
    }
}

void printGrid(const uint64_t *grid, size_t gridLength, size_t gridWith, size_t bitWidth)
{
    // Simple printing function to print our current grid
    size_t xLocal = 0;
    for (size_t i = 0; i < gridLength; i++)
    {
        size_t row = i %width;
        printUint(grid + i, &xLocal, bitWidth);

        if (row == width - 1 )
        {
            printf("\n");
            xLocal = 0;
        }
    }
    printf("\n");
}
/*
bool getCellState(size_t x, size_t y)
{
}

uint8_t num_neighbours(size_t x, size_t y)
{
}
*/
// void initializeGame(int argc, char *argv[], AppState *state){}
int main(int argc, char *argv[])
{
    AppState state = {.height = 2, .width = 5, .gens = 100, .targetHz = 60, .boundary = TORUS};
    AppState *PtState = &state;
    /*
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--height"))
            PtState->height = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--width"))
            PtState->width = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--gens"))
            PtState->gens = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--hz"))
            PtState->targetHz = (uint8_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--torus"))
            PtState->boundary = TORUS;
        else if (!strcmp(argv[i], "--mirror"))
            PtState->boundary = MIRROR;
        else if (!strcmp(argv[i], "--rim"))
            PtState->boundary = RIM;
        else if (!strcmp(argv[i], "--edge"))
            PtState->boundary = EDGE;
        else if (!strcmp(argv[i], "--input"))
            PtState->input_file = argv[++i];
        else if (!strcmp(argv[i], "--output"))
            PtState->target_file = argv[++i];
        else
        {
            printf("Unknown: %s\n", argv[i]);
            return 1;
        }
    }
    */
    size_t height = PtState->height;
    size_t width = (PtState->width + TYPESIZE - 1) / TYPESIZE;
    size_t gridLength = height * width;
    printf("height: %zu\n width: %zu\n", height, width);

    // So we made a one dimensional array because it's going to be easier for the future.
    uint64_t *grid = calloc(gridLength, sizeof(*grid));
    if (grid == NULL)
    {
        perror("Grid memory allocation failed.\n");
        return 1;
    }

    printGrid(grid, gridLength, width, PtState->width);

    free(grid);
    return 0;
}
