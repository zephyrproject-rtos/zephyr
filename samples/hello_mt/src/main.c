/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>  // Required for arch_curr_cpu()->id

#define NUM_THREADS 4  // Number of threads to create
#define NUM_HARTS CONFIG_MP_MAX_NUM_CPUS  // Number of available harts (cores)
#define STACK_SIZE 1024  // Stack size per thread

K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, NUM_THREADS, STACK_SIZE);
struct k_thread thread_data[NUM_THREADS];
k_tid_t thread_ids[NUM_THREADS];

struct k_mutex print_mutex;

void thread_function(void *arg1, void *arg2, void *arg3)
{
    int thread_id = (int)(uintptr_t) arg1;
    int hart_id;
    // for(int i=0; i<10; i++) {
    //     hart_id = arch_curr_cpu()->id;
    //     printf(hart_id);
    // }
    // hart_id = arch_curr_cpu()->id;
    // k_thread_cpu_pin(k_current_get(), hart_id);

    k_mutex_lock(&print_mutex, K_FOREVER);
    hart_id = arch_curr_cpu()->id;
    printf("Thread %d running on hart %d\n", thread_id, hart_id);
    k_mutex_unlock(&print_mutex);


    k_mutex_lock(&print_mutex, K_FOREVER);
    hart_id = arch_curr_cpu()->id;
    printf("Thread %d on hart %d finished execution\n", thread_id, hart_id);
    k_mutex_unlock(&print_mutex);
}

int main(void)
{
    printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
    printf("Zephyr is running on %d CPUs\n", CONFIG_MP_MAX_NUM_CPUS);
    k_mutex_init(&print_mutex);

    for (int i = 0; i < NUM_THREADS; i++) {
        int hart_id = i % NUM_HARTS;

        k_tid_t tid = k_thread_create(&thread_data[i], 
                                      thread_stacks[i], STACK_SIZE,
                                      thread_function, 
                                      (void *)(uintptr_t)i, NULL, NULL,
                                      K_PRIO_PREEMPT(1), 0, K_FOREVER);
        // k_thread_cpu_mask_clear(tid) ;
        k_thread_cpu_pin(tid, hart_id);
        k_thread_start(tid);
        thread_ids[i] = tid;

        k_mutex_lock(&print_mutex, K_FOREVER);
        printf("Thread %d assigned to hart %d\n", i, hart_id);
        k_mutex_unlock(&print_mutex);
    }


    for(int i = 0; i < NUM_THREADS; i++) {
        k_thread_join(thread_ids[i], K_FOREVER);
    }

    printf("All threads finished execution.\n");
    // printf("All threads finished execution. Rebooting...\n");
    // sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
