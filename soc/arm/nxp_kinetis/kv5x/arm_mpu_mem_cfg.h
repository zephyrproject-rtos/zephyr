/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ARM_MPU_MEM_CFG_H_
#define _ARM_MPU_MEM_CFG_H_

#include <soc.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

/* Flash Region Definitions */
#if CONFIG_FLASH_SIZE == 512
#define REGION_FLASH_SIZE REGION_512K
#elif CONFIG_FLASH_SIZE == 1024
#define REGION_FLASH_SIZE REGION_1M
#else
#error "Unsupported configuration"
#endif

/* SRAM Region Definitions */
#if CONFIG_SRAM_SIZE == 64
#define REGION_SRAM_0_SIZE REGION_64K
#elif CONFIG_SRAM_SIZE == 128
#define REGION_SRAM_0_SIZE REGION_128K
#else
#error "Unsupported configuration"
#endif

#endif /* _ARM_MPU_MEM_CFG_H_ */
