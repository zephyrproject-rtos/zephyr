/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_

#ifndef _ASMLANGUAGE

#include <arch/arm64/lib_helpers.h>

#define z_full_mb()	__dsb(sy)
#define z_read_mb()	__dsb(ld)
#define z_write_mb()	__dsb(st)

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_BARRIER_H_ */
