/*	$OpenBSD: getopt_long.c,v 1.22 2006/10/04 21:29:04 jmc Exp $	*/
/*	$NetBSD: getopt_long.c,v 1.15 2002/01/31 22:43:40 tv Exp $	*/

/*
 * Copyright (c) 2002 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F39502-99-1-0512.
 */
/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "getopt.h"
#include "getopt_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(getopt);

#define GNU_COMPATIBLE		/* Be more compatible, configure's use us! */

#define PRINT_ERROR	((state->opterr) && (*options != ':'))

#define FLAG_PERMUTE	0x01	/* permute non-options to the end of argv */
#define FLAG_ALLARGS	0x02	/* treat non-options as args to option "-1" */
#define FLAG_LONGONLY	0x04	/* operate as getopt_long_only */

/* return values */
#define	BADCH		(int)'?'
#define	BADARG		((*options == ':') ? (int)':' : (int)'?')
#define	INORDER	1

#define	EMSG		""

#ifdef GNU_COMPATIBLE
#define NO_PREFIX	(-1)
#define D_PREFIX	0
#define DD_PREFIX	1
#define W_PREFIX	2
#endif

static int getopt_internal(struct getopt_state *, int, char * const *,
			   const char *, const struct option *, int *, int);
static int parse_long_options(struct getopt_state *, char * const *,
			      const char *, const struct option *, int *, int,
			      int);
static int gcd(int, int);
static void permute_args(int, int, int, char *const *);

/* Error messages */
#define RECARGCHAR "option requires an argument -- %c"
#define ILLOPTCHAR "illegal option -- %c" /* From P1003.2 */
#ifdef GNU_COMPATIBLE
static int dash_prefix = NO_PREFIX;
#define GNUOPTCHAR "invalid option -- %c"

#define RECARGSTRING "option `%s%s' requires an argument"
#define AMBIG "option `%s%.*s' is ambiguous"
#define NOARG "option `%s%.*s' doesn't allow an argument"
#define ILLOPTSTRING "unrecognized option `%s%s'"
#else
#define RECARGSTRING "option requires an argument -- %s"
#define AMBIG "ambiguous option -- %.*s"
#define NOARG "option doesn't take an argument -- %.*s"
#define ILLOPTSTRING "unknown option -- %s"
#endif

/*
 * Compute the greatest common divisor of a and b.
 */
static int
gcd(int a, int b)
{
	int c;

	c = a % b;
	while (c != 0) {
		a = b;
		b = c;
		c = a % b;
	}

	return b;
}

/*
 * Exchange the block from nonopt_start to nonopt_end with the block
 * from nonopt_end to opt_end (keeping the same order of arguments
 * in each block).
 */
static void
permute_args(int panonopt_start, int panonopt_end, int opt_end,
	char * const *nargv)
{
	int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
	char *swap;

	/*
	 * compute lengths of blocks and number and size of cycles
	 */
	nnonopts = panonopt_end - panonopt_start;
	nopts = opt_end - panonopt_end;
	ncycle = gcd(nnonopts, nopts);
	cyclelen = (opt_end - panonopt_start) / ncycle;

	for (i = 0; i < ncycle; i++) {
		cstart = panonopt_end+i;
		pos = cstart;
		for (j = 0; j < cyclelen; j++) {
			if (pos >= panonopt_end) {
				pos -= nnonopts;
			} else {
				pos += nopts;
			}
			swap = nargv[pos];
			/* LINTED const cast */
			((char **) nargv)[pos] = nargv[cstart];
			/* LINTED const cast */
			((char **)nargv)[cstart] = swap;
		}
	}
}

/*
 * parse_long_options --
 *	Parse long options in argc/argv argument vector.
 * Returns -1 if short_too is set and the option does not match long_options.
 */
static int
parse_long_options(struct getopt_state *state, char * const *nargv,
		   const char *options,	const struct option *long_options,
		   int *idx, int short_too, int flags)
{
	char *current_argv, *has_equal;
#ifdef GNU_COMPATIBLE
	char *current_dash;
#endif
	size_t current_argv_len;
	int i, match, exact_match, second_partial_match;

	current_argv = state->place;
#ifdef GNU_COMPATIBLE
	switch (dash_prefix) {
	case D_PREFIX:
		current_dash = "-";
		break;
	case DD_PREFIX:
		current_dash = "--";
		break;
	case W_PREFIX:
		current_dash = "-W ";
		break;
	default:
		current_dash = "";
		break;
	}
#endif
	match = -1;
	exact_match = 0;
	second_partial_match = 0;

	state->optind++;

	has_equal = strchr(current_argv, '=');
	if (has_equal != NULL) {
		/* argument found (--option=arg) */
		current_argv_len = has_equal - current_argv;
		has_equal++;
	} else {
		current_argv_len = strlen(current_argv);
	}

	for (i = 0; long_options[i].name; i++) {
		/* find matching long option */
		if (strncmp(current_argv, long_options[i].name, current_argv_len)) {
			continue;
		}

		if (strlen(long_options[i].name) == current_argv_len) {
			/* exact match */
			match = i;
			exact_match = 1;
			break;
		}
		/*
		 * If this is a known short option, don't allow
		 * a partial match of a single character.
		 */
		if (short_too && current_argv_len == 1) {
			continue;
		}

		if (match == -1) { /* first partial match */
			match = i;
		} else if ((flags & FLAG_LONGONLY) ||
			   long_options[i].has_arg != long_options[match].has_arg ||
			   long_options[i].flag != long_options[match].flag ||
			   long_options[i].val != long_options[match].val) {
			second_partial_match = 1;
		}
	}
	if (!exact_match && second_partial_match) {
		/* ambiguous abbreviation */
		if (PRINT_ERROR) {
			LOG_WRN(AMBIG,
#ifdef GNU_COMPATIBLE
			     current_dash,
#endif
			     (int)current_argv_len,
			     current_argv);
		}
		state->optopt = 0;
		return BADCH;
	}
	if (match != -1) {		/* option found */
		if (long_options[match].has_arg == no_argument
		    && has_equal) {
			if (PRINT_ERROR) {
				LOG_WRN(NOARG,
#ifdef GNU_COMPATIBLE
				     current_dash,
#endif
				     (int)current_argv_len,
				     current_argv);
			}
			/*
			 * XXX: GNU sets optopt to val regardless of flag
			 */
			if (long_options[match].flag == NULL) {
				state->optopt = long_options[match].val;
			} else {
				state->optopt = 0;
			}
#ifdef GNU_COMPATIBLE
			return BADCH;
#else
			return BADARG;
#endif
		}
		if (long_options[match].has_arg == required_argument ||
		    long_options[match].has_arg == optional_argument) {
			if (has_equal) {
				state->optarg = has_equal;
			} else if (long_options[match].has_arg == required_argument) {
				/*
				 * optional argument doesn't use next nargv
				 */
				state->optarg = nargv[state->optind++];
			}
		}
		if ((long_options[match].has_arg == required_argument)
		    && (state->optarg == NULL)) {
			/*
			 * Missing argument; leading ':' indicates no error
			 * should be generated.
			 */
			if (PRINT_ERROR) {
				LOG_WRN(RECARGSTRING,
#ifdef GNU_COMPATIBLE
				    current_dash,
#endif
				    current_argv);
			}
			/*
			 * XXX: GNU sets optopt to val regardless of flag
			 */
			if (long_options[match].flag == NULL) {
				state->optopt = long_options[match].val;
			} else {
				state->optopt = 0;
			}
			--state->optind;
			return BADARG;
		}
	} else {			/* unknown option */
		if (short_too) {
			--state->optind;
			return -1;
		}
		if (PRINT_ERROR) {
			LOG_WRN(ILLOPTSTRING,
#ifdef GNU_COMPATIBLE
			      current_dash,
#endif
			      current_argv);
		}
		state->optopt = 0;
		return BADCH;
	}
	if (idx) {
		*idx = match;
	}
	if (long_options[match].flag) {
		*long_options[match].flag = long_options[match].val;
		return 0;
	} else {
		return long_options[match].val;
	}
}

/*
 * getopt_internal --
 *	Parse argc/argv argument vector.  Called by user level routines.
 */
static int
getopt_internal(struct getopt_state *state, int nargc, char * const *nargv,
		const char *options, const struct option *long_options,
		int *idx, int flags)
{
	char *oli;				/* option letter list index */
	int optchar, short_too;

	if (options == NULL) {
		return -1;
	}

	/*
	 * Disable GNU extensions if options string begins with a '+'.
	 */
#ifdef GNU_COMPATIBLE
	if (*options == '-') {
		flags |= FLAG_ALLARGS;
	} else if (*options == '+') {
		flags &= ~FLAG_PERMUTE;
	}
#else
	if (*options == '+')
		flags &= ~FLAG_PERMUTE;
	else if (*options == '-')
		flags |= FLAG_ALLARGS;
#endif
	if (*options == '+' || *options == '-') {
		options++;
	}

	/*
	 * XXX Some GNU programs (like cvs) set optind to 0 instead of
	 * XXX using optreset.  Work around this braindamage.
	 */
	if (state->optind == 0) {
		state->optind = state->optreset = 1;
	}

	state->optarg = NULL;
	if (state->optreset) {
		state->nonopt_start = state->nonopt_end = -1;
	}
start:
	if (state->optreset || !*(state->place)) {/* update scanning pointer */
		state->optreset = 0;
		if (state->optind >= nargc) {          /* end of argument vector */
			state->place = EMSG;
			if (state->nonopt_end != -1) {
				/* do permutation, if we have to */
				permute_args(state->nonopt_start,
					     state->nonopt_end,
					     state->optind, nargv);
				state->optind -= state->nonopt_end -
						state->nonopt_start;
			} else if (state->nonopt_start != -1) {
				/*
				 * If we skipped non-options, set optind
				 * to the first of them.
				 */
				state->optind = state->nonopt_start;
			}
			state->nonopt_start = state->nonopt_end = -1;
			return -1;
		}
		state->place = nargv[state->optind];
		if (*(state->place) != '-' ||
#ifdef GNU_COMPATIBLE
		    state->place[1] == '\0') {
#else
		    (state->place[1] == '\0' && strchr(options, '-') == NULL)) {
#endif
			state->place = EMSG;		/* found non-option */
			if (flags & FLAG_ALLARGS) {
				/*
				 * GNU extension:
				 * return non-option as argument to option 1
				 */
				state->optarg = nargv[state->optind++];
				return INORDER;
			}
			if (!(flags & FLAG_PERMUTE)) {
				/*
				 * If no permutation wanted, stop parsing
				 * at first non-option.
				 */
				return -1;
			}
			/* do permutation */
			if (state->nonopt_start == -1) {
				state->nonopt_start = state->optind;
			} else if (state->nonopt_end != -1) {
				permute_args(state->nonopt_start,
					     state->nonopt_end,
					     state->optind,
					     nargv);
				state->nonopt_start = state->optind -
				    (state->nonopt_end - state->nonopt_start);
				state->nonopt_end = -1;
			}
			state->optind++;
			/* process next argument */
			goto start;
		}
		if (state->nonopt_start != -1 && state->nonopt_end == -1) {
			state->nonopt_end = state->optind;
		}

		/*
		 * If we have "-" do nothing, if "--" we are done.
		 */
		if (state->place[1] != '\0' && *++(state->place) == '-' &&
		    state->place[1] == '\0') {
			state->optind++;
			state->place = EMSG;
			/*
			 * We found an option (--), so if we skipped
			 * non-options, we have to permute.
			 */
			if (state->nonopt_end != -1) {
				permute_args(state->nonopt_start,
					     state->nonopt_end,
					     state->optind,
					     nargv);
				state->optind -= state->nonopt_end -
						 state->nonopt_start;
			}
			state->nonopt_start = state->nonopt_end = -1;
			return -1;
		}
	}

	/*
	 * Check long options if:
	 *  1) we were passed some
	 *  2) the arg is not just "-"
	 *  3) either the arg starts with -- we are getopt_long_only()
	 */
	if (long_options != NULL && state->place != nargv[state->optind] &&
	    (*(state->place) == '-' || (flags & FLAG_LONGONLY))) {
		short_too = 0;
#ifdef GNU_COMPATIBLE
		dash_prefix = D_PREFIX;
#endif
		if (*(state->place) == '-') {
			state->place++;		/* --foo long option */
#ifdef GNU_COMPATIBLE
			dash_prefix = DD_PREFIX;
#endif
		} else if (*(state->place) != ':' && strchr(options, *(state->place)) != NULL) {
			short_too = 1;		/* could be short option too */
		}

		optchar = parse_long_options(state, nargv, options,
					     long_options, idx, short_too,
					     flags);
		if (optchar != -1) {
			state->place = EMSG;
			return optchar;
		}
	}
	optchar = (int)*(state->place)++;
	oli = strchr(options, optchar);
	if (optchar == (int)':' ||
	    (optchar == (int)'-' && *(state->place) != '\0') ||
	    oli == NULL) {
		/*
		 * If the user specified "-" and  '-' isn't listed in
		 * options, return -1 (non-option) as per POSIX.
		 * Otherwise, it is an unknown option character (or ':').
		 */
		if (optchar == (int)'-' && *(state->place) == '\0') {
			return -1;
		}
		if (!*(state->place)) {
			++state->optind;
		}
#ifdef GNU_COMPATIBLE
		if (PRINT_ERROR) {
			LOG_WRN(GNUOPTCHAR, optchar);
		}
#else
		if (PRINT_ERROR) {
			LOG_WRN(ILLOPTCHAR, optchar);
		}
#endif
		state->optopt = optchar;
		return BADCH;
	}
	if (long_options != NULL && optchar == 'W' && oli[1] == ';') {
		/* -W long-option */
		if (*(state->place)) {		/* no space */
			; /* NOTHING */
		} else if (++(state->optind) >= nargc) {	/* no arg */
			state->place = EMSG;
			if (PRINT_ERROR) {
				LOG_WRN(RECARGCHAR, optchar);
			}
			state->optopt = optchar;
			return BADARG;
		} else if ((state->optind) < nargc) {
			state->place = nargv[state->optind];
		}
#ifdef GNU_COMPATIBLE
		dash_prefix = W_PREFIX;
#endif
		optchar = parse_long_options(state, nargv, options,
					     long_options, idx, 0, flags);
		state->place = EMSG;
		return optchar;
	}
	if (*++oli != ':') {			/* doesn't take argument */
		if (!*(state->place)) {
			++state->optind;
		}
	} else {				/* takes (optional) argument */
		state->optarg = NULL;
		if (*(state->place)) {			/* no white space */
			state->optarg = state->place;
		} else if (oli[1] != ':') {	/* arg not optional */
			if (++state->optind >= nargc) {	/* no arg */
				state->place = EMSG;
				if (PRINT_ERROR) {
					LOG_WRN(RECARGCHAR, optchar);
				}
				state->optopt = optchar;
				return BADARG;
			}
			state->optarg = nargv[state->optind];
		}
		state->place = EMSG;
		++state->optind;
	}
	/* dump back option letter */
	return optchar;
}

/*
 * getopt_long --
 *	Parse argc/argv argument vector.
 */
int
getopt_long(int nargc, char *const *nargv,
	    const char *options, const struct option *long_options,
	    int *idx)
{
	struct getopt_state *state;
	int ret;

	/* Get state of the current thread */
	state = getopt_state_get();

	ret = getopt_internal(state, nargc, nargv, options, long_options, idx,
			      FLAG_PERMUTE);

	z_getopt_global_state_update(state);

	return ret;
}

/*
 * getopt_long_only --
 *	Parse argc/argv argument vector.
 */
int
getopt_long_only(int nargc, char *const *nargv,
		 const char *options, const struct option *long_options,
		 int *idx)
{
	struct getopt_state *state;
	int ret;

	/* Get state of the current thread */
	state = getopt_state_get();

	ret = getopt_internal(state, nargc, nargv, options, long_options, idx,
			      FLAG_PERMUTE|FLAG_LONGONLY);

	z_getopt_global_state_update(state);

	return ret;
}
