#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>
#include <stdio.h>
#include <string.h>
//#include "../data/matrices_128.hpp"
#include "../data/matrices.hpp"
#include <riscv_vector.h>
#include <zephyr/timing/timing.h>
#include <math.h>

constexpr int N = 32;
constexpr int nHalf = N / 2;

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
            k_mutex_lock(&print_mutex, K_FOREVER);
            printf("Thread loop iter %d, %d.\n", I, J);
            k_mutex_unlock(&print_mutex);
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

    enable_vector_operations();
    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Thread %d started and waiting.\n", thread_id);
    k_mutex_unlock(&print_mutex );

    while (true) {
        k_sem_take(&task.start_sem, K_FOREVER);
        k_mutex_lock(&print_mutex, K_FOREVER);
        printf("Thread %d started computation.\n", thread_id);
        k_mutex_unlock(&print_mutex );

        task.start_ct = timing_counter_get();
        for (int k = 0; k < N; k += task.tile_size) {
            k_mutex_lock(&print_mutex, K_FOREVER);
            printf("Thread %d loop iter %d.\n", thread_id, k);
            k_mutex_unlock(&print_mutex );
            matmul_rvvt(task.a, task.b, task.c,
                        task.i, task.j, k, N, N, N, task.tile_size);
        }
        task.end_ct = timing_counter_get();

        k_mutex_lock(&print_mutex, K_FOREVER);
        printf("Thread %d finished computation.\n", thread_id);
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

uint64_t execute_task(float *a, float *b, float *c, int tile_size) {
    timing_t task_start = timing_counter_get();

    for (int t = 0; t < NUM_THREADS; ++t) {
        int tile_row = t / 2;
        int tile_col = t % 2;

        thread_tasks[t].a = a;
        thread_tasks[t].b = b;
        thread_tasks[t].c = c;
        thread_tasks[t].i = tile_row * tile_size;
        thread_tasks[t].j = tile_col * tile_size;
        thread_tasks[t].tile_size = tile_size;

        k_sem_give(&thread_tasks[t].start_sem);
    }

    for (int t = 0; t < NUM_THREADS; ++t) {
        k_sem_take(&thread_tasks[t].done_sem, K_FOREVER);
    }

    timing_t task_end = timing_counter_get();

    return timing_cycles_get(&task_start, &task_end);
}

void report_timing(const char *label, uint64_t total_cycles) {
    uint64_t total_ns = timing_cycles_to_ns(total_cycles);
    printf("%s Total time (including overhead): %llu cycles (%llu ns)\n", label, total_cycles, total_ns);
}

int main(void) {
    timing_init();
    timing_start();
    k_mutex_init(&print_mutex);

    printf("Running matmul with threadpool on %s\n", CONFIG_BOARD);

    threadpool_init();
    k_sleep(K_MSEC(100));  // Ensure threads initialize and print their initial messages

    float C_computed[N][N] = {0};

    
    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Starting warm-up run.\n");
    k_mutex_unlock(&print_mutex);
    execute_task((float *)matrix_A, (float *)matrix_B, (float *)C_computed, nHalf);
    k_mutex_lock(&print_mutex, K_FOREVER);
    printf("Warm-up run completed.\n");
    k_mutex_unlock(&print_mutex);

    printf("Starting final measurement run.\n");
    uint64_t final_cycles = execute_task((float *)matrix_A, (float *)matrix_B, (float *)C_computed, nHalf);

    report_timing("Final run", final_cycles);

    // Validation (simplified)
    bool valid = true;
    float tolerance = 1e-2f;
    for (int i = 0; i < N && valid; ++i)
        for (int j = 0; j < N; ++j)
            if (fabs(C_computed[i][j] - matrix_C[i][j]) > tolerance) {
                valid = false;
                break;
            }

    printf(valid ? "Validation passed.\n" : "Validation failed.\n");

    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
