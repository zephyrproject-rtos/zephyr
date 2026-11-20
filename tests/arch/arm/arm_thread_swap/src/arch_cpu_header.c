/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Build check: <zephyr/arch/cpu.h> must compile on its own, before any
 * kernel header.
 */
#include <zephyr/arch/cpu.h>

BUILD_ASSERT(sizeof(struct arch_esf) > 0U);
