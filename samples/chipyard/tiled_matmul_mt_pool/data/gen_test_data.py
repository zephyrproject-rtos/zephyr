#!/usr/bin/env python3
import numpy as np

N=32

def matrix_to_c_array(mat):
    """Converts a NumPy 2D array to a C-style initializer string."""
    rows = []
    # Format each float with a trailing 'f' for single precision.
    for row in mat:
        formatted_row = ", ".join(f"{val:.6g}f" for val in row)
        rows.append(f"  {{ {formatted_row} }}")
    return "{\n" + ",\n".join(rows) + "\n}"

def main():
    # Generate two random NxN float32 matrices.
    A = np.random.rand(N, N).astype(np.float32)
    B = np.random.rand(N, N).astype(np.float32)
    
    # Multiply the matrices.
    C = A @ B

    # Create the C++ header file content.
    header_lines = []
    header_lines.append("#ifndef MATRICES_HPP")
    header_lines.append("#define MATRICES_HPP")
    header_lines.append("")
    header_lines.append("#include <Eigen/Core>")
    header_lines.append("")
    header_lines.append("// Generated matrices")
    header_lines.append(f"static const float matrix_A[{N}][{N}] = " + matrix_to_c_array(A) + ";")
    header_lines.append("")
    header_lines.append(f"static const float matrix_B[{N}][{N}] = " + matrix_to_c_array(B) + ";")
    header_lines.append("")
    header_lines.append(f"static const float matrix_C[{N}][{N}] = " + matrix_to_c_array(C) + ";")
    header_lines.append("")
    header_lines.append("#endif // MATRICES_HPP")
    
    header_content = "\n".join(header_lines)
    
    # Write the header file.
    with open(f"matrices_{N}.hpp", "w") as f:
        f.write(header_content)
    
    print("Header file 'matrices.hpp' generated successfully.")

if __name__ == "__main__":
    main()
