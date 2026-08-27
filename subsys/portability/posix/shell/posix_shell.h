/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_SHELL_POSIX_SHELL_H_
#define ZEPHYR_LIB_POSIX_SHELL_POSIX_SHELL_H_

#include <zephyr/shell/shell.h>

/* Add command to the set of POSIX subcommands, see `SHELL_SUBCMD_ADD` */
#define POSIX_CMD_ADD(_syntax, _subcmd, _help, _handler, _mand, _opt)                              \
	SHELL_SUBCMD_ADD((posix), _syntax, _subcmd, _help, _handler, _mand, _opt);

#endif /* ZEPHYR_LIB_POSIX_SHELL_POSIX_SHELL_H_ */
