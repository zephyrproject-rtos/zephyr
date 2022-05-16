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

void posix_flush_stdout(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CONSOLE_NATIVE_POSIX_CONSOLE_H_ */
