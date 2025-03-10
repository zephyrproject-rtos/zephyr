/*
 * Copyright (c) 2025 Cadence Design Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static int z_argc;
static char **z_argv;

void z_save_bootargs(int argc, char **argv)
{
	z_argc = argc;
	z_argv = argv;
}

char **prepare_main_args(int *argc)
{
	*argc = z_argc;
	return z_argv;
}
