/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


static int s_argc;
static char **s_argv;

/**
 * Store the command line arguments for later use.
 * If this board already handles any, we do so here
 */
void native_handle_cmd_line(int argc, char *argv[])
{
	s_argv = argv;
	s_argc = argc;
}


/**
 * The application/test can use this function to inspect the command line
 * arguments
 */
void native_get_cmd_line_args(int *argc, char ***argv)
{
	*argc = s_argc;
	*argv = s_argv;
}
