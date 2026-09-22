/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>  /* for printfs */
#include <stdarg.h> /* for va args */
#include <unistd.h>
#include "nsi_tasks.h"
#include "nsi_cmdline.h"
#include "nsi_tracing.h"
#include "nsi_main.h"

/**
 * Are stdout and stderr connected to a tty
 * 0  = no
 * 1  = yes
 * -1 = we do not know yet
 * Indexed 0:stdout, 1:stderr
 */
static int is_a_tty[2] = {-1, -1};

#define ST_OUT 0
#define ST_ERR 1

static void decide_about_color(void)
{
	if (is_a_tty[0] == -1) {
		is_a_tty[0] = isatty(STDOUT_FILENO);
	}
	if (is_a_tty[1] == -1) {
		is_a_tty[1] = isatty(STDERR_FILENO);
	}
}

#define ERROR 0
#define WARN 1
#define TRACE 2

static const char * const trace_type_esc_start[] = {
	"\x1b[1;31m", /* ERROR   - Foreground color = red, bold */
	"\x1b[95m",   /* WARNING - Foreground color = magenta */
	"\x1b[0;39m", /* TRACE   - reset all styles */
};

static const char trace_esc_end[] = "\x1b[0;39m"; /* Reset all styles */

void nsi_vprint_warning(const char *format, va_list vargs)
{
	if (is_a_tty[ST_ERR] == -1) {
		decide_about_color();
	}
	if (is_a_tty[ST_ERR]) {
		fprintf(stderr, "%s", trace_type_esc_start[WARN]);
	}

	vfprintf(stderr, format, vargs);

	if (is_a_tty[ST_ERR]) {
		fprintf(stderr, "%s", trace_esc_end);
	}
}

void nsi_vprint_error_and_exit(const char *format, va_list vargs)
{
	if (is_a_tty[ST_ERR] == -1) {
		decide_about_color();
	}
	if (is_a_tty[ST_ERR]) {
		fprintf(stderr, "%s", trace_type_esc_start[ERROR]);
	}

	vfprintf(stderr, format, vargs);

	if (is_a_tty[ST_ERR]) {
		fprintf(stderr, "%s\n", trace_esc_end);
	}

	nsi_exit(1);
}

void nsi_vprint_trace(const char *format, va_list vargs)
{
	if (is_a_tty[ST_OUT] == -1) {
		decide_about_color();
	}
	if (is_a_tty[ST_OUT]) {
		fprintf(stdout, "%s", trace_type_esc_start[TRACE]);
	}

	vfprintf(stdout, format, vargs);

	if (is_a_tty[ST_OUT]) {
		fprintf(stdout, "%s", trace_esc_end);
	}
}

static void trace_disable_color(char *argv, int offset)
{
	is_a_tty[0] = 0;
	is_a_tty[1] = 0;
}

static void trace_enable_color(char *argv, int offset)
{
	is_a_tty[0] = -1;
	is_a_tty[1] = -1;

}

static void trace_force_color(char *argv, int offset)
{
	is_a_tty[0] = 1;
	is_a_tty[1] = 1;
}

int nsi_trace_over_tty(int file_number)
{
	return is_a_tty[file_number];
}

NSI_TASK(decide_about_color, PRE_BOOT_2, 0);

static void nsi_add_tracing_options(void)
{
	static struct args_struct_t trace_options[] = {
		{
			.is_switch = true,
			.option = "color",
			.type = 'b',
			.call_when_found = trace_enable_color,
			.descript = "(default) Enable color in traces if printing to console"
		},
		{
			.is_switch = true,
			.option = "no-color",
			.type = 'b',
			.call_when_found = trace_disable_color,
			.descript = "Disable color in traces even if printing to console"
		},
		{
			.is_switch = true,
			.option = "force-color",
			.type = 'b',
			.call_when_found = trace_force_color,
			.descript = "Enable color in traces even if printing to files/pipes"
		},
		ARG_TABLE_ENDMARKER
	};

	nsi_add_command_line_opts(trace_options);
}

NSI_TASK(nsi_add_tracing_options, PRE_BOOT_1, 0);
