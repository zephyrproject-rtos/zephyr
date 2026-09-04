/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_OS_SYS_PROCESS_PRIV_H_
#define ZEPHYR_LIB_OS_SYS_PROCESS_PRIV_H_

#include <zephyr/kernel.h>

#ifdef CONFIG_USERSPACE
/* kernel copies of a user-mode caller's argv/envp, bounded by CONFIG_SYS_PROCESS_ARG_BYTES */
int z_clone_user_vectors(char *const **argv, char *const **envp);
void z_clone_vec_free(char **vec);
#endif /* CONFIG_USERSPACE */

#endif /* ZEPHYR_LIB_OS_SYS_PROCESS_PRIV_H_ */
