/*
 * Abelian Sandpile Model with Image and Output File (Serial Implementation)
 *
 * Modifications:
 * - Outputs stabilized grid to a .txt file instead of stdout.
 * - Output file is named based on the input file: e.g., input1.txt → output1.txt.
 * - Writes a PNG image of the final state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <png.h> // Requires libpng-dev
#include <mpi.h>

void write_png(const char *filename, int **grid, int N, int M)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if (!png || !info)
    {
        fprintf(stderr, "PNG creation failed\n");
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png)))
    {
        fprintf(stderr, "PNG error during creation\n");
        exit(EXIT_FAILURE);
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, M, N, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png, info);

    png_bytep row = malloc(3 * M);
    for (int y = 0; y < N; y++)
    {
        for (int x = 0; x < M; x++)
        {
            int val = grid[y][x];
            switch (val)
            {
            case 0:
                row[3 * x + 0] = 0;
                row[3 * x + 1] = 0;
                row[3 * x + 2] = 0;
                break;
            case 1:
                row[3 * x + 0] = 0;
                row[3 * x + 1] = 255;
                row[3 * x + 2] = 0;
                break;
            case 2:
                row[3 * x + 0] = 0;
                row[3 * x + 1] = 0;
                row[3 * x + 2] = 255;
                break;
            case 3:
                row[3 * x + 0] = 255;
                row[3 * x + 1] = 0;
                row[3 * x + 2] = 0;
                break;
            default:
                row[3 * x + 0] = 255;
                row[3 * x + 1] = 255;
                row[3 * x + 2] = 255;
                break;
            }
        }
        png_write_row(png, row);
    }
    free(row);
    png_write_end(png, NULL);
    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

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
        fprintf(stderr, "Usage: %s N M input.txt image.png\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    const char *input_filename = argv[3];
    const char *image_filename = argv[4];

    FILE *input = fopen(input_filename, "r");
    if (!input)
    {
        perror("fopen input");
        return EXIT_FAILURE;
    }

    char *output_filename = generate_output_filename(input_filename);
    FILE *output = fopen(output_filename, "w");
    if (!output)
    {
        perror("fopen output");
        return EXIT_FAILURE;
    }

    int **grid = malloc(N * sizeof *grid);
    int **next = malloc(N * sizeof *next);
    for (int i = 0; i < N; i++)
    {
        grid[i] = malloc(M * sizeof *grid[i]);
        next[i] = malloc(M * sizeof *next[i]);
    }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            fscanf(input, "%d", &grid[i][j]);

    fclose(input);

    bool changed = true;
    MPI_Init(&argc, &argv);
    // Start MPI timer
    double start_time = MPI_Wtime();
    while (changed)
    {
        changed = false;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
                next[i][j] = 0;

        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < M; j++)
            {
                int grains = grid[i][j];
                int keep = grains % 4;
                int distribute = grains / 4;
                next[i][j] += keep;
                if (distribute > 0)
                {
                    changed = true;
                    if (i > 0)
                        next[i - 1][j] += distribute;
                    if (i < N - 1)
                        next[i + 1][j] += distribute;
                    if (j > 0)
                        next[i][j - 1] += distribute;
                    if (j < M - 1)
                        next[i][j + 1] += distribute;
                }
            }
        }

        int **tmp = grid;
        grid = next;
        next = tmp;
    }
    // end MPI timer and print time
    double end_time = MPI_Wtime();
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
    {
        printf("Serial time: %f seconds\n", end_time - start_time);
    }

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            fprintf(output, "%d%s", grid[i][j], j == M - 1 ? "" : " ");
        }
        fprintf(output, "\n");
    }

    fclose(output);
    write_png(image_filename, grid, N, M);

    for (int i = 0; i < N; i++)
    {
        free(grid[i]);
        free(next[i]);
    }
    free(grid);
    free(next);
    free(output_filename);

    MPI_Finalize();
    return EXIT_SUCCESS;
}