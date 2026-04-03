/*
 * Copyright (c) 2025, Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

extern const uint32_t __rom_region_start;
extern const uint32_t __rom_region_mpu_size_bits;

/*
 * MPU configuration for AMD Versal RPU (Cortex-R5F)
 *
 * Memory Map:
 * - 0x00000000 - 0x7FFFFFFF: DDR/TCM (dynamically sized based on DT)
 * - 0x80000000 - 0xBFFFFFFF: PL (1GB, strongly ordered)
 * - 0xC0000000 - 0xDFFFFFFF: QSPI (512MB, device)
 * - 0xE0000000 - 0xEFFFFFFF: PCIe Low (256MB, device)
 * - 0xF0000000 - 0xF7FFFFFF: PMC (128MB, device)
 * - 0xF8000000 - 0xF8FFFFFF: STM_CORESIGHT (16MB, device)
 * - 0xF9000000 - 0xF90FFFFF: RPU_A53_GIC (1MB, device)
 * - 0xFD000000 - 0xFDFFFFFF: FPS slaves (16MB, device)
 * - 0xFE000000 - 0xFEFFFFFF: Upper LPS slaves (16MB, device)
 * - 0xFF000000 - 0xFFFFFFFF: Lower LPS slaves, TCM, OCM (16MB, device)
 * - 0xFFFC0000 - 0xFFFFFFFF: OCM (256KB, cacheable - overlay)
 */

/*
 * Calculate MPU region size based on memory size
 * MPU regions must be power-of-2 sized and naturally aligned
 * This matches the Xilinx BSP approach for DDR configuration
 *
 * IMPORTANT: For non-contiguous memory (e.g., shared memory with Linux),
 * define separate memory regions in device tree and they will be mapped
 * as separate MPU regions below.
 */
#if (CONFIG_SRAM_BASE_ADDRESS != 0)
/* Calculate the region size needed to cover from 0x0 to end of SRAM
 * For DDR at 0x40000, we need to cover from 0x0 (TCM) to end of DDR
 * Region must start at 0x0 to include TCM for reset vectors
 *
 * NOTE: This assumes contiguous memory from 0x0 to DDR end.
 * For non-contiguous memory layouts (e.g., Linux at 0x2000000-0x30000000,
 * Zephyr at 0x4000000-0x60000000), you MUST define separate regions
 * in device tree to avoid mapping inaccessible or protected memory.
 */
#define DDR_END_ADDRESS (CONFIG_SRAM_BASE_ADDRESS + (CONFIG_SRAM_SIZE * 1024))

/* Check if memory layout is contiguous from 0x0
 * If SRAM_BASE is far from 0 (e.g., > 64MB), it's likely a shared memory
 * design where TCM and DDR are separate regions
 */
#if (CONFIG_SRAM_BASE_ADDRESS > 0x4000000)
#warning "SRAM base address is > 64MB - ensure TCM is defined separately in " \
	 "device tree for vector relocation"
#endif

/* Determine appropriate region size to cover entire range
 * Following Xilinx BSP logic for dynamic sizing
 *
 * Example configurations:
 * - Isolation design (32MB per R5):  SRAM=32MB  → Region=64M
 * - Isolation design (256MB per R5): SRAM=256MB → Region=512M
 * - Shared memory (512MB):           SRAM=512MB → Region=1G
 * - Full DDR (1GB):                  SRAM=1GB   → Region=2G
 * - Full DDR (2GB):                  SRAM=2GB   → Region=2G
 */
#if (DDR_END_ADDRESS <= 0x80000)         /* 512KB */
#define MEMORY_REGION_SIZE REGION_512K
#elif (DDR_END_ADDRESS <= 0x100000)      /* 1MB */
#define MEMORY_REGION_SIZE REGION_1M
#elif (DDR_END_ADDRESS <= 0x1000000)     /* 16MB */
#define MEMORY_REGION_SIZE REGION_16M
#elif (DDR_END_ADDRESS <= 0x4000000)     /* 64MB */
#define MEMORY_REGION_SIZE REGION_64M
#elif (DDR_END_ADDRESS <= 0x10000000)    /* 256MB */
#define MEMORY_REGION_SIZE REGION_256M
#elif (DDR_END_ADDRESS <= 0x20000000)    /* 512MB */
#define MEMORY_REGION_SIZE REGION_512M
#elif (DDR_END_ADDRESS <= 0x40000000)    /* 1GB */
#define MEMORY_REGION_SIZE REGION_1G
#elif (DDR_END_ADDRESS <= 0x80000000)    /* 2GB */
#define MEMORY_REGION_SIZE REGION_2G
#else
#warning "DDR size exceeds 2GB - limiting MPU region to 2GB."
#define MEMORY_REGION_SIZE REGION_2G
#endif

/* Helper to calculate power-of-2 region size for a given memory size */
#define CALC_REGION_SIZE(size) ( \
	(size) <= 0x80000 ? REGION_512K : \
	(size) <= 0x100000 ? REGION_1M : \
	(size) <= 0x1000000 ? REGION_16M : \
	(size) <= 0x4000000 ? REGION_64M : \
	(size) <= 0x10000000 ? REGION_256M : \
	(size) <= 0x20000000 ? REGION_512M : \
	(size) <= 0x40000000 ? REGION_1G : \
	REGION_2G)

#endif

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0: SRAM mapping (TCM/DDR) - data R/W + XN */
#if (CONFIG_SRAM_BASE_ADDRESS == 0)
	/* Using TCM only - size from device tree */
	MPU_REGION_ENTRY(
		"sram",
		CONFIG_SRAM_BASE_ADDRESS,
		REGION_SRAM_SIZE,
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
			 NOT_EXEC}),
#elif (CONFIG_SRAM_BASE_ADDRESS <= 0x80000)
	/* DDR is contiguous with TCM - use single large region */
	MPU_REGION_ENTRY(
		"sram",
		0x00000000,
		MEMORY_REGION_SIZE,
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
			 NOT_EXEC}),
#else
	/* DDR is NOT contiguous with TCM - map TCM separately */
	MPU_REGION_ENTRY(
		"sram",
		0x00000000,
		REGION_512K,
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
			 NOT_EXEC}),
#endif

	/* Region 1: rom_region mapping for SRAM - RO + executable */
	MPU_REGION_ENTRY(
		"rom_region",
		(uint32_t)(&__rom_region_start),
		(uint32_t)(&__rom_region_mpu_size_bits),
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),

	/* Region 2: Consolidated peripherals - 2GB covering 0x80000000 to 0xFFFFFFFF
	 * Includes PL, QSPI, PCIe, PMC, STM, GIC, FPS, LPS, OCM (all device memory)
	 */
	MPU_REGION_ENTRY(
		"peripherals",
		0x80000000,
		REGION_2G,
		{.rasr = P_RW_U_NA_Msk |
			 DEVICE_SHAREABLE |
			 NOT_EXEC}),

	/* Region 3: OCM overlay - 256KB normal cacheable memory from 0xFFFC0000
	 * This overlays the peripheral region and marks OCM as cacheable
	 */
	MPU_REGION_ENTRY(
		"ocm",
		0xFFFC0000,
		REGION_256K,
		{.rasr = FULL_ACCESS_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),

	/* Region 4: Interrupt vectors at 0x0
	 * ARMv7-R requires vectors at 0x0 (HIVECS=0)
	 */
	MPU_REGION_ENTRY(
		"vectors",
		0x00000000,
		REGION_64B,
		{.rasr = P_RO_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE}),

#if (CONFIG_SRAM_BASE_ADDRESS > 0x80000)
	/* Region 5: Separate DDR region for non-contiguous memory layouts */
	MPU_REGION_ENTRY(
		"ddr",
		CONFIG_SRAM_BASE_ADDRESS,
		CALC_REGION_SIZE(CONFIG_SRAM_SIZE * 1024),
		{.rasr = FULL_ACCESS_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
