#!/usr/bin/env python3
import numpy as np

def matrix_to_c_array(mat):
    """Converts a NumPy 2D array to a C-style initializer string."""
    rows = []
    # Use .6f to always include a decimal point and exactly 10 digits after the decimal.
    for row in mat:
        formatted_row = ", ".join(f"{val:.10f}f" for val in row)
        rows.append(f"  {{ {formatted_row} }}")
    return "{\n" + ",\n".join(rows) + "\n}"

def main():
    for N in [32, 64, 96, 128, 192, 256]:
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
        with open(f"matrices-{N}.hpp", "w") as f:
            f.write(header_content)
        
        print(f"Header file 'matrices-{N}.hpp' generated successfully.")

if __name__ == "__main__":
    main()

