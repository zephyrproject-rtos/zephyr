/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "bs_tracing.h"
#include "posix_board_if.h"

#define _STDOUT_BUF_SIZE 256
static char stdout_buff[_STDOUT_BUF_SIZE];
static int n_pend; /* Number of pending characters in buffer */

int bsim_print_char(int c)
{
	int printnow = 0;

	if ((c != '\n') && (c != '\r')) {
		stdout_buff[n_pend++] = c;
		stdout_buff[n_pend] = 0;
	} else {
		printnow = 1;
	}

	if (n_pend >= _STDOUT_BUF_SIZE - 1) {
		printnow = 1;
	}

	if (printnow) {
		bs_trace_print(BS_TRACE_RAW, NULL, 0, 2, BS_TRACE_AUTOTIME, 0,
				"%s\n", stdout_buff);
		n_pend = 0;
		stdout_buff[0] = 0;
	}
	return c;
}

/**
 * Ensure that whatever was written thru printk is displayed now
 */
void posix_flush_stdout(void)
{
	if (n_pend) {
		stdout_buff[n_pend] = 0;
		bs_trace_print(BS_TRACE_RAW, NULL, 0, 2, BS_TRACE_AUTOTIME, 0,
				"%s", stdout_buff);
		n_pend = 0;
		stdout_buff[0] = 0;
		fflush(stdout);
	}
}

/*
 * @brief Initialize the driver that provides the printk output
 *
 * @return 0 if successful, otherwise failed.
 */
static int bsim_console_init(void)
{

#ifdef CONFIG_PRINTK
	extern void __printk_hook_install(int (*fn)(int));
	__printk_hook_install(bsim_print_char);
#endif

	return 0;
}

SYS_INIT(bsim_console_init, PRE_KERNEL_1, CONFIG_BSIM_CONSOLE_INIT_PRIORITY);
