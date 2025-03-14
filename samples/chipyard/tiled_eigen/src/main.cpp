/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/reboot.h>

#include <iostream>
#include <Eigen/Dense>
#include "data/matrices.hpp"  // This header defines matrix_A, matrix_B, and matrix_C

int main(void)
{
    std::cout << "Hello, C++ world! " << CONFIG_BOARD << std::endl;

    constexpr int N = 32;     // Full matrix size
    constexpr int nHalf = N / 2; // Block size (16)

    // Create Eigen matrices to hold the data from the header arrays.
    Eigen::Matrix<float, N, N> A, B, C_computed, C_precomputed;

    // Load matrices from header file arrays.
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            A(i, j) = matrix_A[i][j];
            B(i, j) = matrix_B[i][j];
            C_precomputed(i, j) = matrix_C[i][j];
        }
    }

    // Compute the tiled matrix multiplication by splitting the output into 4 blocks.
    C_computed.block(0, 0, nHalf, nHalf) = A.topRows(nHalf) * B.leftCols(nHalf);

    C_computed.block(0, nHalf, nHalf, nHalf) = A.topRows(nHalf) * B.rightCols(nHalf);

    C_computed.block(nHalf, 0, nHalf, nHalf) = A.bottomRows(nHalf) * B.leftCols(nHalf);

    C_computed.block(nHalf, nHalf, nHalf, nHalf) = A.bottomRows(nHalf) * B.rightCols(nHalf);

    // Validate the computed matrix against the precomputed output.
    const float tolerance = 1e-5f;
    bool valid = C_computed.isApprox(C_precomputed, tolerance);

    if (valid) {
        std::cout << "Validation passed: Tiled matrix multiplication matches the precomputed output." << std::endl;
    } else {
        std::cout << "Validation failed: Tiled matrix multiplication does not match the precomputed output." << std::endl;
    }

    // Optionally, print the top-left 4x4 block of the computed matrix.
    std::cout << "Top-left 4x4 block of the computed matrix:\n" 
              << C_computed.block(0, 0, 4, 4) << std::endl;

    sys_reboot(SYS_REBOOT_COLD);  
    return 0;
}