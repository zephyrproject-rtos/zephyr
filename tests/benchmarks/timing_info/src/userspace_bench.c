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


extern char sline[256];
extern u64_t __end_drop_to_usermode_time;

u32_t drop_to_user_mode_end_time, drop_to_user_mode_start_time;
u32_t user_thread_creation_end_time, user_thread_creation_start_time;

__kernel struct k_thread my_thread_user;
K_THREAD_STACK_EXTERN(my_stack_area);
K_THREAD_STACK_EXTERN(my_stack_area_0);


/******************************************************************************/
/* syscall needed to read timer value when in user space */
u32_t _impl_userspace_read_timer_value(void)
{
	TIMING_INFO_PRE_READ();
	return TIMING_INFO_GET_TIMER_VALUE();
}

Z_SYSCALL_HANDLER(userspace_read_timer_value)
{
	TIMING_INFO_PRE_READ();
	return TIMING_INFO_GET_TIMER_VALUE();
}

/******************************************************************************/

void drop_to_user_mode(void);
void user_thread_creation(void);
void syscall_overhead(void);
void validation_overhead(void);

void userspace_bench(void)
{
	drop_to_user_mode();

	user_thread_creation();

	syscall_overhead();

	validation_overhead();

}
/******************************************************************************/

void test_drop_to_user_mode_1(void *p1, void *p2, void *p3)
{
	volatile u32_t dummy = 100;

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
	/* Reset the counter so that a interrupt doesn't happen inbetween
	 * the benchmark test
	 */
	SysTick->VAL = 0;
#endif

	/* Test time to drop to usermode from SU */

	k_thread_create(&my_thread_user, my_stack_area_0, STACK_SIZE,
			drop_to_user_mode_thread,
			NULL, NULL, NULL,
			-1 /*priority*/, K_INHERIT_PERMS, 0);

	k_yield();

	drop_to_user_mode_end_time = (u32_t)
		SUBTRACT_CLOCK_CYCLES(__end_drop_to_usermode_time);

	u32_t tmp_start_time =
		SUBTRACT_CLOCK_CYCLES(drop_to_user_mode_start_time);

	u32_t total_drop_to_user_mode_time =
		CYCLES_TO_NS((u32_t)
			     ((drop_to_user_mode_end_time - tmp_start_time) &
			      0xFFFFFFFFULL));

	PRINT_STATS("Drop to user mode",
		    (u32_t)((drop_to_user_mode_end_time - tmp_start_time) &
		     0xFFFFFFFFULL),
		    (u32_t) (total_drop_to_user_mode_time  & 0xFFFFFFFFULL));



}

/******************************************************************************/
void user_thread_creation(void)
{
	u32_t total_user_thread_creation_time;

	TIMING_INFO_PRE_READ();
	user_thread_creation_start_time = TIMING_INFO_GET_TIMER_VALUE();

	k_thread_create(&my_thread_user, my_stack_area, STACK_SIZE,
			test_drop_to_user_mode_1,
			NULL, NULL, NULL,
			0 /*priority*/, K_INHERIT_PERMS | K_USER, 0);

	TIMING_INFO_PRE_READ();
	user_thread_creation_end_time = TIMING_INFO_GET_TIMER_VALUE();
	k_thread_abort(&my_thread_user);

	u32_t total_cycles = (u32_t)
		((SUBTRACT_CLOCK_CYCLES(user_thread_creation_end_time) -
		  SUBTRACT_CLOCK_CYCLES(user_thread_creation_start_time)) &
		 0xFFFFFFFFULL);

	total_user_thread_creation_time = CYCLES_TO_NS(total_cycles);

	PRINT_STATS("User thread Creation",
		    total_cycles,
		    (u32_t) (total_user_thread_creation_time  & 0xFFFFFFFFULL));

}

/******************************************************************************/
/* dummy syscalls creation */
u32_t syscall_overhead_start_time, syscall_overhead_end_time;

int _impl_k_dummy_syscall(void)
{
	return 0;
}

Z_SYSCALL_HANDLER(k_dummy_syscall)
{
	TIMING_INFO_PRE_READ();
	syscall_overhead_end_time = TIMING_INFO_GET_TIMER_VALUE();
	return 0;
}


void syscall_overhead_user_thread(void *p1, void *p2, void *p3)
{
	syscall_overhead_start_time = userspace_read_timer_value();
	int val = k_dummy_syscall();

	val |= 0xFF;
}


void syscall_overhead(void)
{
	u32_t total_syscall_overhead_time;

	k_thread_create(&my_thread_user, my_stack_area_0, STACK_SIZE,
			syscall_overhead_user_thread,
			NULL, NULL, NULL,
			-1 /*priority*/, K_INHERIT_PERMS | K_USER, 0);


	u32_t total_cycles = (u32_t)
		((SUBTRACT_CLOCK_CYCLES(syscall_overhead_end_time) -
		  SUBTRACT_CLOCK_CYCLES(syscall_overhead_start_time)) &
		 0xFFFFFFFFULL);

	total_syscall_overhead_time = CYCLES_TO_NS(total_cycles);

	PRINT_STATS("Syscall overhead",
		    total_cycles,
		    (u32_t) (total_syscall_overhead_time  & 0xFFFFFFFFULL));

}

/******************************************************************************/
K_SEM_DEFINE(test_sema, 1, 10);
u32_t validation_overhead_obj_init_start_time;
u32_t validation_overhead_obj_init_end_time;
u32_t validation_overhead_obj_start_time;
u32_t validation_overhead_obj_end_time;

int _impl_validation_overhead_syscall(void)
{
	return 0;
}

Z_SYSCALL_HANDLER(validation_overhead_syscall)
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


void validation_overhead_user_thread(void *p1, void *p2, void *p3)
{
	/* get validation numbers */
	validation_overhead_syscall();
}

void validation_overhead(void)
{

	k_thread_access_grant(k_current_get(), &test_sema, NULL);


	k_thread_create(&my_thread_user, my_stack_area, STACK_SIZE,
			validation_overhead_user_thread,
			NULL, NULL, NULL,
			-1 /*priority*/, K_INHERIT_PERMS | K_USER, 0);


	u32_t total_cycles_obj_init = (u32_t)
		((SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_init_end_time) -
		  SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_init_start_time)
		  ) & 0xFFFFFFFFULL);

	u32_t total_cycles_obj = (u32_t)
		((SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_end_time) -
		  SUBTRACT_CLOCK_CYCLES(validation_overhead_obj_start_time)) &
		 0xFFFFFFFFULL);

	u32_t total_validation_overhead_obj_init_time =
		CYCLES_TO_NS(total_cycles_obj_init);

	u32_t  total_validation_overhead_obj_time =
		CYCLES_TO_NS(total_cycles_obj);

	PRINT_STATS("Validation overhead k object init",
		    total_cycles_obj_init,
		    (u32_t) (total_validation_overhead_obj_init_time  &
			     0xFFFFFFFFULL));

	PRINT_STATS("Validation overhead k object permission",
		    total_cycles_obj,
		    (u32_t) (total_validation_overhead_obj_time  &
			     0xFFFFFFFFULL));


}
