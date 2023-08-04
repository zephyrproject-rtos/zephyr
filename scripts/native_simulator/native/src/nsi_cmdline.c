/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "nsi_cmdline.h"
#include "nsi_cmdline_internal.h"
#include "nsi_tracing.h"
#include "nsi_timer_model.h"
#include "nsi_hw_scheduler.h"
#include "nsi_tasks.h"

static int s_argc, test_argc;
static char **s_argv, **test_argv;

static struct args_struct_t *args_struct;
static int used_args;
static int args_aval;
#define ARGS_ALLOC_CHUNK_SIZE 20

static void nsi_cleanup_cmd_line(void)
{
	if (args_struct != NULL) { /* LCOV_EXCL_BR_LINE */
		free(args_struct);
		args_struct = NULL;
	}
}

NSI_TASK(nsi_cleanup_cmd_line, ON_EXIT_POST, 0);

/**
 * Add a set of command line options to the program.
 *
 * Each option to be added is described in one entry of the input <args>
 * This input must be terminated with an entry containing ARG_TABLE_ENDMARKER.
 */
void nsi_add_command_line_opts(struct args_struct_t *args)
{
	int count = 0;

	while (args[count].option != NULL) {
		count++;
	}
	count++; /*for the end marker*/

	if (used_args + count >= args_aval) {
		int growby = count;
		/* reallocs are expensive let's do them only in big chunks */
		if (growby < ARGS_ALLOC_CHUNK_SIZE) {
			growby = ARGS_ALLOC_CHUNK_SIZE;
		}

		struct args_struct_t *new_args_struct = realloc(args_struct,
				      (args_aval + growby)*
				      sizeof(struct args_struct_t));
		args_aval += growby;
		/* LCOV_EXCL_START */
		if (new_args_struct == NULL) {
			nsi_print_error_and_exit("Could not allocate memory");
		} else {
			args_struct = new_args_struct;
		}
		/* LCOV_EXCL_STOP */
	}

	memcpy(&args_struct[used_args], args,
		count*sizeof(struct args_struct_t));

	used_args += count - 1;
	/*
	 * -1 as the end marker should be overwritten next time something
	 * is added
	 */
}

void nsi_add_testargs_option(void)
{
	static struct args_struct_t testargs_options[] = {
		{
		.manual = true,
		.option = "testargs",
		.name = "arg",
		.type = 'l',
		.descript = "Any argument that follows will be ignored by the top level, "
			    "and made available for possible tests"
		},
		ARG_TABLE_ENDMARKER
	};

	nsi_add_command_line_opts(testargs_options);
}

static void print_invalid_opt_error(char *argv)
{
	nsi_print_error_and_exit("Incorrect option '%s'. Did you misspell it?"
				 " Is that feature supported in this build?\n",
				 argv);

}

/**
 * Handle possible command line arguments.
 *
 * We also store them for later use by possible test applications
 */
void nsi_handle_cmd_line(int argc, char *argv[])
{
	int i;

	nsi_add_testargs_option();

	s_argv = argv;
	s_argc = argc;

	nsi_cmd_args_set_defaults(args_struct);

	for (i = 1; i < argc; i++) {

		if ((nsi_cmd_is_option(argv[i], "testargs", 0))) {
			test_argc = argc - i - 1;
			test_argv = &argv[i+1];
			break;
		}

		if (!nsi_cmd_parse_one_arg(argv[i], args_struct)) {
			nsi_cmd_print_switches_help(args_struct);
			print_invalid_opt_error(argv[i]);
		}
	}
}

/**
 * The application/test can use this function to inspect all the command line
 * arguments
 */
void nsi_get_cmd_line_args(int *argc, char ***argv)
{
	*argc = s_argc;
	*argv = s_argv;
}

/**
 * The application/test can use this function to inspect the command line
 * arguments received after --testargs
 */
void nsi_get_test_cmd_line_args(int *argc, char ***argv)
{
	*argc = test_argc;
	*argv = test_argv;
}
