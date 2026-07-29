/*
 * Copyright (c) 2026 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/sys/util.h>

#define ATCM_NODE		DT_NODELABEL(atcm)
#define GIC_NODE		DT_NODELABEL(gic)
#define SOC_NODE		DT_NODELABEL(soc)
#define PERIPH_NODE		DT_NODELABEL(peripherals)

#define ATCM_REGION_START	DT_REG_ADDR(ATCM_NODE)
#define ATCM_REGION_END		(DT_REG_ADDR(ATCM_NODE) + DT_REG_SIZE(ATCM_NODE))

#define GIC_REGION_START	DT_REG_ADDR_BY_IDX(GIC_NODE, 0)
#define GIC_REGION_END		(DT_REG_ADDR_BY_IDX(GIC_NODE, 1) + DT_REG_SIZE_BY_IDX(GIC_NODE, 1))

#define DEVICE_REGION_START	DT_REG_ADDR(PERIPH_NODE)
#define DEVICE_REGION_END	(DT_REG_ADDR(PERIPH_NODE) + DT_REG_SIZE(PERIPH_NODE))

static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("ATCM",
			 ATCM_REGION_START,
			 REGION_RAM_ATTR(ATCM_REGION_END)),

	MPU_REGION_ENTRY("SRAM_TEXT",
			 (uintptr_t)__rom_region_start,
			 REGION_RAM_TEXT_ATTR((uintptr_t)__rodata_region_start)),

	MPU_REGION_ENTRY("SRAM_RODATA",
			 (uintptr_t)__rodata_region_start,
			 REGION_RAM_RO_ATTR((uintptr_t)__rodata_region_end)),

	MPU_REGION_ENTRY("SRAM_DATA",
#if defined(CONFIG_USERSPACE)
			(uintptr_t)_app_smem_start,
#else
			(uintptr_t)__kernel_ram_start,
#endif
			 REGION_RAM_ATTR((uintptr_t)__kernel_ram_end)),

	MPU_REGION_ENTRY("DEVICE",
			 DEVICE_REGION_START,
			 REGION_DEVICE_ATTR(DEVICE_REGION_END)),

	MPU_REGION_ENTRY("GIC",
			 GIC_REGION_START,
			 REGION_DEVICE_ATTR(GIC_REGION_END)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
