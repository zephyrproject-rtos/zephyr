/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_SOC_INF_CLOCK_SOC_H
#define _POSIX_SOC_INF_CLOCK_SOC_H

#include "board_soc.h"
#include "posix_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

void posix_soc_clean_up(void);

/**
 * NATIVE_EXIT_TASK
 *
 * Register a function to be called during termination of the native application
 * execution.
 *
 * The function must take no parameters and return nothing.
 *
 * This can be used to close files, free memory, etc.
 *
 * Note that this function will be called when neither the Zephyr kernel or
 * any Zephyr thread is running anymore.
 */
#define NATIVE_EXIT_TASK(fn)	\
	static void (*_CONCAT(__native_exit_task_, fn)) __used	\
	__attribute__((__section__(".native_exit_tasks"))) = fn

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_SOC_INF_CLOCK_SOC_H */
