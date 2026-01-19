/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/sys_getopt.h>

#include "getopt.h"

char *optarg;
int opterr, optind, optopt;

void getopt_init(void)
{
	sys_getopt_init();
}

struct getopt_state *getopt_state_get(void)
{
	return sys_getopt_state_get();
}

int getopt(int argc, char *const argv[], const char *optstring)
{
	return sys_getopt(argc, argv, optstring);
}

void z_getopt_global_state_update_shim(struct sys_getopt_state *state)
{
	opterr = state->opterr;
	optind = state->optind;
	optopt = state->optopt;
	optarg = state->optarg;
}

#if CONFIG_GETOPT_LONG
int getopt_long(int argc, char *const argv[], const char *shortopts,
		const struct option *longopts, int *longind)
{
	return sys_getopt_long(argc, argv, shortopts, longopts, longind);
}

int getopt_long_only(int argc, char *const argv[], const char *shortopts,
		     const struct option *longopts, int *longind)
{
	return sys_getopt_long_only(argc, argv, shortopts, longopts, longind);
}
#endif
