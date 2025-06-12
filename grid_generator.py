def create_grid_txt(size, filename=None):
    grid = [[4 for _ in range(size)] for _ in range(size)]

    if filename is None:
        filename = f"input_{size}.txt"

    with open(filename, "w") as f:
        for row in grid:
            f.write(" ".join(map(str, row)) + "\n")

    print(f"Grid saved to {filename}")


create_grid_txt(2048)  # Creates nxm grid with all values set to 4
