#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>
#include <stdio.h>
#include "data/matrices.hpp"
#include "data/matlib_rvv.h"

#define N 32
#define nHalf (N / 2)
#define NUM_THREADS 4
#define STACK_SIZE 16384

K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);
struct k_thread thread_data[NUM_THREADS];
k_tid_t thread_ids[NUM_THREADS];
struct k_mutex print_mutex;

// Matrix buffers
float A[N * N];
float B[N * N];
float C_computed[N * N];
float C_precomputed[N * N];

void tile_thread_function(void *arg1, void *arg2, void *arg3)
{
    int thread_id = (int)(uintptr_t)arg1;
    int tile_row = thread_id / 2;
    int tile_col = thread_id % 2;

    int i = tile_row * nHalf;
    int j = tile_col * nHalf;
    int k = 0;

    // Only compute the tile (i,j) portion
    matmul_rvvt(A, B, C_computed, i, j, k, N, N, N, nHalf);

    k_mutex_lock(&print_mutex, K_FOREVER);
    int hart_id = arch_curr_cpu()->id;
    printf("Thread %d computed tile (%d, %d) on hart %d\n", thread_id, tile_row, tile_col, hart_id);
    printf("Thread %d finished execution\n", thread_id);
    k_mutex_unlock(&print_mutex);
}

int main(void)
{
    printf("Hello, C world! %s\n", CONFIG_BOARD);
    k_mutex_init(&print_mutex);

    // Load input matrices from header
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            A[i * N + j] = matrix_A[i][j];
            B[i * N + j] = matrix_B[i][j];
            C_precomputed[i * N + j] = matrix_C[i][j];
        }

    matset_rvv(C_computed, 0.0f, N, N);

    for (int i = 0; i < NUM_THREADS; ++i) {
        int hart_id = i % CONFIG_MP_MAX_NUM_CPUS;
        thread_ids[i] = k_thread_create(&thread_data[i],
                                        thread_stacks[i],
                                        STACK_SIZE,
                                        tile_thread_function,
                                        (void *)(uintptr_t)i,
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

    for (int i = 0; i < NUM_THREADS; ++i) {
        k_thread_join(thread_ids[i], K_FOREVER);
    }

    float tolerance = 1e-5f;
    float diff[N * N];
    matsub_rvv(C_computed, C_precomputed, diff, N, N);
    float error = matnorm_rvv(diff, N, N);

    if (error < tolerance) {
        printf("Validation passed: Result matches precomputed output (error = %f)\n", error);
    } else {
        printf("Validation failed: error = %f\n", error);
    }

    printf("Top-left 4x4 block of the computed matrix:\n");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j)
            printf("%f ", C_computed[i * N + j]);
        printf("\n");
    }

    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
