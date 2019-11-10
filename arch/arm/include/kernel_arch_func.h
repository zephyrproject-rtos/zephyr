/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_

#if defined(CONFIG_ARM64)
#include <aarch64/kernel_arch_func.h>
#else
#include <aarch32/kernel_arch_func.h>
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_ */
