/*
 * Copyright (c) 2021 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_ADDR_TYPES_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_ADDR_TYPES_H_
#ifndef _ASMLANGUAGE

/*
 * MWDT provides paddr_t type and it conflicts with Zephyr definition:
 * - Zephyr defines paddr_t as a uintptr_t
 * - MWDT defines paddr_t as a unsigned long
 * This causes extra warnings. However we can safely define
 * paddr_t as a unsigned long for the case when MWDT toolchain is used as
 * they are both unsigned, have same size and aligning.
 */
#ifdef __CCAC__
typedef unsigned long paddr_t;
typedef void *vaddr_t;
#else
#include <zephyr/arch/common/addr_types.h>
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARC_ADDR_TYPES_H_ */
