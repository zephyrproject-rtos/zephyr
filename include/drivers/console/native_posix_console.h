/*
 * Copyright (c) 2018 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NATIVE_POSIX_CONSOLE_H
#define _NATIVE_POSIX_CONSOLE_H

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_NATIVE_POSIX_STDOUT_CONSOLE)
void posix_flush_stdout(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_POSIX_CONSOLE_H */
