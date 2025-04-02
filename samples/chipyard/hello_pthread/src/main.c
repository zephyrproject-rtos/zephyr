/*
 * Copyright (c) 2025 Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#define NUM_THREADS 4  // Number of threads to create

pthread_mutex_t print_mutex;

void *thread_function(void *arg)
{
    int thread_id = *(int *)arg;
    int hart_id;
    volatile int i;

    pthread_mutex_lock(&print_mutex);
    hart_id = arch_curr_cpu()->id;
    printf("Thread %d running on hart %d\n", thread_id, hart_id);
    pthread_mutex_unlock(&print_mutex);

    // Simulate some work by busy-waiting
    for (i = 0; i < 1000000; i++) {
        /* busy-wait loop */
    }
    return NULL;
}

int main(void)
{
    pthread_mutex_init(&print_mutex, NULL);
    printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // Create multiple threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_function, &thread_ids[i]) != 0) {
            printf("Failed to create thread %d\n", i);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads finished execution. Rebooting...\n");
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}

