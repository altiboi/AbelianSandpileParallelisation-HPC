
# compare_out.py – verify that two stable Abelian-sandpile output files are identical

from pathlib import Path
import argparse
import numpy as np
import sys
from textwrap import indent

def load_grid(path: Path) -> np.ndarray:
    try:
        return np.loadtxt(path)
    except Exception as exc:
        sys.exit(f"❌  Failed to read {path}: {exc}")

def compare(a: np.ndarray, b: np.ndarray, rtol: float, atol: float):
    if a.shape != b.shape:
        sys.exit(f"❌  Shape mismatch: {a.shape} vs {b.shape}")

    mismatch  = ~np.isclose(a, b, rtol=rtol, atol=atol)
    n_bad     = mismatch.sum()

    print(f"✓  Shapes match: {a.shape}")
    print(f"•  cells ≈   = {a.size - n_bad} / {a.size} "
          f"({100*(1-n_bad/a.size):.2f} % match)")
    if n_bad:
        r, c = np.argwhere(mismatch)[:5].T
        print("⚠️  first differing indices (row, col):")
        for i, j in zip(r, c):
            print(f"    ({i}, {j}) : serial={a[i, j]}  mpi={b[i, j]}")

def main():
    parser = argparse.ArgumentParser(
        description="Compare two stable-grid text files.")
    parser.add_argument("serial_file", type=Path,
                        help="Reference serial output (text)")
    parser.add_argument("mpi_file",    type=Path,
                        help="MPI output to compare (text)")
    parser.add_argument("--rtol", type=float, default=0.0,
                        help="Relative tolerance for fp comparison "
                             "(default: 0 – exact match)")
    parser.add_argument("--atol", type=float, default=0.0,
                        help="Absolute tolerance for fp comparison "
                             "(default: 0 – exact match)")
    args = parser.parse_args()

    serial_grid = load_grid(args.serial_file)
    mpi_grid    = load_grid(args.mpi_file)

    print(f"Comparing '{args.serial_file}'  vs  '{args.mpi_file}'")
    compare(serial_grid, mpi_grid, args.rtol, args.atol)

if __name__ == "__main__":
    main()