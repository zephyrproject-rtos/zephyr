/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define __need_getopt_legacy
#include "getopt_common.h"

/* Referring  below variables is not thread safe. They reflects getopt state
 * only when 1 thread is using getopt.
 * When more threads are using getopt please use getopt_r and friends
 * to hold the getopt state in local variables
 */
int opterr;	/* if error message should be printed */
int optind;	/* index into parent argv vector */
int optopt;     /* character checked for validity */
int optreset;   /* reset getopt */
char *optarg;   /* argument associated with option */

static char *place;
#if CONFIG_GETOPT_LONG
static int nonopt_start;
static int nonopt_end;
#endif

/* This function is not thread safe. All threads using getopt are calling
 * this function.
 */
void z_getopt_global_state_put(struct getopt_data *state)
{
	opterr = state->opterr;
	optind = state->optind;
	optopt = state->optopt;
	optreset = state->optreset;
	optarg = state->optarg;
	place = state->place;
#if CONFIG_GETOPT_LONG
	nonopt_start = state->nonopt_start;
	nonopt_end = state->nonopt_end;
#endif
}

/* Place the global state into a struct */
void z_getopt_global_state_get(struct getopt_data *state)
{
	state->opterr = opterr;
	state->optind = optind;
	state->optopt = optopt;
	state->optreset = optreset;
	state->optarg = optarg;
	state->place = place;
#if CONFIG_GETOPT_LONG
	state->nonopt_start = nonopt_start;
	state->nonopt_end = nonopt_end;
#endif
}
