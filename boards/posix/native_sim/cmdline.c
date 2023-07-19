/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * To support native_posix drivers or tests which register their own arguments
 * we provide the same API as in native_posix
 */

#include "nsi_cmdline.h"

void native_add_command_line_opts(struct args_struct_t *args)
{
	nsi_add_command_line_opts(args);
}

void native_get_cmd_line_args(int *argc, char ***argv)
{
	nsi_get_cmd_line_args(argc, argv);
}

void native_get_test_cmd_line_args(int *argc, char ***argv)
{
	nsi_get_test_cmd_line_args(argc, argv);
}
