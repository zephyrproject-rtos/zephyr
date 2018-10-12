/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CORE_H_
#define ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CORE_H_

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	k_thread_entry_t entry_point;
	void *arg1;
	void *arg2;
	void *arg3;

	int thread_idx;

#if defined(CONFIG_ARCH_HAS_THREAD_ABORT)
	/* The kernel may indicate that a thread has been aborted several */
	/* times */
	int aborted;
#endif

	/*
	 * Note: If more elements are added to this structure, remember to
	 * update ARCH_POSIX_RECOMMENDED_STACK_SIZE in the configuration
	 * Currently it is 24 = 4 pointers + 2 ints = 4*4 + 2*4
	 */
} posix_thread_status_t;


void posix_new_thread(posix_thread_status_t *ptr);
void posix_swap(int next_allowed_thread_nbr, int this_thread_nbr);
void posix_main_thread_start(int next_allowed_thread_nbr);
void posix_init_multithreading(void);
void posix_core_clean_up(void);

void posix_new_thread_pre_start(void); /* defined in thread.c */
void posix_irq_check_idle_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_POSIX_CORE_H_ */
