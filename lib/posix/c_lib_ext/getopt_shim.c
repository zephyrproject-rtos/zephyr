/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/sys_getopt.h>

#include "getopt.h"

char *optarg;
int opterr, optind, optopt;

static struct sys_getopt_state	getopt_state = SYS_GETOPT_STATE_INITIALIZER;

static void getopt_global_state_get(void)
{
	getopt_state.opterr = opterr;
	getopt_state.optind = optind;
	getopt_state.optopt = optopt;
	getopt_state.optarg = optarg;
}

static void getopt_global_state_put(void)
{
	opterr = getopt_state.opterr;
	optind = getopt_state.optind;
	optopt = getopt_state.optopt;
	optarg = getopt_state.optarg;
}

int getopt(int argc, char *const argv[], const char *optstring)
{
	int ret;

	getopt_global_state_get();
	ret = sys_getopt_r(argc, argv, optstring, &getopt_state);
	getopt_global_state_put();
	return ret;
}

int getopt_long(int argc, char *const argv[], const char *shortopts,
		const struct option *longopts, int *longind)
{
	int ret;

	getopt_global_state_get();
	ret = sys_getopt_long_r(argc, argv, shortopts, longopts, longind, &getopt_state);
	getopt_global_state_put();
	return ret;
}

int getopt_long_only(int argc, char *const argv[], const char *shortopts,
		     const struct option *longopts, int *longind)
{
	int ret;

	getopt_global_state_get();
	ret = sys_getopt_long_only_r(argc, argv, shortopts, longopts, longind, &getopt_state);
	getopt_global_state_put();
	return ret;
}
