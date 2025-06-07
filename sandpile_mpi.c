/*
 * Abelian Sandpile Model - Parallel MPI Implementation
 *
 * This implementation distributes rows across MPI processes using domain decomposition.
 * Each process handles a contiguous block of rows and communicates with neighbors
 * to exchange boundary information during the stabilization process.
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
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 5)
    {
        if (rank == 0)
            fprintf(stderr, "Usage: %s N M input.txt image.png\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    const char *input_filename = argv[3];
    const char *image_filename = argv[4];

    // Calculate domain decomposition
    int rows_per_proc = N / size;
    int remainder = N % size;
    int local_rows = rows_per_proc + (rank < remainder ? 1 : 0);

    // Allocate local grid with ghost rows
    int **local_grid = malloc((local_rows + 2) * sizeof(int *));
    int **local_next = malloc((local_rows + 2) * sizeof(int *));
    for (int i = 0; i < local_rows + 2; i++)
    {
        local_grid[i] = calloc(M, sizeof(int));
        local_next[i] = calloc(M, sizeof(int));
    }

    // Read and distribute initial grid
    int **full_grid = NULL;
    if (rank == 0)
    {
        FILE *input = fopen(input_filename, "r");
        if (!input)
        {
            perror("fopen input");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        full_grid = malloc(N * sizeof(int *));
        for (int i = 0; i < N; i++)
        {
            full_grid[i] = malloc(M * sizeof(int));
            for (int j = 0; j < M; j++)
                fscanf(input, "%d", &full_grid[i][j]);
        }
        fclose(input);
    }

    // Scatter initial data to all processes
    int *sendbuf = NULL;
    int *sendcounts = malloc(size * sizeof(int));
    int *displs = malloc(size * sizeof(int));

    if (rank == 0)
    {
        sendbuf = malloc(N * M * sizeof(int));
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
                sendbuf[i * M + j] = full_grid[i][j];

        // Calculate send counts and displacements
        int offset = 0;
        for (int p = 0; p < size; p++)
        {
            int p_rows = rows_per_proc + (p < remainder ? 1 : 0);
            sendcounts[p] = p_rows * M;
            displs[p] = offset;
            offset += sendcounts[p];
        }
    }

    int *recvbuf = malloc(local_rows * M * sizeof(int));
    MPI_Scatterv(sendbuf, sendcounts, displs, MPI_INT, recvbuf, local_rows * M, MPI_INT, 0, MPI_COMM_WORLD);

    // Copy received data to local grid (skip ghost row 0)
    for (int i = 0; i < local_rows; i++)
        for (int j = 0; j < M; j++)
            local_grid[i + 1][j] = recvbuf[i * M + j];

    // Determine neighbors
    int prev_rank = (rank == 0) ? MPI_PROC_NULL : rank - 1;
    int next_rank = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    // Start mpi timing
    double start_time = MPI_Wtime();

    bool global_changed = true;

    while (global_changed)
    {

        // Exchange ghost rows
        MPI_Request requests[4];
        int req_count = 0;

        // Send top boundary, receive into bottom ghost row
        if (prev_rank != MPI_PROC_NULL)
        {
            MPI_Isend(local_grid[1], M, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, &requests[req_count++]);
            MPI_Irecv(local_grid[0], M, MPI_INT, prev_rank, 1, MPI_COMM_WORLD, &requests[req_count++]);
        }

        // Send bottom boundary, receive into top ghost row
        if (next_rank != MPI_PROC_NULL)
        {
            MPI_Isend(local_grid[local_rows], M, MPI_INT, next_rank, 1, MPI_COMM_WORLD, &requests[req_count++]);
            MPI_Irecv(local_grid[local_rows + 1], M, MPI_INT, next_rank, 0, MPI_COMM_WORLD, &requests[req_count++]);
        }

        MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);

        // Clear next grid
        for (int i = 0; i < local_rows + 2; i++)
            for (int j = 0; j < M; j++)
                local_next[i][j] = 0;

        bool local_changed = false;

        // Process local cells (excluding ghost rows)
        for (int i = 1; i <= local_rows; i++)
        {
            for (int j = 0; j < M; j++)
            {
                int grains = local_grid[i][j];
                int keep = grains % 4;
                int distribute = grains / 4;

                local_next[i][j] += keep;

                if (distribute > 0)
                {
                    local_changed = true;

                    // Distribute to neighbors
                    local_next[i - 1][j] += distribute; // up (might be ghost)
                    local_next[i + 1][j] += distribute; // down (might be ghost)

                    if (j > 0)
                        local_next[i][j - 1] += distribute; // left
                    if (j < M - 1)
                        local_next[i][j + 1] += distribute; // right
                }
            }
        }

        // Handle boundary contributions from ghost cells
        // Top ghost row contributions
        if (prev_rank != MPI_PROC_NULL)
        {
            for (int j = 0; j < M; j++)
            {
                int grains = local_grid[0][j];
                int distribute = grains / 4;
                if (distribute > 0)
                    local_next[1][j] += distribute;
            }
        }

        // Bottom ghost row contributions
        if (next_rank != MPI_PROC_NULL)
        {
            for (int j = 0; j < M; j++)
            {
                int grains = local_grid[local_rows + 1][j];
                int distribute = grains / 4;
                if (distribute > 0)
                    local_next[local_rows][j] += distribute;
            }
        }

        // Swap grids
        int **tmp = local_grid;
        local_grid = local_next;
        local_next = tmp;

        // Global reduction to check if any process had changes
        MPI_Allreduce(&local_changed, &global_changed, 1, MPI_C_BOOL, MPI_LOR, MPI_COMM_WORLD);
    }

    // End mpi timing
    double end_time = MPI_Wtime();

    if (rank == 0)
    {
        printf("MPI execution time: %f seconds\n", end_time - start_time);
    }

    // Gather final results
    for (int i = 0; i < local_rows; i++)
        for (int j = 0; j < M; j++)
            recvbuf[i * M + j] = local_grid[i + 1][j];

    MPI_Gatherv(recvbuf, local_rows * M, MPI_INT, sendbuf, sendcounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // Write output (rank 0 only)
    if (rank == 0)
    {
        // Reconstruct full grid
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
                full_grid[i][j] = sendbuf[i * M + j];

        char *output_filename = generate_output_filename(input_filename);
        FILE *output = fopen(output_filename, "w");
        if (!output)
        {
            perror("fopen output");
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < M; j++)
            {
                fprintf(output, "%d%s", full_grid[i][j], j == M - 1 ? "" : " ");
            }
            fprintf(output, "\n");
        }
        fclose(output);

        write_png(image_filename, full_grid, N, M);

        // Cleanup
        for (int i = 0; i < N; i++)
            free(full_grid[i]);
        free(full_grid);
        free(output_filename);
        free(sendbuf);
    }

    // Cleanup local data
    for (int i = 0; i < local_rows + 2; i++)
    {
        free(local_grid[i]);
        free(local_next[i]);
    }
    free(local_grid);
    free(local_next);
    free(recvbuf);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return EXIT_SUCCESS;
}