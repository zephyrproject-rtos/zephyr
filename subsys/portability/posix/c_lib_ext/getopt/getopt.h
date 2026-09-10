/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GETOPT_H_
#define _GETOPT_H_

#include <zephyr/sys/sys_getopt.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void getopt_init(void);

#define getopt_state sys_getopt_state
#define option sys_getopt_option

#define no_argument       0
#define required_argument 1
#define optional_argument 2

extern struct sys_getopt_state *getopt_state_get(void);

extern int getopt_long(int nargc, char *const *nargv, const char *options,
		       const struct option *long_options, int *idx);

extern int getopt_long_only(int nargc, char *const *nargv, const char *options,
			    const struct option *long_options, int *idx);

#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_H_ */
