/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GETOPT_COMMON_H__
#define _GETOPT_COMMON_H__

#define __need_getopt_newlib
#include "getopt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These functions are not thread safe. All threads using getopt are calling
 * them.
 */
/* Put state into POSIX global variables */
void z_getopt_global_state_put(struct getopt_data *state);

/* Get state from POSIX global variables */
void z_getopt_global_state_get(struct getopt_data *state);

#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_COMMON_H__ */
