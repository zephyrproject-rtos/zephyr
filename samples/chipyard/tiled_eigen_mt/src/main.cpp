/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>
#include <iostream>
#include <Eigen/Dense>
#include "data/matrices.hpp"  // This header defines matrix_A, matrix_B, and matrix_C

// Matrix dimensions and tiling parameters.
constexpr int N = 32;       // Full matrix size.
constexpr int nHalf = N/2;  // Block size (16).

// Global Eigen matrices.
static Eigen::Matrix<float, N, N> A, B, C_computed, C_precomputed;

// Zephyr threading definitions.
#define NUM_THREADS 4    // One thread per tile.
#define STACK_SIZE 16384  // Stack size per thread.

K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);
struct k_thread thread_data[NUM_THREADS];
k_tid_t thread_ids[NUM_THREADS];

struct k_mutex print_mutex;

// Thread function that computes one tile of the matrix multiplication.
void tile_thread_function(void *arg1, void *arg2, void *arg3)
{
    int thread_id = (int)(uintptr_t)arg1;
    int tile_row = thread_id / 2;  // 0 for top, 1 for bottom.
    int tile_col = thread_id % 2;  // 0 for left, 1 for right.

    // Select the appropriate block of A and B.
    Eigen::Matrix<float, nHalf, N> A_block = (tile_row == 0) ? A.topRows(nHalf) : A.bottomRows(nHalf);
    Eigen::Matrix<float, N, nHalf> B_block = (tile_col == 0) ? B.leftCols(nHalf) : B.rightCols(nHalf);

    // Compute the tile: (nHalf x N) * (N x nHalf) = (nHalf x nHalf)
    Eigen::Matrix<float, nHalf, nHalf> C_tile = A_block * B_block;

    // Store the result in the correct block of the global C_computed matrix.
    C_computed.block(tile_row * nHalf, tile_col * nHalf, nHalf, nHalf) = C_tile;

    // Print thread information.
    k_mutex_lock(&print_mutex, K_FOREVER);
    int hart_id = arch_curr_cpu()->id;
    printf("Thread %d computed tile (%d, %d) on hart %d\n", thread_id, tile_row, tile_col, hart_id);
    printf("Thread %d finished execution\n", thread_id);
    k_mutex_unlock(&print_mutex);
}

int main(void)
{
    std::cout << "Hello, C++ world! " << CONFIG_BOARD << std::endl;
    k_mutex_init(&print_mutex);

    // Load matrices from header arrays into global Eigen matrices.
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            A(i, j) = matrix_A[i][j];
            B(i, j) = matrix_B[i][j];
            C_precomputed(i, j) = matrix_C[i][j];
        }
    }

    // Create and start NUM_THREADS threads, each pinned to a hart.
    for (int i = 0; i < NUM_THREADS; i++) {
        int hart_id = i % CONFIG_MP_MAX_NUM_CPUS;
        thread_ids[i] = k_thread_create(&thread_data[i],
                                        thread_stacks[i],
                                        STACK_SIZE,
                                        tile_thread_function,
                                        (void *)(uintptr_t)i,  // Pass thread id.
                                        NULL, NULL,
                                        K_PRIO_PREEMPT(1),
                                        0,
                                        K_FOREVER);
        k_thread_cpu_pin(thread_ids[i], hart_id);
        k_mutex_lock(&print_mutex, K_FOREVER);
        printf("Thread %d assigned to hart %d\n", i, hart_id);
        k_mutex_unlock(&print_mutex);
        k_thread_start(thread_ids[i]);
    }

    // Wait for all threads to complete.
    for (int i = 0; i < NUM_THREADS; i++) {
        k_thread_join(thread_ids[i], K_FOREVER);
    }

    // Validate the computed result against the precomputed matrix.
    const float tolerance = 1e-5f;
    bool valid = C_computed.isApprox(C_precomputed, tolerance);

    if (valid) {
        std::cout << "Validation passed: Parallel tiled matmul matches the precomputed output." << std::endl;
    } else {
        std::cout << "Validation failed: Parallel tiled matmul does not match the precomputed output." << std::endl;
    }

    // Optionally, print the top-left 4x4 block of the computed matrix.
    std::cout << "Top-left 4x4 block of the computed matrix:\n"
              << C_computed.block(0, 0, 4, 4) << std::endl;

    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
