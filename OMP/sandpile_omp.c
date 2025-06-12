#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>

char *generate_output_filename(const char *input_filename)
{
    const char *input_prefix = "input";
    const char *suffix = strstr(input_filename, input_prefix);
    if (!suffix)
    {
        fprintf(stderr, "Input file name must contain 'input' prefix\n");
        exit(EXIT_FAILURE);
    }
    char *output_filename = malloc(strlen(input_filename) + 10);
    sprintf(output_filename, "output%s", suffix + strlen(input_prefix));
    return output_filename;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s N M input.txt output.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    const char *input_filename = argv[3];
    const char *output_filename = argv[4];

    FILE *input = fopen(input_filename, "r");
    if (!input)
    {
        perror("fopen input");
        return EXIT_FAILURE;
    }

    FILE *output = fopen(output_filename, "w");
    if (!output)
    {
        perror("fopen output");
        return EXIT_FAILURE;
    }

    int **grid = malloc(N * sizeof *grid);
    for (int i = 0; i < N; i++)
    {
        grid[i] = malloc(M * sizeof *grid[i]);
    }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            fscanf(input, "%d", &grid[i][j]);

    fclose(input);

    bool changed = true;
    double start_time = omp_get_wtime();

    while (changed)
    {
        changed = false;

        // Red phase: Updates cells where (i + j) % 2 == 0
        #pragma omp parallel for collapse(2) reduction(||:changed)
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < M; j++)
            {
                if ((i + j) % 2 == 0) // Red cells
                {
                    if (grid[i][j] >= 4)
                    {
                        changed = true;
                        int distribute = grid[i][j] / 4;
                        grid[i][j] %= 4;
                        if (i > 0)
                        {
                            #pragma omp atomic
                            grid[i - 1][j] += distribute;
                        }
                        if (i < N - 1)
                        {
                            #pragma omp atomic
                            grid[i + 1][j] += distribute;
                        }
                        if (j > 0)
                        {
                            #pragma omp atomic
                            grid[i][j - 1] += distribute;
                        }
                        if (j < M - 1)
                        {
                            #pragma omp atomic
                            grid[i][j + 1] += distribute;
                        }
                    }
                }
            }
        }

        // Black phase: Updates cells where (i + j) % 2 == 1
        #pragma omp parallel for collapse(2) reduction(||:changed)
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < M; j++)
            {
                if ((i + j) % 2 == 1) // Black cells
                {
                    if (grid[i][j] >= 4)
                    {
                        changed = true;
                        int distribute = grid[i][j] / 4;
                        grid[i][j] %= 4;
                        if (i > 0)
                        {
                            #pragma omp atomic
                            grid[i - 1][j] += distribute;
                        }
                        if (i < N - 1)
                        {
                            #pragma omp atomic
                            grid[i + 1][j] += distribute;
                        }
                        if (j > 0)
                        {
                            #pragma omp atomic
                            grid[i][j - 1] += distribute;
                        }
                        if (j < M - 1)
                        {
                            #pragma omp atomic
                            grid[i][j + 1] += distribute;
                        }
                    }
                }
            }
        }
    }

    double end_time = omp_get_wtime();
    printf("OpenMP time: %f seconds\n", end_time - start_time);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            fprintf(output, "%d%s", grid[i][j], j == M - 1 ? "" : " ");
        }
        fprintf(output, "\n");
    }

    fclose(output);

    for (int i = 0; i < N; i++)
    {
        free(grid[i]);
    }
    free(grid);

    return EXIT_SUCCESS;
}