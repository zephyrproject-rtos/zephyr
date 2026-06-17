/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

/*
 * Static MPU regions are scoped to the core that actually owns them.
 *
 * The PSE84 memory map exposes several core-specific code and shared windows
 * (secure/non-secure CM33 code, the per-core allocatable shared windows, and
 * the CM55 TCMs). Instantiating every window on both cores wastes scarce HW
 * MPU regions: the CM33 only has 8 data regions, so unconditionally creating 6
 * static regions left no room for memory-domain partitions and broke the
 * userspace / memory-protection tests with -ENOSPC at init (the default domain
 * could not allocate its libc/malloc partitions).
 *
 * Gating each window on its owning core keeps the CM33 static set down to
 * {FLASH, SRAM, SHARED_MEMORY_33, M33S_CODE}, recovering partition slots, while
 * the CM55 keeps only the windows it actually uses.
 *
 * We are expected to give *CONFIG_SIZE* in KB, but REGION_ATTR
 * expects bytes, so we multiply by 1024 to convert.
 */
static const struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY(
		"FLASH",
		CONFIG_FLASH_BASE_ADDRESS,
		REGION_FLASH_ATTR(
			CONFIG_FLASH_BASE_ADDRESS,
			CONFIG_FLASH_SIZE * 1024)),

	MPU_REGION_ENTRY(
		"SRAM",
		 DT_CHOSEN_SRAM_ADDR,
		REGION_RAM_ATTR(
			DT_CHOSEN_SRAM_ADDR,
			DT_CHOSEN_SRAM_SIZE)),

#if DT_NODE_EXISTS(DT_NODELABEL(m33_allocatable_shared))
	MPU_REGION_ENTRY(
		"SHARED_MEMORY_33",
		DT_REG_ADDR(DT_NODELABEL(m33_allocatable_shared)),
		REGION_RAM_NOCACHE_ATTR(
			DT_REG_ADDR(DT_NODELABEL(m33_allocatable_shared)),
			DT_REG_SIZE(DT_NODELABEL(m33_allocatable_shared)))),
#endif

	/* CM55-only nocache window: unused on the CM33, where it only wastes a
	 * HW MPU region.
	 */
#if DT_NODE_EXISTS(DT_NODELABEL(m55_allocatable_shared)) && defined(CONFIG_CPU_CORTEX_M55)
	MPU_REGION_ENTRY(
		"SHARED_MEMORY 55",
		DT_REG_ADDR(DT_NODELABEL(m55_allocatable_shared)),
		REGION_RAM_NOCACHE_ATTR(
			DT_REG_ADDR(DT_NODELABEL(m55_allocatable_shared)),
			DT_REG_SIZE(DT_NODELABEL(m55_allocatable_shared)))),
#endif

	/* Secure CM33 code-in-SRAM window: only meaningful for the secure CM33
	 * image. The m33s_code node is also visible to a non-secure CM33 build, so
	 * gate it on TRUSTED_EXECUTION_SECURE (not just !CM55) to keep it off the
	 * non-secure CM33 and the CM55.
	 */
#if DT_NODE_EXISTS(DT_NODELABEL(m33s_code)) && \
	defined(CONFIG_CPU_CORTEX_M33) && defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	MPU_REGION_ENTRY(
		"M33S_CODE",
		DT_REG_ADDR(DT_NODELABEL(m33s_code)),
		REGION_RAM_ATTR_WITH_EXEC(
			DT_REG_ADDR(DT_NODELABEL(m33s_code)),
			DT_REG_SIZE(DT_NODELABEL(m33s_code)))),
#endif

	/* Non-secure CM33 code-in-SRAM window: only meaningful for a non-secure
	 * CM33 image. It is empty on the secure CM33 build and unused on the
	 * CM55, so do not spend a HW MPU region on it there.
	 */
#if DT_NODE_EXISTS(DT_NODELABEL(m33_code)) && \
	defined(CONFIG_CPU_CORTEX_M33) && !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	MPU_REGION_ENTRY(
		"M33_CODE",
		DT_REG_ADDR(DT_NODELABEL(m33_code)),
		REGION_RAM_ATTR_WITH_EXEC(
			DT_REG_ADDR(DT_NODELABEL(m33_code)),
			DT_REG_SIZE(DT_NODELABEL(m33_code)))),
#endif

	/* CM55-only tightly-coupled memories: the itcm/dtcm node labels exist only
	 * in the CM55 devicetree (the CM33 uses separate itcm_m33s/dtcm_m33s nodes),
	 * so DT_NODE_EXISTS already scopes these to the CM55 build.
	 */
#if DT_NODE_EXISTS(DT_NODELABEL(itcm))
	MPU_REGION_ENTRY(
		"ITCM",
		DT_REG_ADDR(DT_NODELABEL(itcm)),
		REGION_RAM_ATTR_WITH_EXEC(
			DT_REG_ADDR(DT_NODELABEL(itcm)),
			DT_REG_SIZE(DT_NODELABEL(itcm)))),
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(dtcm))
	MPU_REGION_ENTRY(
		"DTCM",
		DT_REG_ADDR(DT_NODELABEL(dtcm)),
		REGION_RAM_ATTR(DT_REG_ADDR(
			DT_NODELABEL(dtcm)),
			DT_REG_SIZE(DT_NODELABEL(dtcm)))),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
