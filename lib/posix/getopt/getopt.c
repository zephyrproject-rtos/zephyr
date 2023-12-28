/*	$NetBSD: getopt.c,v 1.26 2003/08/07 16:43:40 agc Exp $	*/
/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 1987, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <string.h>
#ifdef CONFIG_NATIVE_LIBC
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif
#include "getopt.h"
#include "getopt_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(getopt);

#define	BADCH	((int)'?')
#define	BADARG	((int)':')
#define	EMSG	""

void getopt_init(void)
{
	struct getopt_state *state;

	state = getopt_state_get();

	state->opterr = 1;
	state->optind = 1;
	state->optopt = 0;
	state->optreset = 0;
	state->optarg = NULL;

	state->place = ""; /* EMSG */

#if CONFIG_GETOPT_LONG
	state->nonopt_start = -1; /* first non option argument (for permute) */
	state->nonopt_end = -1; /* first option after non options (for permute) */
#endif

	opterr = 1;
	optind = 1;
	optopt = 0;
	optreset = 0;
	optarg = NULL;
}

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int getopt(int nargc, char *const nargv[], const char *ostr)
{
	struct getopt_state *state;
	char *oli; /* option letter list index */

	/* get getopt state of the current thread */
	state = getopt_state_get();

	if (state->optreset || *state->place == 0) { /* update scanning pointer */
		state->optreset = 0;
		state->place = nargv[state->optind];
		if (state->optind >= nargc || *state->place++ != '-') {
			/* Argument is absent or is not an option */
			state->place = EMSG;
			z_getopt_global_state_update(state);
			return -1;
		}
		state->optopt = *state->place++;
		if (state->optopt == '-' && *state->place == 0) {
			/* "--" => end of options */
			++state->optind;
			state->place = EMSG;
			z_getopt_global_state_update(state);
			return -1;
		}
		if (state->optopt == 0) {
			/* Solitary '-', treat as a '-' option
			 * if the program (eg su) is looking for it.
			 */
			state->place = EMSG;
			if (strchr(ostr, '-') == NULL) {
				z_getopt_global_state_update(state);
				return -1;
			}
			state->optopt = '-';
		}
	} else {
		state->optopt = *state->place++;
	}

	/* See if option letter is one the caller wanted... */
	oli = strchr(ostr, state->optopt);
	if (state->optopt == ':' || oli == NULL) {
		if (*state->place == 0) {
			++state->optind;
		}
		if (state->opterr && *ostr != ':') {
			LOG_ERR("illegal option -- %c", state->optopt);
		}
		z_getopt_global_state_update(state);
		return BADCH;
	}

	/* Does this option need an argument? */
	if (oli[1] != ':') {
		/* don't need argument */
		state->optarg = NULL;
		if (*state->place == 0) {
			++state->optind;
		}
	} else {
		/* Option-argument is either the rest of this argument or the
		 * entire next argument.
		 */
		if (*state->place) {
			state->optarg = state->place;
		} else if (nargc > ++state->optind) {
			state->optarg = nargv[state->optind];
		} else {
			/* option-argument absent */
			state->place = EMSG;
			if (*ostr == ':') {
				z_getopt_global_state_update(state);
				return BADARG;
			}
			if (state->opterr) {
				LOG_ERR("option requires an argument -- %c",
					state->optopt);
			}
			z_getopt_global_state_update(state);
			return BADCH;
		}
		state->place = EMSG;
		++state->optind;
	}
	z_getopt_global_state_update(state);
	return state->optopt;	/* return option letter */
}
