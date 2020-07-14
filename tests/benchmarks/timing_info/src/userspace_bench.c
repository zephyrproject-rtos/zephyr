/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <kernel.h>
#include <zephyr.h>
#include <tc_util.h>
#include <ksched.h>
#include "timing_info.h"
#include <app_memory/app_memdomain.h>

K_APPMEM_PARTITION_DEFINE(bench_ptn);
struct k_mem_domain bench_domain;

extern char sline[256];
extern uint64_t arch_timing_enter_user_mode_end;

uint64_t drop_to_user_mode_start_time;

struct k_thread my_thread_user;
K_THREAD_STACK_EXTERN(my_stack_area);
K_THREAD_STACK_EXTERN(my_stack_area_0);


/******************************************************************************/
/* syscall needed to read timer value when in user space */
uint32_t z_impl_userspace_read_timer_value(void)
{
	TIMING_INFO_PRE_READ();
	return TIMING_INFO_GET_TIMER_VALUE();
}

static inline uint32_t z_vrfy_userspace_read_timer_value(void)
{
	TIMING_INFO_PRE_READ();
	return TIMING_INFO_GET_TIMER_VALUE();
}
#include <syscalls/userspace_read_timer_value_mrsh.c>

/******************************************************************************/

void drop_to_user_mode(void);
void user_thread_creation(void);
void syscall_overhead(void);
void validation_overhead(void);

void userspace_bench(void)
{
	struct k_mem_partition *parts[] = {
		&bench_ptn
	};

	k_mem_domain_init(&bench_domain, ARRAY_SIZE(parts), parts);
	k_mem_domain_add_thread(&bench_domain, k_current_get());

	drop_to_user_mode();

	user_thread_creation();

	syscall_overhead();

	validation_overhead();
}
/******************************************************************************/

void test_drop_to_user_mode_1(void *p1, void *p2, void *p3)
{
	volatile uint32_t dummy = 100U;

	dummy++;
}

void drop_to_user_mode_thread(void *p1, void *p2, void *p3)
{
	TIMING_INFO_PRE_READ();
	drop_to_user_mode_start_time = TIMING_INFO_GET_TIMER_VALUE();
	k_thread_user_mode_enter(test_drop_to_user_mode_1, NULL, NULL, NULL);
}


void drop_to_user_mode(void)
{
#ifdef SysTick
	/* Reset the counter so that a interrupt doesn't happen between
	 * the benchmark test
	 */
	SysTick->VAL = 0;
#endif

	/* Test time to drop to usermode from SU */

	k_thread_create(&my_thread_user, my_stack_area_0, STACK_SIZE,
			drop_to_user_mode_thread,
			NULL, NULL, NULL,
			-1 /*priority*/, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();

	uint32_t total_cycles =
		SUBTRACT_CLOCK_CYCLES(arch_timing_enter_user_mode_end) -
		SUBTRACT_CLOCK_CYCLES(drop_to_user_mode_start_time);

	PRINT_STATS("Drop to user mode", total_cycles);
}

/******************************************************************************/
void user_thread_creation(void)
{
	uint64_t user_thread_creation_end_time, user_thread_creation_start_time;

	TIMING_INFO_PRE_READ();
	user_thread_creation_start_time = TIMING_INFO_GET_TIMER_VALUE();

	k_thread_create(&my_thread_user, my_stack_area, STACK_SIZE,
			test_drop_to_user_mode_1,
			NULL, NULL, NULL,
			0 /*priority*/, K_INHERIT_PERMS | K_USER, K_FOREVER);

	TIMING_INFO_PRE_READ();
	user_thread_creation_end_time = TIMING_INFO_GET_TIMER_VALUE();
	k_thread_abort(&my_thread_user);

	uint32_t total_cycles =
		SUBTRACT_CLOCK_CYCLES(user_thread_creation_end_time) -
		SUBTRACT_CLOCK_CYCLES(user_thread_creation_start_time);

	PRINT_STATS("User thread creation", total_cycles);
}

/******************************************************************************/
/* dummy syscalls creation */
K_APP_BMEM(bench_ptn) uint64_t syscall_overhead_start_time,
	syscall_overhead_end_time;

int z_impl_k_dummy_syscall(void)
{
	return 0;
}

static inline int z_vrfy_k_dummy_syscall(void)
{
	TIMING_INFO_PRE_READ();
	syscall_overhead_end_time = TIMING_INFO_GET_TIMER_VALUE();
	return 0;
}
#include <syscalls/k_dummy_syscall_mrsh.c>


void syscall_overhead_user_thread(void *p1, void *p2, void *p3)
{
	syscall_overhead_start_time = userspace_read_timer_value();
	int val = k_dummy_syscall();

	val |= 0xFF;
}


void syscall_overhead(void)
{
	k_thread_create(&my_thread_user, my_stack_area_0, STACK_SIZE,
			syscall_overhead_user_thread,
			NULL, NULL, NULL,
			-1 /*priority*/, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	uint32_t total_cycles =
		SUBTRACT_CLOCK_CYCLES(syscall_overhead_end_time) -
		SUBTRACT_CLOCK_CYCLES(syscall_overhead_start_time);

	PRINT_STATS("Syscall overhead", total_cycles);
}

/******************************************************************************/
K_SEM_DEFINE(test_sema, 1, 10);
uint32_t validation_overhead_obj_init_start_time;
uint32_t validation_overhead_obj_init_end_time;
uint32_t validation_overhead_obj_start_time;
uint32_t validation_overhead_obj_end_time;

int z_impl_validation_overhead_syscall(void)
{
	return 0;
}

static inline int z_vrfy_validation_overhead_syscall(void)
{
	TIMING_INFO_PRE_READ();
	validation_overhead_obj_init_start_time = TIMING_INFO_GET_TIMER_VALUE();

	bool status_0 = Z_SYSCALL_OBJ_INIT(&test_sema, K_OBJ_SEM);

	TIMING_INFO_PRE_READ();
	validation_overhead_obj_init_end_time = TIMING_INFO_GET_TIMER_VALUE();


	TIMING_INFO_PRE_READ();
	validation_overhead_obj_start_time = TIMING_INFO_GET_TIMER_VALUE();

	bool status_1 = Z_SYSCALL_OBJ(&test_sema, K_OBJ_SEM);

	TIMING_INFO_PRE_READ();
	validation_overhead_obj_end_time = TIMING_INFO_GET_TIMER_VALUE();
	return status_0 || status_1;
}
#include <syscalls/validation_overhead_syscall_mrsh.c>

void validation_overhead_user_thread(void *p1, void *p2, void *p3)
{
	/* get validation numbers */
	validation_overhead_syscall();
}

void validation_overhead(void)
{
	k_thread_access_grant(k_current_get(), &test_sema);

	k_thread_create(&my_thread_user, my_stack_area, STACK_SIZE,
			validation_overhead_user_thread,
			NULL, NULL, NULL,
			-1 /*priority*/, K_INHERIT_PERMS | K_USER, K_NO_WAIT);


	uint32_t total_cycles_obj_init =
		SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_init_end_time) -
		SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_init_start_time);

	uint32_t total_cycles_obj =
		SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_end_time) -
		SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_start_time);

	PRINT_STATS("Validation overhead k_object init",
		    total_cycles_obj_init);

	PRINT_STATS("Validation overhead k_object permission",
		    total_cycles_obj);
}
