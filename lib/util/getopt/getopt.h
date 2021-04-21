/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GETOPT_H__
#define _GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>

struct getopt_state {
	int opterr;	/* if error message should be printed */
	int optind;	/* index into parent argv vector */
	int optopt;	/* character checked for validity */
	int optreset;	/* reset getopt */
	char *optarg;	/* argument associated with option */

	char *place;	/* option letter processing */
};

/* Function intializes getopt_state structure */
void getopt_init(struct getopt_state *state);

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int getopt(struct getopt_state *const state, int nargc,
	   char *const nargv[], const char *ostr);


#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_H__ */
