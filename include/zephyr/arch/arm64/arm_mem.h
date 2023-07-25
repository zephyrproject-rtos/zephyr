/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MEM_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MEM_H_

/*
 * Define ARM specific memory flags used by z_phys_map()
 * followed public definitions in include/sys/mem_manage.h.
 */
/* For ARM64, K_MEM_CACHE_NONE is nGnRnE. */
#define K_MEM_ARM_DEVICE_nGnRnE	K_MEM_CACHE_NONE

/** ARM64 Specific flags: device memory with nGnRE */
#define K_MEM_ARM_DEVICE_nGnRE	3

/** ARM64 Specific flags: device memory with GRE */
#define K_MEM_ARM_DEVICE_GRE	4

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ARM_MEM_H_ */
