/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "nsi_cpu_es_if.h"

/*
 * This file provides the interfaces the POSIX architecture and soc_inf
 * expect from all boards that use them
 */

void posix_exit(int exit_code)
{
	nsi_exit(exit_code);
}

void posix_vprint_error_and_exit(const char *format, va_list vargs)
{
	nsi_vprint_error_and_exit(format, vargs);
}

void posix_vprint_warning(const char *format, va_list vargs)
{
	nsi_vprint_warning(format, vargs);
}

void posix_vprint_trace(const char *format, va_list vargs)
{
	nsi_vprint_trace(format, vargs);
}

void posix_print_error_and_exit(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	nsi_vprint_error_and_exit(format, variable_args);
	va_end(variable_args);
}

void posix_print_warning(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	nsi_vprint_warning(format, variable_args);
	va_end(variable_args);
}

void posix_print_trace(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	nsi_vprint_trace(format, variable_args);
	va_end(variable_args);
}

int posix_trace_over_tty(int file_number)
{
	return nsi_trace_over_tty(file_number);
}

uint64_t posix_get_hw_cycle(void)
{
	return nsi_hws_get_time();
}
