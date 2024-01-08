/*
 * Copyright (c) 2016-2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/app_memory/app_memdomain.h>

#include "footprint.h"
#include "userspace.h"

#define STACK_SIZE	512

K_SEM_DEFINE(test_sema, 1, 10);

struct k_thread my_thread_user;

int z_impl_dummy_syscall(void)
{
	return 0;
}

static inline int z_vrfy_dummy_syscall(void)
{
	return 0;
}
#include <syscalls/dummy_syscall_mrsh.c>

int z_impl_validation_overhead_syscall(void)
{
	return 0;
}

static inline int z_vrfy_validation_overhead_syscall(void)
{
	bool status_0 = K_SYSCALL_OBJ_INIT(&test_sema, K_OBJ_SEM);

	bool status_1 = K_SYSCALL_OBJ(&test_sema, K_OBJ_SEM);

	return status_0 || status_1;
}
#include <syscalls/validation_overhead_syscall_mrsh.c>


void test_drop_to_user_mode_1(void *p1, void *p2, void *p3)
{
	volatile uint32_t dummy = 100U;

	dummy++;
}

void drop_to_user_mode_thread(void *p1, void *p2, void *p3)
{
	k_thread_user_mode_enter(test_drop_to_user_mode_1, NULL, NULL, NULL);
}

void drop_to_user_mode(void)
{
	k_thread_create(&my_thread_user, my_stack_area_0, STACK_SIZE,
			drop_to_user_mode_thread,
			NULL, NULL, NULL,
			-1, K_INHERIT_PERMS, K_NO_WAIT);

	k_yield();
}

void user_thread_creation(void)
{
	k_thread_create(&my_thread_user, my_stack_area, STACK_SIZE,
			test_drop_to_user_mode_1,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS | K_USER, K_FOREVER);

	k_thread_abort(&my_thread_user);
}

void syscall_overhead_user_thread(void *p1, void *p2, void *p3)
{
	int val = dummy_syscall();

	val |= 0xFF;
}

void syscall_overhead(void)
{
	k_thread_create(&my_thread_user, my_stack_area_0, STACK_SIZE,
			syscall_overhead_user_thread,
			NULL, NULL, NULL,
			-1, K_INHERIT_PERMS | K_USER, K_NO_WAIT);
}

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
}

void run_userspace(void)
{
	k_mem_domain_add_thread(&footprint_mem_domain, k_current_get());

	drop_to_user_mode();

	user_thread_creation();

	syscall_overhead();

	validation_overhead();

}
