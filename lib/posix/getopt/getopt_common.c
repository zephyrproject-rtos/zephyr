/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include "getopt.h"

/* Referring  below variables is not thread safe. They reflects getopt state
 * only when 1 thread is using getopt.
 * When more threads are using getopt please call getopt_state_get to know
 * getopt state for the current thread.
 */
int opterr = 1;	/* if error message should be printed */
int optind = 1;	/* index into parent argv vector */
int optopt;	/* character checked for validity */
int optreset;	/* reset getopt */
char *optarg;	/* argument associated with option */

/* Common state for all threads that did not have own getopt state. */
static struct getopt_state m_getopt_common_state = {
	.opterr = 1,
	.optind = 1,
	.optopt = 0,
	.optreset = 0,
	.optarg = NULL,

	.place = "", /* EMSG */

#if CONFIG_GETOPT_LONG
	.nonopt_start = -1, /* first non option argument (for permute) */
	.nonopt_end = -1, /* first option after non options (for permute) */
#endif
};

/* This function is not thread safe. All threads using getopt are calling
 * this function.
 */
void z_getopt_global_state_update(struct getopt_state *state)
{
	opterr = state->opterr;
	optind = state->optind;
	optopt = state->optopt;
	optreset = state->optreset;
	optarg = state->optarg;
}

/* It is internal getopt API function, it shall not be called by the user. */
struct getopt_state *getopt_state_get(void)
{
#if CONFIG_SHELL_GETOPT
	k_tid_t tid;

	tid = k_current_get();
	STRUCT_SECTION_FOREACH(shell, sh) {
		if (tid == sh->ctx->tid) {
			return &sh->ctx->getopt;
		}
	}
#endif
	/* If not a shell thread return a common pointer */
	return &m_getopt_common_state;
}
