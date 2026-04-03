/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include "nsi_tracing.h"

void nsi_print_error_and_exit(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	nsi_vprint_error_and_exit(format, variable_args);
	va_end(variable_args);
}

void nsi_print_warning(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	nsi_vprint_warning(format, variable_args);
	va_end(variable_args);
}

void nsi_print_trace(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	nsi_vprint_trace(format, variable_args);
	va_end(variable_args);
}
