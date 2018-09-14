/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdio.h>
#include <init.h>
#include "bs_tracing.h"

#define _STDOUT_BUF_SIZE 256
static char stdout_buff[_STDOUT_BUF_SIZE];
static int n_pend; /* Number of pending characters in buffer */

int print_char(int c)
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
 * Flush to the terminal any possible pending printk
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
static int printk_init(struct device *arg)
{
	ARG_UNUSED(arg);

	extern void __printk_hook_install(int (*fn)(int));
	__printk_hook_install(print_char);

	return 0;
}

SYS_INIT(printk_init, PRE_KERNEL_1, CONFIG_PRINTK_HOOK_INIT_PRIORITY);

void posix_print_error_and_exit(const char *format, ...)
{
	va_list variable_argsp;

	va_start(variable_argsp, format);
	bs_trace_vprint(BS_TRACE_ERROR, NULL, 0, 0, BS_TRACE_AUTOTIME, 0,
			format, variable_argsp);
	va_end(variable_argsp);
}

void posix_print_warning(const char *format, ...)
{
	va_list variable_argsp;

	va_start(variable_argsp, format);
	bs_trace_vprint(BS_TRACE_WARNING, NULL, 0, 0, BS_TRACE_AUTOTIME, 0,
			format, variable_argsp);
	va_end(variable_argsp);
}

void posix_print_trace(const char *format, ...)
{
	va_list variable_argsp;

	va_start(variable_argsp, format);
	bs_trace_vprint(BS_TRACE_RAW, NULL, 0, 2, BS_TRACE_AUTOTIME, 0,
			format, variable_argsp);
	va_end(variable_argsp);
}


int posix_trace_over_tty(int file_number)
{
	return bs_trace_is_tty(file_number);
}
