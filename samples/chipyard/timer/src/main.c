/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This example measures three types of operations:
 *  1. A compute-bound dummy function (dummy_work)
 *  2. A busy-wait delay using k_busy_wait()
 *  3. A sleep delay using k_msleep()
 *
 * For each test, both the Zephyr timing API (mtime based) and the raw
 * rdcycle measurement (which reads the CPU cycle counter) are used.
 */

#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>
#include <zephyr/sys/reboot.h>
#include <stdio.h>

/* rdcycle: Read the raw cycle counter using inline assembly.
 * This provides the CPU's raw cycle count from the mcycle CSR.
 */
inline unsigned long rdcycle(void)
{
    unsigned long cc;
    __asm__ volatile("rdcycle %0" : "=r"(cc));
    return cc;
}

/* A dummy function to simulate a compute-bound workload */
void dummy_work(void)
{
    volatile int sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += i;
    }
}

int main(void)
{
    printf("Starting timing tests:\n");

    volatile timing_t start_counter, end_counter;
    uint64_t cycles, ns;
    unsigned long start_rd, end_rd, cycles_rd;

    /* Test 1: Measure the execution time of dummy_work() */
    start_counter = timing_counter_get();
    start_rd = rdcycle();
    dummy_work();
    end_counter = timing_counter_get();
    end_rd = rdcycle();

    cycles = timing_cycles_get(&start_counter, &end_counter);
    ns = timing_cycles_to_ns(cycles);
    cycles_rd = end_rd - start_rd;
    printf("dummy_work() took %llu cycles (timing API), %llu ns, and %lu cycles (rdcycle)\n",
           cycles, ns, cycles_rd);

    /* Test 2: Measure a busy-wait delay (k_busy_wait for 1000 microseconds) */
    start_counter = timing_counter_get();
    start_rd = rdcycle();
    k_busy_wait(1000);  // Busy wait for 1000 microseconds (1 ms)
    end_counter = timing_counter_get();
    end_rd = rdcycle();

    cycles = timing_cycles_get(&start_counter, &end_counter);
    ns = timing_cycles_to_ns(cycles);
    cycles_rd = end_rd - start_rd;
    printf("k_busy_wait(1000 us) took %llu cycles (timing API), %llu ns, and %lu cycles (rdcycle)\n",
           cycles, ns, cycles_rd);

    /* Test 3: Measure a sleep delay (k_msleep for 1 ms) */
    start_counter = timing_counter_get();
    start_rd = rdcycle();
    k_msleep(1);  // Sleep for 1 milliseconds
    end_counter = timing_counter_get();
    end_rd = rdcycle();

    cycles = timing_cycles_get(&start_counter, &end_counter);
    ns = timing_cycles_to_ns(cycles);
    cycles_rd = end_rd - start_rd;
    printf("k_msleep(1) took %llu cycles (timing API), %llu ns, and %lu cycles (rdcycle)\n",
           cycles, ns, cycles_rd);


    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}
