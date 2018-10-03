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
#include <unistd.h>
#include "soc.h"
#include "posix_board_if.h"
#include "cmdline.h"

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

/**
 * Are stdout and stderr connectd to a tty
 * 0  = no
 * 1  = yes
 * -1 = we do not know yet
 * Indexed 0:stdout, 1:stderr
 */
static int is_a_tty[2] = {-1, -1};

void trace_disable_color(char *argv, int offset)
{
	is_a_tty[0] = 0;
	is_a_tty[1] = 0;
}

void trace_enable_color(char *argv, int offset)
{
	is_a_tty[0] = -1;
	is_a_tty[1] = -1;

}

void trace_force_color(char *argv, int offset)
{
	is_a_tty[0] = 1;
	is_a_tty[1] = 1;
}

int posix_trace_over_tty(int file_number)
{
	return is_a_tty[file_number];
}

static void decide_about_color(void)
{
	if (is_a_tty[0] == -1) {
		is_a_tty[0] = isatty(STDOUT_FILENO);
	}
	if (is_a_tty[1] == -1) {
		is_a_tty[1] = isatty(STDERR_FILENO);
	}
}

NATIVE_TASK(decide_about_color, PRE_BOOT_2, 0);

void native_add_tracing_options(void)
{
	static struct args_struct_t trace_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{ false, false, true,
		"color", "color", 'b',
		NULL, trace_enable_color,
		"(default) Enable color in traces if printing to console"},
		{ false, false, true,
		"no-color", "no-color", 'b',
		NULL, trace_disable_color,
		"Disable color in traces even if printing to console"},
		{ false, false, true,
		"force-color", "force-color", 'b',
		NULL, trace_force_color,
		"Enable color in traces even if printing to files/pipes"},
		ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(trace_options);
}
