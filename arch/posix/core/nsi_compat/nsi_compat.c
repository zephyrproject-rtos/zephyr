/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This module exist to provide a basic compatibility shim
 * from Native simulator components into the POSIX architecture.
 *
 * It is a transitional component, intended to facilitate
 * the migration towards the Native simulator.
 */

#include <zephyr/arch/posix/posix_trace.h>

void nsi_print_error_and_exit(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	posix_vprint_error_and_exit(format, variable_args);
	va_end(variable_args);
}

void nsi_print_warning(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	posix_vprint_warning(format, variable_args);
	va_end(variable_args);
}

void nsi_print_trace(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	posix_vprint_trace(format, variable_args);
	va_end(variable_args);
}

void nsi_exit(int exit_code)
{
	extern void posix_exit(int exit_code);

	posix_exit(exit_code);
}
