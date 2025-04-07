#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>
#include <stdio.h>
#include <string.h>
//#include "../data/matrices_128.hpp"
#include "../data/matrices.hpp"
#include <riscv_vector.h>
#include <zephyr/timing/timing.h>

constexpr int N = 32;
constexpr int nHalf = N / 2;

#define NUM_THREADS 3
#define STACK_SIZE 16384*4

#define MSTATUS_VS          0x00000600
#define MSTATUS_FS          0x00006000
#define MSTATUS_XS          0x00018000

static inline void enable_vector_operations() {
    unsigned long mstatus;

    // Read current mstatus
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));

    // Set VS field to Dirty (11)
    mstatus |= MSTATUS_VS | MSTATUS_FS | MSTATUS_XS;

    // Write back updated mstatus
    asm volatile("csrw mstatus, %0"::"r"(mstatus));
}

struct ThreadInfo {
    int thread_id;
    int tile_row;
    int tile_col;
    int hart_id;
    uint64_t assigned_cycle;
    uint64_t compute_cycles;
    uint64_t compute_ns;
    timing_t start_ct;
    timing_t end_ct;
};
ThreadInfo thread_info[NUM_THREADS];

K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);
struct k_thread thread_data[NUM_THREADS];
k_tid_t thread_ids[NUM_THREADS];
struct k_mutex print_mutex;

float C_computed[N][N] = {0};
uint64_t assignment_cycles[NUM_THREADS];
timing_t join_start, join_end;

static inline uint64_t read_cycle() {
    uint64_t cycle;
    asm volatile("rdcycle %0" : "=r"(cycle));
    return cycle;
}

inline void matmul_rvvt(float *a, float *b, float *c,
                        int i, int j, int k, int n, int m, int o, int tile_size) {
    float *A = a + i * o + k;
    float *B_tile = b + k * m + j;
    float *C = c + i * m + j;

    int N = i + tile_size <= n ? tile_size : n % tile_size;
    int M = j + tile_size <= m ? tile_size : m % tile_size;
    int O = k + tile_size <= o ? tile_size : o % tile_size;

    size_t vlmax = __riscv_vsetvlmax_e32m1();
    vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);

    for (int I = 0; I < N; ++I) {
        for (int J = 0; J < M; ++J) {
            float *ptr_a = A + I * o;
            float *ptr_b = B_tile + J;
            int K = O;

            vfloat32m1_t vec_s = __riscv_vfmv_v_f_f32m1(0, vlmax);
            for (size_t vl, l = 0; K > 0; K -= vl, ptr_a += vl, ptr_b += vl * m, l += vl) {
                vl = __riscv_vsetvl_e32m1(K);
                vfloat32m1_t vec_a = __riscv_vle32_v_f32m1(ptr_a, vl);
                vfloat32m1_t vec_b = __riscv_vlse32_v_f32m1(ptr_b, m * sizeof(float), vl);
                vec_s = __riscv_vfmacc_vv_f32m1(vec_s, vec_a, vec_b, vl);
            }

            vfloat32m1_t vec_sum = __riscv_vfredusum_vs_f32m1_f32m1(vec_s, vec_zero, vlmax);
            float sum = __riscv_vfmv_f_s_f32m1_f32(vec_sum);
            C[I * m + J] = k == 0 ? sum : C[I * m + J] + sum;
        }
    }
}
void tile_thread_function(void *arg1, void *arg2, void *arg3)
{
    int thread_id = (int)(uintptr_t)arg1;
    int hart_id = arch_curr_cpu()->id;
    int tile_row = thread_id / 2;
    int tile_col = thread_id % 2;

    int i = tile_row * nHalf;
    int j = tile_col * nHalf;

    //enable_vector_operations();

    // Serialize printf to avoid garbled output
    //k_mutex_lock(&print_mutex, K_FOREVER);
    //printf("Started thread %d on tile (%d, %d) on hart %d\n",
    //       thread_id, tile_row, tile_col, hart_id);
    //k_mutex_unlock(&print_mutex);

    timing_t start_ct = timing_counter_get();
    uint64_t start_rd = read_cycle();

    k_sched_lock();
    for (int k = 0; k < N; k += nHalf) {
        matmul_rvvt((float *)matrix_A, (float *)matrix_B, (float *)C_computed,
                    i, j, k, N, N, N, nHalf);
        
    }
    k_sched_unlock();

    timing_t end_ct = timing_counter_get();
    uint64_t end_rd = read_cycle();

    uint64_t cycles = timing_cycles_get(&start_ct, &end_ct);
    uint64_t ns = timing_cycles_to_ns(cycles);

    thread_info[thread_id] = {
        .thread_id = thread_id,
        .tile_row = tile_row,
        .tile_col = tile_col,
        .hart_id = hart_id,
        .assigned_cycle = assignment_cycles[thread_id],
        .compute_cycles = cycles,
        .compute_ns = ns,
        .start_ct = start_ct,
        .end_ct = end_ct
    };
}

// void tile_thread_function(void *arg1, void *arg2, void *arg3)
// {
//     int thread_id = (int)(uintptr_t)arg1;
//     int hart_id = arch_curr_cpu()->id;
//     int tile_row = thread_id / 2;
//     int tile_col = thread_id % 2;

//     int i = tile_row * nHalf;
//     int j = tile_col * nHalf;

//     printf("Started thread %d on tile (%d, %d) on hart %d\n",
//            thread_id, tile_row, tile_col, hart_id);

//     timing_t start_ct = timing_counter_get();
//     uint64_t start_rd = read_cycle();

//     for (int k = 0; k < N; k += nHalf) {
//         matmul_rvvt((float *)matrix_A, (float *)matrix_B, (float *)C_computed,
//                     i, j, k, N, N, N, nHalf);
//     }

//     timing_t end_ct = timing_counter_get();
//     uint64_t end_rd = read_cycle();

//     uint64_t cycles = timing_cycles_get(&start_ct, &end_ct);
//     uint64_t ns = timing_cycles_to_ns(cycles);



//     thread_info[thread_id] = {
//         .thread_id = thread_id,
//         .tile_row = tile_row,
//         .tile_col = tile_col,
//         .hart_id = hart_id,
//         .assigned_cycle = assignment_cycles[thread_id],
//         .compute_cycles = cycles,
//         .compute_ns = ns,
//         .start_ct = start_ct,
//         .end_ct = end_ct
//     };

// }

int main(void)
{
    timing_init();
    timing_start();

    printf("Hello, C world! %s\n", CONFIG_BOARD);
    k_mutex_init(&print_mutex);
    for(int runs = 0; runs < 2; runs++) {
    printf("Starting run %d\n", runs);
    uint64_t launch_start = read_cycle();  
    for (int i = 0; i < NUM_THREADS; ++i) {
        assignment_cycles[i] = read_cycle(); 
        int hart_id = i % CONFIG_MP_MAX_NUM_CPUS;
        thread_ids[i] = k_thread_create(&thread_data[i],
                                        thread_stacks[i],
                                        STACK_SIZE,
                                        tile_thread_function,
                                        (void *)(uintptr_t)i,
                                        NULL, NULL,
                                        K_PRIO_PREEMPT(1), 0, K_FOREVER);
        k_thread_cpu_pin(thread_ids[i], hart_id);
        k_thread_start(thread_ids[i]);
    }
    uint64_t launch_end = read_cycle();  
    uint64_t launch_cycles = launch_end - launch_start;

    join_start = timing_counter_get();
    for (int i = 0; i < NUM_THREADS; ++i) {
        k_thread_join(thread_ids[i], K_FOREVER);
    }
    join_end = timing_counter_get();
    uint64_t join_cycles = timing_cycles_get(&join_start, &join_end);

    printf("\nPer-thread execution summary:\n");
    for (int i = 0; i < NUM_THREADS; ++i) {
        const ThreadInfo &info = thread_info[i];
        printf("Thread %d | Tile (%d, %d) | Hart %d | Assigned @ %llu | Compute = %llu cycles (~%llu ns)\n",
               info.thread_id, info.tile_row, info.tile_col, info.hart_id,
               info.assigned_cycle, info.compute_cycles, info.compute_ns);
    }

    // timing_t global_start = thread_info[0].start_ct;
    // timing_t global_end = thread_info[0].end_ct;
    // for (int i = 1; i < NUM_THREADS; ++i) {
    //     if (timing_cycles_get(&thread_info[i].start_ct, &global_start) > 0)
    //         global_start = thread_info[i].start_ct;
    //     if (timing_cycles_get(&global_end, &thread_info[i].end_ct) > 0)
    //         global_end = thread_info[i].end_ct;
    // }
    // uint64_t total_execution_cycles = timing_cycles_get(&global_start, &global_end);

    printf("\nSynchronization timing:\n");
    printf("Total thread launch overhead (cycles): %llu\n", launch_cycles);
    printf("Total thread join time (cycles):   %llu\n", join_cycles);
    
    // printf("Total parallel execution time (cycles): %llu\n", total_execution_cycles);

    // Validation
    float tolerance = 1e-1f;
    bool valid = true;
    for (int i = 0; i < N && valid; ++i) {
        for (int j = 0; j < N; ++j) {
            float diff = C_computed[i][j] - matrix_C[i][j];
            if (diff < -tolerance || diff > tolerance) {
                valid = false;
                break;
            }
        }
    }

    if (valid) {
        printf("Validation passed: Parallel tiled matmul matches the precomputed output.\n");
    } else {
        printf("Validation failed: Parallel tiled matmul does not match the precomputed output.\n");
    }

    printf("\nTop-left 4x4 block of the computed matrix (C_computed):\n");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            printf("%f ", C_computed[i][j]);
        }
        printf("\n");
    }

    printf("\nTop-left 4x4 block of the expected matrix (matrix_C):\n");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            printf("%f ", matrix_C[i][j]);
        }
        printf("\n");
    }
    }

    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}

// #include <zephyr/kernel.h>
// #include <zephyr/sys/reboot.h>
// #include <zephyr/arch/cpu.h>
// #include <stdio.h>
// #include <string.h>
// #include "../data/matrices_128.hpp"
// #include <riscv_vector.h>
// #include <zephyr/timing/timing.h>

// constexpr int N = 128;
// constexpr int nHalf = N / 2;

// #define NUM_THREADS 4
// #define STACK_SIZE 16384

// struct ThreadInfo {
//     int thread_id;
//     int tile_row;
//     int tile_col;
//     int hart_id;
//     uint64_t assigned_cycle;
//     uint64_t compute_cycles;
//     uint64_t compute_ns;
//     timing_t start_ct;
//     timing_t end_ct;
// };
// ThreadInfo thread_info[NUM_THREADS];


// K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);
// struct k_thread thread_data[NUM_THREADS];
// k_tid_t thread_ids[NUM_THREADS];
// struct k_mutex print_mutex;

// float C_computed[N][N] = {0};
// uint64_t assignment_cycles[NUM_THREADS];
// uint64_t start_cycles[NUM_THREADS] = {0};
// uint64_t end_cycles[NUM_THREADS] = {0};
// timing_t launch_start, launch_end;
// timing_t join_start, join_end;

// static inline uint64_t read_cycle() {
//     uint64_t cycle;
//     asm volatile("rdcycle %0" : "=r"(cycle));
//     return cycle;
// }

// inline void matmul_rvvt(float *a, float *b, float *c,
//                         int i, int j, int k, int n, int m, int o, int tile_size) {
//     float *A = a + i * o + k;
//     float *B_tile = b + k * m + j;
//     float *C = c + i * m + j;

//     int N = i + tile_size <= n ? tile_size : n % tile_size;
//     int M = j + tile_size <= m ? tile_size : m % tile_size;
//     int O = k + tile_size <= o ? tile_size : o % tile_size;

//     size_t vlmax = __riscv_vsetvlmax_e32m1();
//     vfloat32m1_t vec_zero = __riscv_vfmv_v_f_f32m1(0, vlmax);

//     for (int I = 0; I < N; ++I) {
//         for (int J = 0; J < M; ++J) {
//             float *ptr_a = A + I * o;
//             float *ptr_b = B_tile + J;
//             int K = O;

//             vfloat32m1_t vec_s = __riscv_vfmv_v_f_f32m1(0, vlmax);
//             for (size_t vl, l = 0; K > 0; K -= vl, ptr_a += vl, ptr_b += vl * m, l += vl) {
//                 vl = __riscv_vsetvl_e32m1(K);
//                 vfloat32m1_t vec_a = __riscv_vle32_v_f32m1(ptr_a, vl);
//                 vfloat32m1_t vec_b = __riscv_vlse32_v_f32m1(ptr_b, m * sizeof(float), vl);
//                 vec_s = __riscv_vfmacc_vv_f32m1(vec_s, vec_a, vec_b, vl);
//             }

//             vfloat32m1_t vec_sum = __riscv_vfredusum_vs_f32m1_f32m1(vec_s, vec_zero, vlmax);
//             float sum = __riscv_vfmv_f_s_f32m1_f32(vec_sum);
//             C[I * m + J] = k == 0 ? sum : C[I * m + J] + sum;
//         }
//     }
// }

// void tile_thread_function(void *arg1, void *arg2, void *arg3)
// {
//     int thread_id = (int)(uintptr_t)arg1;
//     int tile_row = thread_id / 2;
//     int tile_col = thread_id % 2;

//     int i = tile_row * nHalf;
//     int j = tile_col * nHalf;

//     // uint64_t start_ct, end_ct;
//     // uint64_t start_rd, end_rd;

//     timing_t start_ct = timing_counter_get();
//     uint64_t start_rd = read_cycle();

//     for (int k = 0; k < N; k += nHalf) {
//         matmul_rvvt((float *)matrix_A, (float *)matrix_B, (float *)C_computed,
//                     i, j, k, N, N, N, nHalf);
//     }

//     timing_t end_ct = timing_counter_get();

//     uint64_t end_rd = read_cycle();

//     uint64_t cycles = timing_cycles_get(&start_ct, &end_ct);
//     uint64_t ns = timing_cycles_to_ns(cycles);

//     int hart_id = arch_curr_cpu()->id;

//     thread_info[thread_id] = {
//         .thread_id = thread_id,
//         .tile_row = tile_row,
//         .tile_col = tile_col,
//         .hart_id = hart_id,
//         .assigned_cycle = assignment_cycles[thread_id],
//         .compute_cycles = cycles,
//         .compute_ns = ns,
//         .start_ct = start_ct,
//         .end_ct = end_ct
//     };
// }
// //     // Store result to be printed later
// //     thread_info[thread_id] = {
// //         .thread_id = thread_id,
// //         .tile_row = tile_row,
// //         .tile_col = tile_col,
// //         .hart_id = hart_id,
// //         .assigned_cycle = assignment_cycles[thread_id],
// //         .compute_cycles = cycles,
// //         .compute_ns = ns
// //     };
// // }
// // void tile_thread_function(void *arg1, void *arg2, void *arg3)
// // {
// //     int thread_id = (int)(uintptr_t)arg1;
// //     int tile_row = thread_id / 2;
// //     int tile_col = thread_id % 2;

// //     int i = tile_row * nHalf;
// //     int j = tile_col * nHalf;

// //     start_cycles[thread_id] = read_cycle();

// //     for (int k = 0; k < N; k += nHalf) {
// //         matmul_rvvt((float *)matrix_A, (float *)matrix_B, (float *)C_computed,
// //                     i, j, k, N, N, N, nHalf);
// //     }

// //     end_cycles[thread_id] = read_cycle();

// //     k_mutex_lock(&print_mutex, K_FOREVER);
// //     int hart_id = arch_curr_cpu()->id;
// //     printf("Thread %d computed tile (%d, %d) on hart %d\n", thread_id, tile_row, tile_col, hart_id);
// //     printf("Thread %d finished execution (compute cycles = %llu)\n",
// //            thread_id, end_cycles[thread_id] - start_cycles[thread_id]);
// //     k_mutex_unlock(&print_mutex);
// // }

// int main(void)
// {
//     timing_init();
//     timing_start();


//     printf("Hello, C world! %s\n", CONFIG_BOARD);
//     k_mutex_init(&print_mutex);

//     timing_t launch_start = timing_counter_get();
//     for (int i = 0; i < NUM_THREADS; ++i) {
//         assignment_cycles[i] = timing_counter_get();

//         thread_ids[i] = k_thread_create(&thread_data[i],
//                                         thread_stacks[i],
//                                         STACK_SIZE,
//                                         tile_thread_function,
//                                         (void *)(uintptr_t)i,
//                                         NULL, NULL,
//                                         K_PRIO_PREEMPT(1), 0, K_FOREVER);
//         k_thread_start(thread_ids[i]);
//     }
//     timing_t launch_end = timing_counter_get();
//     // launch_start = read_cycle();
//     // for (int i = 0; i < NUM_THREADS; ++i) {
//     //     int hart_id = i % CONFIG_MP_MAX_NUM_CPUS;
//     //     assignment_cycles[i] = read_cycle();

//     //     thread_ids[i] = k_thread_create(&thread_data[i],
//     //                                     thread_stacks[i],
//     //                                     STACK_SIZE,
//     //                                     tile_thread_function,
//     //                                     (void *)(uintptr_t)i,
//     //                                     NULL, NULL,
//     //                                     K_PRIO_PREEMPT(1), 0, K_FOREVER);

//     //     k_mutex_lock(&print_mutex, K_FOREVER);
//     //     printf("Thread %d assigned to hart %d (assigned @ cycle = %llu)\n", i, hart_id, assignment_cycles[i]);
//     //     k_mutex_unlock(&print_mutex);
//     //     k_thread_start(thread_ids[i]);
//     // } 
//     // launch_end = read_cycle();
//     join_start = timing_counter_get();
//     for (int i = 0; i < NUM_THREADS; ++i) {
//         k_thread_join(thread_ids[i], K_FOREVER);
//     }
//     join_end = timing_counter_get();

//     printf("\nPer-thread execution summary:\n");
//     for (int i = 0; i < NUM_THREADS; ++i) {
//         const ThreadInfo &info = thread_info[i];
//         printf("Thread %d | Tile (%d, %d) | Hart %d | Assigned @ %llu | Compute = %llu cycles (~%llu ns)\n",
//             info.thread_id, info.tile_row, info.tile_col, info.hart_id,
//             info.assigned_cycle, info.compute_cycles, info.compute_ns);
//     }
//     // join_start = read_cycle();
//     // for (int i = 0; i < NUM_THREADS; ++i) {
//     //     k_thread_join(thread_ids[i], K_FOREVER);
//     // }
//     // join_end = read_cycle();

//     // // Print per-thread timing
//     // printf("\nPer-thread execution timing:\n");
//     // for (int i = 0; i < NUM_THREADS; ++i) {
//     //     printf("Thread %d: start = %llu, end = %llu, delta = %llu\n",
//     //            i, start_cycles[i], end_cycles[i], end_cycles[i] - start_cycles[i]);
//     // }

//     // Compute total parallel execution time
//     timing_t global_start = thread_info[0].start_ct;
//     timing_t global_end = thread_info[0].end_ct;

//     for (int i = 1; i < NUM_THREADS; ++i) {
//         if (timing_cycles_get(&thread_info[i].start_ct, &global_start) > 0)
//             global_start = thread_info[i].start_ct;
//         if (timing_cycles_get(&global_end, &thread_info[i].end_ct) > 0)
//             global_end = thread_info[i].end_ct;
//     }

//     uint64_t total_execution_cycles = timing_cycles_get(&global_start, &global_end);
//     uint64_t launch_cycles = timing_cycles_get(&launch_start, &launch_end);
//     uint64_t join_cycles = timing_cycles_get(&join_start, &join_end);

//     printf("launch_start raw: %llu\n", (uint64_t)launch_start);
//     printf("launch_end raw:   %llu\n", (uint64_t)launch_end);   
//     printf("\nSynchronization timing:\n");
//     printf("Total thread launch overhead (cycles): %llu\n", launch_cycles);
//     printf("Total thread join overhead (cycles):   %llu\n", join_cycles);
//     printf("Total parallel execution time (cycles): %llu\n", total_execution_cycles);

//     // Validation
//     float tolerance = 1e-5f;
//     bool valid = true;
//     for (int i = 0; i < N && valid; ++i) {
//         for (int j = 0; j < N; ++j) {
//             float diff = C_computed[i][j] - matrix_C[i][j];
//             if (diff < -tolerance || diff > tolerance) {
//                 valid = false;
//                 break;
//             }
//         }
//     }

//     if (valid) {
//         printf("Validation passed: Parallel tiled matmul matches the precomputed output.\n");
//     } else {
//         printf("Validation failed: Parallel tiled matmul does not match the precomputed output.\n");
//     }

//     // Print top-left 4x4 blocks
//     printf("\nTop-left 4x4 block of the computed matrix (C_computed):\n");
//     for (int i = 0; i < 4; ++i) {
//         for (int j = 0; j < 4; ++j) {
//             printf("%f ", C_computed[i][j]);
//         }
//         printf("\n");
//     }

//     printf("\nTop-left 4x4 block of the expected matrix (matrix_C):\n");
//     for (int i = 0; i < 4; ++i) {
//         for (int j = 0; j < 4; ++j) {
//             printf("%f ", matrix_C[i][j]);
//         }
//         printf("\n");
//     }

//     sys_reboot(SYS_REBOOT_COLD);
//     return 0;
// }
