#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>
#include <stdio.h>
#include <string.h>
// #include "../data/matrices_128.hpp"
#include "../data/matrices.hpp"
// #include "../data/matrices_8.hpp"
#include <riscv_vector.h>
#include <zephyr/timing/timing.h>
#include <math.h>

constexpr int N_CFG = 32;
constexpr int nHalf = N_CFG / 2;

#define NUM_THREADS 4
#define STACK_SIZE 16384 * 4

struct ThreadTask {
    float *a, *b, *c;
    int i, j, tile_size;
    struct k_sem start_sem;
    struct k_sem done_sem;
    timing_t start_ct;
    timing_t end_ct;
} thread_tasks[NUM_THREADS];

struct TaskTiming {
    timing_t dispatch;
    timing_t first_start;
    timing_t last_end;
    timing_t all_done;
};


K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);
struct k_thread thread_data[NUM_THREADS];
k_tid_t thread_ids[NUM_THREADS];
struct k_mutex print_mutex;

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

    vfloat32m1_t vec_a;
    vfloat32m1_t vec_b;

    for (int I = 0; I < N; ++I) {
        for (int J = 0; J < M; ++J) {
            float *ptr_a = A + I * o;
            float *ptr_b = B_tile + J;
            int K = O;

            vfloat32m1_t vec_s = __riscv_vfmv_v_f_f32m1(0, vlmax);
            for (size_t vl, l = 0; K > 0; K -= vl, ptr_a += vl, ptr_b += vl * m, l += vl) {
                vl = __riscv_vsetvl_e32m1(K);
                vec_a = __riscv_vle32_v_f32m1(ptr_a, vl);
                vec_b = __riscv_vlse32_v_f32m1(ptr_b, m * sizeof(float), vl);
            // k_mutex_lock(&print_mutex, K_FOREVER);
            // printf("Thread loop iter %d, %d.\n", I, J);
            // k_mutex_unlock(&print_mutex);
            //     enable_vector_operations();
                vec_s = __riscv_vfmacc_vv_f32m1(vec_s, vec_a, vec_b, vl);
            }

            vfloat32m1_t vec_sum = __riscv_vfredusum_vs_f32m1_f32m1(vec_s, vec_zero, vlmax);
            float sum = __riscv_vfmv_f_s_f32m1_f32(vec_sum);
            C[I * m + J] = k == 0 ? sum : C[I * m + J] + sum;
        }
    }
}


void worker_thread(void *arg1, void *, void *) {
    int thread_id = (int)(uintptr_t)arg1;
    ThreadTask &task = thread_tasks[thread_id];

    // enable_vector_operations();
    // k_mutex_lock(&print_mutex, K_FOREVER);
    // printf("Thread %d started and waiting.\n", thread_id);
    // k_mutex_unlock(&print_mutex );

    while (true) {
        k_sem_take(&task.start_sem, K_FOREVER);
        // k_mutex_lock(&print_mutex, K_FOREVER);
        // printf("Thread %d started computation.\n", thread_id);
        // k_mutex_unlock(&print_mutex );

        task.start_ct = timing_counter_get();
        for (int k = 0; k < N_CFG; k += task.tile_size) {
            // k_mutex_lock(&print_mutex, K_FOREVER);
            // printf("Thread %d loop iter %d.\n", thread_id, k);
            // k_mutex_unlock(&print_mutex );
            matmul_rvvt(task.a, task.b, task.c,
                        task.i, task.j, k, N_CFG, N_CFG, N_CFG, task.tile_size);
        }
        task.end_ct = timing_counter_get();

        k_mutex_lock(&print_mutex, K_FOREVER);
        // printf("Thread %d Done\n", thread_id);
        k_mutex_unlock(&print_mutex );
        k_sem_give(&task.done_sem);
    }
}

void threadpool_init() {
    for (int i = 0; i < NUM_THREADS; ++i) {
        k_sem_init(&thread_tasks[i].start_sem, 0, 1);
        k_sem_init(&thread_tasks[i].done_sem, 0, 1);

        thread_ids[i] = k_thread_create(&thread_data[i], thread_stacks[i], STACK_SIZE,
                                        worker_thread, (void *)(uintptr_t)i, NULL, NULL,
                                        K_PRIO_PREEMPT(1), 0, K_FOREVER);
        k_thread_cpu_pin(thread_ids[i], i % CONFIG_MP_MAX_NUM_CPUS);
        k_thread_start(thread_ids[i]);
    }
}


TaskTiming execute_task(float *a, float *b, float *c, int tile_size) {
    timing_t dispatch = timing_counter_get();

    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_tasks[i].a = a;
        thread_tasks[i].b = b;
        thread_tasks[i].c = c;
        thread_tasks[i].tile_size = tile_size;
        thread_tasks[i].i = (i / 2) * tile_size;
        thread_tasks[i].j = (i % 2) * tile_size;
        k_sem_give(&thread_tasks[i].start_sem);
    }

    timing_t first_start = thread_tasks[0].start_ct;
    timing_t last_end = thread_tasks[0].end_ct;

    for (int i = 0; i < NUM_THREADS; ++i) {
        k_sem_take(&thread_tasks[i].done_sem, K_FOREVER);

        if (timing_cycles_get(&thread_tasks[i].start_ct, &first_start) > 0)
            first_start = thread_tasks[i].start_ct;

        if (timing_cycles_get(&last_end, &thread_tasks[i].end_ct) > 0)
            last_end = thread_tasks[i].end_ct;
    }

    timing_t all_done = timing_counter_get();
    return {dispatch, first_start, last_end, all_done};
}
void report_timing(const char *label, TaskTiming t) {
    uint64_t start_overhead_cycles = timing_cycles_get(&t.dispatch, &t.first_start);
    uint64_t compute_cycles        = timing_cycles_get(&t.first_start, &t.last_end);
    uint64_t sync_overhead_cycles  = timing_cycles_get(&t.last_end, &t.all_done);
    uint64_t total_cycles          = timing_cycles_get(&t.dispatch, &t.all_done);

    uint64_t start_overhead_ns = timing_cycles_to_ns(start_overhead_cycles);
    uint64_t compute_ns        = timing_cycles_to_ns(compute_cycles);
    uint64_t sync_overhead_ns  = timing_cycles_to_ns(sync_overhead_cycles);
    uint64_t total_ns          = timing_cycles_to_ns(total_cycles);

    printf("\n=== Timing Report: %s ===\n", label);
    printf("Start overhead:      %llu ns (%llu mtime cycles)\n", start_overhead_ns, start_overhead_cycles);
    printf("Compute time:        %llu ns (%llu mtime cycles)\n", compute_ns, compute_cycles);
    printf("Sync overhead:       %llu ns (%llu mtime cycles)\n", sync_overhead_ns, sync_overhead_cycles);
    printf("Total time:          %llu ns (%llu mtime cycles)\n", total_ns, total_cycles);
}

int main(void) {
    timing_init();
    timing_start();
    k_mutex_init(&print_mutex);

    printf("Running matmul of size %d with threadpool on %s\n", N_CFG, CONFIG_BOARD);

    threadpool_init();
    // k_sleep(K_MSEC(100));  // Ensure threads initialize and print their initial messages

    float C_computed[N_CFG][N_CFG] = {0};

    
    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Starting warm-up run.\n");
    k_mutex_unlock(&print_mutex);
    execute_task((float *)matrix_A, (float *)matrix_B, (float *)C_computed, nHalf);
    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Warm-up run completed.\n");
    k_mutex_unlock(&print_mutex);

    printf("Starting final measurement run.\n");
    TaskTiming t_final = execute_task((float *)matrix_A, (float *)matrix_B, (float *)C_computed, nHalf);

    report_timing("Final run", t_final);

    // Validation (simplified)
    bool valid = true;
    float tolerance = 1e-2f;
    for (int i = 0; i < N_CFG && valid; ++i)
        for (int j = 0; j < N_CFG; ++j)
            if (fabs(C_computed[i][j] - matrix_C[i][j]) > tolerance) {
                valid = false;
                printf("Error at index %d, %d, expected %f, got %f\n", i, j, C_computed[i][j], matrix_C[i][j]);
                break;
            }

    printf(valid ? "Validation passed.\n" : "Validation failed.\n");

    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
