/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Functions to print errors and traces
 *
 * A "board" meant to provide hooks into a simulator framework
 * can redefine them to route to that framework tracing facility
 */

#include <stdlib.h> /*for exit*/
#include <stdio.h>  /*for printfs*/
#include <stdarg.h> /*for va args*/

#include "pb_main.h"


void ps_print_error_and_exit(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	vfprintf(stderr, format, variable_args);
	va_end(variable_args);
	main_clean_up(1);
}

void ps_print_warning(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	vfprintf(stderr, format, variable_args);
	va_end(variable_args);
}

void ps_print_trace(const char *format, ...)
{
	va_list variable_args;

	va_start(variable_args, format);
	vfprintf(stdout, format, variable_args);
	va_end(variable_args);
}
