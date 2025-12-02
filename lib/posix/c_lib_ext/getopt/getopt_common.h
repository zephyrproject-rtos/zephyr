/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GETOPT_COMMON_H__
#define _GETOPT_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/* This function is not thread safe. All threads using getopt are calling
 * this function.
 */
void z_getopt_global_state_update(struct getopt_state *state);

#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_COMMON_H__ */
