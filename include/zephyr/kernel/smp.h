/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_SMP_H_
#define ZEPHYR_INCLUDE_KERNEL_SMP_H_

typedef void (*smp_init_fn)(void *arg);

/**
 * @brief Start a CPU.
 *
 * This routine is used to manually start the CPU specified
 * by @a id. It may be called to restart a CPU that had been
 * stopped or powered down, as well as some other scenario.
 * After the CPU has finished initialization, the CPU will be
 * ready to participate in thread scheduling and execution.
 *
 * @note This function must not be used on currently running
 *       CPU. The target CPU must be in off state, or in
 *       certain architectural state(s) where the CPU is
 *       permitted to go through the power up process.
 *       Detection of such state(s) must be provided by
 *       the platform layers.
 *
 * @param id ID of target CPU.
 * @param fn Function to be called before letting scheduler
 *           run.
 * @param arg Argument to @a fn.
 */
void k_smp_cpu_start(int id, smp_init_fn fn, void *arg);

#endif /* ZEPHYR_INCLUDE_KERNEL_SMP_H_ */
