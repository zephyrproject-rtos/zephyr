/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <Eigen/Dense>

int main(void)
{
    std::cout << "Hello, C++ world! " << CONFIG_BOARD << std::endl;

    constexpr int N = 64;
    // Create two N x N matrices with random float values
    Eigen::Matrix<float, N, N> A = Eigen::Matrix<float, N, N>::Random();
    Eigen::Matrix<float, N, N> B = Eigen::Matrix<float, N, N>::Random();
    Eigen::Matrix<float, N, N> C;

    // Perform matrix multiplication
    C = A * B;

    // Print only a small 3x3 block from the result
    std::cout << "Top-left 3x3 block of the result:\n" 
              << C.block(0, 0, 3, 3) << std::endl;

    return 0;
}
