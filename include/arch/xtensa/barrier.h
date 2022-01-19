/*
 * Copyright (c) 2022 Alexandre Mergnat <amergnat@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_BARRIER_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_BARRIER_H_

#ifndef _ASMLANGUAGE

#if defined(CONFIG_ARCH_HAS_MEMORY_BARRIER)
#error The XTENSA architecture does not have a memory barrier implementation
#endif /* CONFIG_ARCH_HAS_MEMORY_BARRIER */

#include <sys/barrier.h>

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_BARRIER_H_ */
