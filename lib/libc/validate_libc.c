/*
 * Copyright © 2025 Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

/* Validate various C library implementation characteristics */

#ifndef CONFIG_LIBC_ALLOW_LESS_THAN_64BIT_TIME
#include <time.h>
/*
 * Ensure that time_t can hold at least 64 bit values.
 */
BUILD_ASSERT(sizeof(time_t) >= 8, "time_t cannot hold 64-bit values");
#endif
