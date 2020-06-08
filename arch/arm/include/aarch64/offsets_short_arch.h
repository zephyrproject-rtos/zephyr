/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_OFFSETS_SHORT_ARCH_H_ */
