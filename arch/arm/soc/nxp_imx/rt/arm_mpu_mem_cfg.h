/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _IMX_RT_MPU_MEM_CFG_H_
#define _IMX_RT_MPU_MEM_CFG_H_

#include <soc.h>
#include <arch/arm/cortex_m/mpu/arm_mpu.h>

/* Flash Region Definitions */
#if CONFIG_FLASH_SIZE == 64
#define REGION_FLASH_SIZE REGION_64K
#elif CONFIG_FLASH_SIZE == 128
#define REGION_FLASH_SIZE REGION_128K
#elif CONFIG_FLASH_SIZE == 256
#define REGION_FLASH_SIZE REGION_256K
#elif CONFIG_FLASH_SIZE == 512
#define REGION_FLASH_SIZE REGION_512K
#elif CONFIG_FLASH_SIZE == 1024
#define REGION_FLASH_SIZE REGION_1M
#elif CONFIG_FLASH_SIZE == 2048
#define REGION_FLASH_SIZE REGION_2M
#elif CONFIG_FLASH_SIZE == 8192
#define REGION_FLASH_SIZE REGION_8M
#elif CONFIG_FLASH_SIZE == 65536
#define REGION_FLASH_SIZE REGION_64M
#else
#error "Unsupported configuration"
#endif

/* SRAM Region Definitions */
#if CONFIG_SRAM_SIZE == 32
#define REGION_SRAM_0_SIZE REGION_32K
#elif CONFIG_SRAM_SIZE == 64
#define REGION_SRAM_0_SIZE REGION_64K
#elif CONFIG_SRAM_SIZE == 128
#define REGION_SRAM_0_SIZE REGION_128K
#elif CONFIG_SRAM_SIZE == 256
#define REGION_SRAM_0_SIZE REGION_256K
#elif CONFIG_SRAM_SIZE == 32768
#define REGION_SRAM_0_SIZE REGION_32M
#else
#error "Unsupported configuration"
#endif

#endif /* _IMX_RT_MPU_MEM_CFG_H_ */
