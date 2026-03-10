/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#if CONFIG_SHELL_GETOPT
#include <zephyr/sys/iterable_sections.h>
#endif

#include <zephyr/sys/sys_getopt.h>

/* Referring  below variables is not thread safe. They reflects getopt state
 * only when 1 thread is using getopt.
 * When more threads are using getopt please call getopt_state_get to know
 * getopt state for the current thread.
 */
int sys_getopt_opterr = 1; /* if error message should be printed */
int sys_getopt_optind = 1; /* index into parent argv vector */
int sys_getopt_optopt;     /* character checked for validity */
int sys_getopt_optreset;   /* reset getopt */
char *sys_getopt_optarg;   /* argument associated with option */

/* Common state for all threads that did not have own getopt state. */
static struct sys_getopt_state m_getopt_common_state = {
	.opterr = 1,
	.optind = 1,
	.optopt = 0,
	.optreset = 0,
	.optarg = NULL,

	.place = "", /* EMSG */

#if CONFIG_GETOPT_LONG
	.nonopt_start = -1, /* first non option argument (for permute) */
	.nonopt_end = -1,   /* first option after non options (for permute) */
#endif
};

/* Shim function to update global variables in getopt_shim.c if user wants to
 * still use the original non-posix compliant getopt. The shim will be deprecated
 * and eventually removed in the future.
 */
extern void z_getopt_global_state_update_shim(struct sys_getopt_state *state);

/* This function is not thread safe. All threads using getopt are calling
 * this function.
 */
void z_getopt_global_state_update(struct sys_getopt_state *state)
{
	sys_getopt_opterr = state->opterr;
	sys_getopt_optind = state->optind;
	sys_getopt_optopt = state->optopt;
	sys_getopt_optreset = state->optreset;
	sys_getopt_optarg = state->optarg;

	if (!IS_ENABLED(CONFIG_NATIVE_LIBC) && IS_ENABLED(CONFIG_POSIX_C_LIB_EXT)) {
		z_getopt_global_state_update_shim(state);
	}
}

/* It is internal getopt API function, it shall not be called by the user. */
struct sys_getopt_state *sys_getopt_state_get(void)
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
