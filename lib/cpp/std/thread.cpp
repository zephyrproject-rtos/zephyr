/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _GLIBCXX_VISIBILITY(x)
#define _GLIBCXX_THREAD_ABI_COMPAT 1

#include <threads.h>

#include <thread>

#include <zephyr/init.h>
#include <zephyr/kernel.h>

static void __z_cpp_thread_terminate() FUNC_NORETURN;
static void __z_cpp_thread_terminate()
{
	thrd_exit(0);
}

static int __z_cpp_thread_init(void)
{
	/* use thrd_exit(0) instead of std::abort() to avoid kernel panic */
	std::set_terminate(__z_cpp_thread_terminate);
	return 0;
}
SYS_INIT(__z_cpp_thread_init, PRE_KERNEL_1, 0);
