/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_IAR_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_IAR_H_

#ifdef TOOLCHAIN_PRAGMA
#define _TOOLCHAIN_DISABLE_WARNING(warning) TOOLCHAIN_PRAGMA(diag_suppress = warning)
#define _TOOLCHAIN_ENABLE_WARNING(warning) TOOLCHAIN_PRAGMA(diag_default = warning)

#define TOOLCHAIN_DISABLE_WARNING(warning) _TOOLCHAIN_DISABLE_WARNING(warning)
#define TOOLCHAIN_ENABLE_WARNING(warning)  _TOOLCHAIN_ENABLE_WARNING(warning)
#endif

#ifdef __ICCARM__
#include "iar/iccarm.h"
#endif
#ifdef __ICCRISCV__
#include "iar/iccriscv.h"
#endif

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_H_ */
