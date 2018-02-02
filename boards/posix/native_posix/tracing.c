/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Functions to print errors and traces
 */

#include <stdlib.h> /* for exit */
#include <stdio.h>  /* for printfs */
#include <stdarg.h> /* for va args */

#include "posix_board_if.h"


void posix_print_error_and_exit(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	vfprintf(stderr, format, variable_args);
	va_end(variable_args);
	posix_exit(1);
}

void posix_print_warning(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	vfprintf(stderr, format, variable_args);
	va_end(variable_args);
}

void posix_print_trace(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	vfprintf(stdout, format, variable_args);
	va_end(variable_args);
}
