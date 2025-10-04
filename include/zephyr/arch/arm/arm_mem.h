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

/* @name optional region type attributes when K_MEM_CACHE_NONE is set
 *
 * Default is non cacheable device type
 *
 * @{
 */

/** Non-cacheable region will get device attribute allowing non aligned access */
#define K_MEM_ARM_DEVICE_NC		0
/** Non-cacheable region will get normal attribute allowing non aligned access */
#define K_MEM_ARM_NORMAL_NC		BIT(30)
/** Non-cacheable region will get strongly ordered attribute */
#define K_MEM_ARM_STRONGLY_ORDERED_NC	BIT(31)
/** Reserved bits for cache modes in k_map() flags argument */
#define K_MEM_ARM_NC_TYPE_MASK		(BIT(31) | BIT(30))
/** @} */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MEM_H_ */
