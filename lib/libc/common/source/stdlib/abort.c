/*
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <signal.h>
#include <zephyr/kernel.h>

FUNC_NORETURN void abort(void)
{
	/*
	 * According to C11 7.22.4.1, abort() causes abnormal program
	 * termination. It first calls raise(SIGABRT), and if that returns,
	 * it must ensure the program never returns.
	 */
	raise(SIGABRT);

	/* If raise(SIGABRT) returns, ensure we never return */
	printk("abort()\n");
	k_panic();
	CODE_UNREACHABLE;
}
