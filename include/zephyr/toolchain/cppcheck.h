/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_CPPCHECK_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_CPPCHECK_H_

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

#define __CHAR_BIT__ 8
#define __SIZEOF_LONG__ 32
#define __SIZEOF_LONG_LONG__ 64
#define __LITTLE_ENDIAN__

#include <zephyr/toolchain/gcc.h>
