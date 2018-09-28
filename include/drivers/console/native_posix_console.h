/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NATIVE_POSIX_STDOUT_CONSOLE)
void posix_flush_stdout(void);
#endif

#if defined(CONFIG_NATIVE_POSIX_STDIN_CONSOLE)
void native_stdin_register_input(struct k_fifo *avail, struct k_fifo *lines,
			 u8_t (*completion)(char *str, u8_t len));
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_ */
