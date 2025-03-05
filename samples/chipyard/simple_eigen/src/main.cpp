/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <Eigen/Dense>


int main()
{
    std::cout << "Hello, C++ world! " << CONFIG_BOARD << std::endl;

    // Define two 2x2 float matrices
    Eigen::Matrix2f A;
    A << 1, 2,
         3, 4;

    Eigen::Matrix2f B;
    B << 5, 6,
         7, 8;

    // Compute the sum of A and B
    Eigen::Matrix2f C = A + B;
    std::cout << "The result of A + B is:\n" << C << std::endl;

    return 0;
}
