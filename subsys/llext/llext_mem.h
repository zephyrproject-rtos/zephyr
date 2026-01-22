/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_LLEXT_MEM_H_
#define ZEPHYR_SUBSYS_LLEXT_MEM_H_

#ifdef CONFIG_MMU_PAGE_SIZE
#define LLEXT_PAGE_SIZE CONFIG_MMU_PAGE_SIZE
#elif CONFIG_ARC_MPU_VER == 2
#define LLEXT_PAGE_SIZE 2048
#else
/* Arm and non-v2 ARC MPUs want a 32 byte minimum MPU region */
#define LLEXT_PAGE_SIZE 32
#endif

#endif /* ZEPHYR_SUBSYS_LLEXT_MEM_H_ */
