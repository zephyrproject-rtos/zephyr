/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdarg.h>
#include "bs_tracing.h"
#include "posix_board_if.h"

/*
 * Provide the posix_print_* functions required from all POSIX arch boards
 * (This functions provide a lower level, more direct, print mechanism than
 * Zephyr's printk or logger and therefore can be relied on even if the kernel
 * is down.
 */

void posix_print_error_and_exit(const char *format, ...)
{
	va_list variable_argsp;

	va_start(variable_argsp, format);
	bs_trace_vprint(BS_TRACE_WARNING, NULL, 0, 0, BS_TRACE_AUTOTIME, 0,
			format, variable_argsp);
	va_end(variable_argsp);
	posix_exit(1);
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
