#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define BITS_PER_WORD 64

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
    const char *inputFile;
    const char *targetFile;
} AppState;

void printWord(uint64_t word, size_t *cellCol, size_t cellWidth)
{
    for (size_t bit = 0; bit < BITS_PER_WORD && *cellCol < cellWidth; bit++)
    {
        ((word >> bit) & 1) ? printf("*") : printf(".");
        (*cellCol)++;
    }
}

void printGrid(const uint64_t *grid, size_t totalWords, size_t wordsPerRow, size_t cellWidth)
{
    size_t cellCol = 0;
    for (size_t wordIdx = 0; wordIdx < totalWords; wordIdx++)
    {
        size_t wordCol = wordIdx % wordsPerRow;
        printWord(grid[wordIdx], &cellCol, cellWidth);

        if (wordCol == wordsPerRow - 1)
        {
            printf("\n");
            cellCol = 0;
        }
    }
    printf("\n");
}

/*
bool getCellState(size_t cellX, size_t cellY)
{
}

uint8_t countNeighbors(size_t cellX, size_t cellY)
{
}
*/

// void initializeGame(int argc, char *argv[], AppState *state) {}

int main(int argc, char *argv[])
{
    AppState state = {.height = 2, .width = 5, .gens = 100, .targetHz = 60, .boundary = TORUS};

    /*
    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--height"))
            state.height = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--width"))
            state.width = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--gens"))
            state.gens = (uint16_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--hz"))
            state.targetHz = (uint8_t)atoi(argv[++i]);
        else if (!strcmp(argv[i], "--torus"))
            state.boundary = TORUS;
        else if (!strcmp(argv[i], "--mirror"))
            state.boundary = MIRROR;
        else if (!strcmp(argv[i], "--rim"))
            state.boundary = RIM;
        else if (!strcmp(argv[i], "--edge"))
            state.boundary = EDGE;
        else if (!strcmp(argv[i], "--input"))
            state.inputFile = argv[++i];
        else if (!strcmp(argv[i], "--output"))
            state.targetFile = argv[++i];
        else
        {
            printf("Unknown: %s\n", argv[i]);
            return 1;
        }
    }
    */

    size_t rowCount = state.height;
    size_t wordsPerRow = (state.width + BITS_PER_WORD - 1) / BITS_PER_WORD;
    size_t totalWords = rowCount * wordsPerRow;
    printf("rows: %zu\nwords per row: %zu\n", rowCount, wordsPerRow);

    uint64_t *grid = calloc(totalWords, sizeof(*grid));
    if (grid == NULL)
    {
        perror("Grid memory allocation failed.\n");
        return 1;
    }

    printGrid(grid, totalWords, wordsPerRow, state.width);

    free(grid);
    return 0;
}