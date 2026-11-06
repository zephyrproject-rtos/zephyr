/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LLEXT_MEM_H_
#define ZEPHYR_SUBSYS_LLEXT_MEM_H_

#ifdef CONFIG_MMU
#define LLEXT_PAGE_SIZE CONFIG_MMU_PAGE_SIZE
#elif defined(CONFIG_ARM_MPU)
#define LLEXT_PAGE_SIZE CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
#elif CONFIG_ARC_MPU_VER == 2
#define LLEXT_PAGE_SIZE 2048
#elif defined(CONFIG_XTENSA_MPU)
#include <xtensa/config/core-isa.h>

#define LLEXT_PAGE_SIZE XCHAL_MPU_ALIGN
#else
/* Default minimum region size. */
#define LLEXT_PAGE_SIZE 32
#endif

#endif /* ZEPHYR_SUBSYS_LLEXT_MEM_H_ */
