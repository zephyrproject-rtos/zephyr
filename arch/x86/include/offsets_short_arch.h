/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_X86_INCLUDE_OFFSETS_SHORT_ARCH_H_

#ifdef CONFIG_X86_LONGMODE
#include <intel64/offsets_short_arch.h>
#else
#include <ia32/offsets_short_arch.h>
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
