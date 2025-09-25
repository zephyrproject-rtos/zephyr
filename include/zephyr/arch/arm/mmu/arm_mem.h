/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_ARM_MEM_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_ARM_MEM_H_

/*
 * Define ARM specific memory flags used by k_mem_map_phys_bare()
 * followed public definitions in include/kernel/mm.h.
 */

/** ARM Specific flags: normal memory with Non-cacheable */
#define K_MEM_ARM_NORMAL_NC	5

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_ARM_MEM_H_ */
