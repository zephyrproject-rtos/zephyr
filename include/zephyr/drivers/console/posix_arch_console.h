/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_POSIX_ARCH_CONSOLE_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_POSIX_ARCH_CONSOLE_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void posix_flush_stdout(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_POSIX_ARCH_CONSOLE_H_ */
