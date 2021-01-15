/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_GETOPT_H__
#define SHELL_GETOPT_H__

#include <getopt.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Initializing shell getopt module.
 *
 * @param[in] shell	Pointer to the shell instance.
 */
void z_shell_getopt_init(struct getopt_state *state);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_GETOPT_H__ */
