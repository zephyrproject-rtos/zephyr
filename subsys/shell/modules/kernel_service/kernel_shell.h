/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_SHELL_MODULES_KERNEL_SERVICE_KERNEL_SHELL_H_
#define ZEPHYR_SUBSYS_SHELL_MODULES_KERNEL_SERVICE_KERNEL_SHELL_H_

#include <zephyr/shell/shell.h>

/* Add command to the set of kernel subcommands, see `SHELL_SUBCMD_ADD` */
#define KERNEL_CMD_ARG_ADD(_syntax, _subcmd, _help, _handler, _mand, _opt)                         \
	SHELL_SUBCMD_ADD((kernel), _syntax, _subcmd, _help, _handler, _mand, _opt);

#define KERNEL_CMD_ADD(_syntax, _subcmd, _help, _handler)                                          \
	KERNEL_CMD_ARG_ADD(_syntax, _subcmd, _help, _handler, 0, 0);

/* Add command to the set of `kernel thread` subcommands */
#define KERNEL_THREAD_CMD_ARG_ADD(_syntax, _subcmd, _help, _handler, _mand, _opt)                  \
	SHELL_SUBCMD_ADD((thread), _syntax, _subcmd, _help, _handler, _mand, _opt);

#define KERNEL_THREAD_CMD_ADD(_syntax, _subcmd, _help, _handler)                                   \
	KERNEL_THREAD_CMD_ARG_ADD(_syntax, _subcmd, _help, _handler, 0, 0);

/* Internal function to check if a thread pointer is valid */
bool z_thread_is_valid(const struct k_thread *thread);

#endif /* ZEPHYR_SUBSYS_SHELL_MODULES_KERNEL_SERVICE_KERNEL_SHELL_H_ */
