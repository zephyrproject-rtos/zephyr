/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline_common.h"
#include "zephyr/types.h"
#include "hw_models_top.h"
#include "timer_model.h"
#include "cmdline.h"
#include "toolchain.h"
#include "posix_trace.h"
#include "native_tracing.h"

static int s_argc, test_argc;
static char **s_argv, **test_argv;

static struct args_struct_t *args_struct;
static int used_args;
static int args_aval;
#define ARGS_ALLOC_CHUNK_SIZE 20

void native_cleanup_cmd_line(void)
{
	if (args_struct != NULL) { /* LCOV_EXCL_BR_LINE */
		free(args_struct);
		args_struct = NULL;
	}
}

/**
 * Add a set of command line options to the program.
 *
 * Each option to be added is described in one entry of the input <args>
 * This input must be terminated with an entry containing ARG_TABLE_ENDMARKER.
 */
void native_add_command_line_opts(struct args_struct_t *args)
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

		args_struct = realloc(args_struct,
				      (args_aval + growby)*
				      sizeof(struct args_struct_t));
		args_aval += growby;
		/* LCOV_EXCL_START */
		if (args_struct == NULL) {
			posix_print_error_and_exit("Could not allocate memory");
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

void native_add_testargs_option(void)
{
	static struct args_struct_t testargs_options[] = {
		/*
		 * Fields:
		 * manual, mandatory, switch,
		 * option_name, var_name ,type,
		 * destination, callback,
		 * description
		 */
		{true, false, false,
		"testargs", "arg", 'l',
		(void *)NULL, NULL,
		"Any argument that follows will be ignored by the top level, "
		"and made available for possible tests"},
		ARG_TABLE_ENDMARKER};

	native_add_command_line_opts(testargs_options);
}

/**
 * Handle possible command line arguments.
 *
 * We also store them for later use by possible test applications
 */
void native_handle_cmd_line(int argc, char *argv[])
{
	int i;

	native_add_tracing_options();
	native_add_testargs_option();

	s_argv = argv;
	s_argc = argc;

	cmd_args_set_defaults(args_struct);

	for (i = 1; i < argc; i++) {

		if ((cmd_is_option(argv[i], "testargs", 0))) {
			test_argc = argc - i - 1;
			test_argv = &argv[i+1];
			break;
		}

		if (!cmd_parse_one_arg(argv[i], args_struct)) {
			cmd_print_switches_help(args_struct);
			posix_print_error_and_exit("Incorrect option '%s'\n",
						   argv[i]);
		}
	}
}

/**
 * The application/test can use this function to inspect all the command line
 * arguments
 */
void native_get_cmd_line_args(int *argc, char ***argv)
{
	*argc = s_argc;
	*argv = s_argv;
}

/**
 * The application/test can use this function to inspect the command line
 * arguments received after --testargs
 */
void native_get_test_cmd_line_args(int *argc, char ***argv)
{
	*argc = test_argc;
	*argv = test_argv;
}
