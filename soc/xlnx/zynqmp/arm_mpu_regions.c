/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 Lexmark International, Inc.
 * Copyright (c) 2025 Immo Birnbaum
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

extern const uint32_t __rom_region_start;
extern const uint32_t __rom_region_mpu_size_bits;

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ipc_shm), okay)
BUILD_ASSERT((DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm)) % 1024U) == 0U,
	     "zephyr,ipc_shm size must be a multiple of 1 KiB");
BUILD_ASSERT(REGION_CUSTOMED_MEMORY_SIZE(DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm)) / 1024U) !=
		     REGION_SIZE_UNSUPPORTED,
	     "zephyr,ipc_shm size is not representable by the ARMv7-R MPU");
#define ZYNQMP_IPC_SHM_REGION_SIZE \
	REGION_CUSTOMED_MEMORY_SIZE(DT_REG_SIZE(DT_CHOSEN(zephyr_ipc_shm)) / 1024U)
#endif

/*
 * ARMv7-R MPU natural alignment rule: base_address % region_size == 0.
 * The hardware silently masks the low bits of the base address when a region
 * is misaligned, mapping the wrong physical range with no build error.
 *
 * Two constraints must both be satisfied:
 *   1. The chosen base must be naturally aligned to the region size.
 *   2. The region must cover the full range [base, DDR_END) where
 *      DDR_END = DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE.
 *      Using only DT_CHOSEN_SRAM_SIZE as the region size fails when
 *      DT_CHOSEN_SRAM_ADDR is non-zero (e.g. 1 MiB offset pushes DDR_END
 *      past the next 256 MiB boundary, leaving the top of stack uncovered).
 *
 * Strategy: for each candidate size, round DT_CHOSEN_SRAM_ADDR DOWN to the
 * nearest naturally-aligned boundary to get the candidate base, then check
 * whether base + size >= DDR_END.  Select the smallest size that fits.
 * This finds the tightest single region rather than always anchoring at 0x0,
 * which is critical for AMP designs where the R5 app occupies only a high
 * portion of DDR (e.g. 0x50000000-0x7FEFFFFF) and mapping 0x0 onwards would
 * expose Linux DDR as R5-cacheable normal memory.
 *
 * Alignment is guaranteed by construction: (SRAM_ADDR & ~(size-1)) is always
 * a multiple of size for any power-of-2 size.
 *
 * Examples:
 *   SRAM=0x100000, SIZE=256M → DDR_END=0x10100000:
 *     256M: base=0x0, end=0x10000000 < DDR_END → skip
 *     512M: base=0x0, end=0x20000000 >= DDR_END → base=0x00000000, REGION_512M
 *
 *   SRAM=0x50000000, SIZE=767M → DDR_END=0x7FF00000:
 *     256M: base=0x50000000, end=0x60000000 < DDR_END → skip
 *     512M: base=0x40000000, end=0x60000000 < DDR_END → skip
 *     1G:   base=0x40000000, end=0x80000000 >= DDR_END → base=0x40000000, REGION_1G
 *     (vs old anchor-at-0: base=0x0, REGION_2G — 1 GB of Linux DDR over-mapped)
 */
#define ZYNQMP_DDR_END     (DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE)

/* Naturally-aligned base for each candidate size */
#define ZYNQMP_256M_BASE   ((DT_CHOSEN_SRAM_ADDR) & 0xF0000000U)
#define ZYNQMP_512M_BASE   ((DT_CHOSEN_SRAM_ADDR) & 0xE0000000U)
#define ZYNQMP_1G_BASE     ((DT_CHOSEN_SRAM_ADDR) & 0xC0000000U)

/* Exclusive end of each candidate region */
#define ZYNQMP_256M_REND   (ZYNQMP_256M_BASE + 0x10000000U)
#define ZYNQMP_512M_REND   (ZYNQMP_512M_BASE + 0x20000000U)
#define ZYNQMP_1G_REND     (ZYNQMP_1G_BASE   + 0x40000000U)

#if (ZYNQMP_DDR_END <= ZYNQMP_256M_REND)
#  define ZYNQMP_SRAM_REGION_BASE  ZYNQMP_256M_BASE
#  define ZYNQMP_SRAM_REGION_SIZE  REGION_256M
#elif (ZYNQMP_DDR_END <= ZYNQMP_512M_REND)
#  define ZYNQMP_SRAM_REGION_BASE  ZYNQMP_512M_BASE
#  define ZYNQMP_SRAM_REGION_SIZE  REGION_512M
#elif (ZYNQMP_DDR_END <= ZYNQMP_1G_REND)
#  define ZYNQMP_SRAM_REGION_BASE  ZYNQMP_1G_BASE
#  define ZYNQMP_SRAM_REGION_SIZE  REGION_1G
#else
#  define ZYNQMP_SRAM_REGION_BASE  0x00000000U
#  define ZYNQMP_SRAM_REGION_SIZE  REGION_2G
#endif

static const struct arm_mpu_region mpu_regions[] = {
	/* SRAM/DDR: R/W + XN, tightest naturally-aligned region covering
	 * [ZYNQMP_SRAM_REGION_BASE, DDR_END).  Base and size are computed at
	 * build time from DT_CHOSEN_SRAM_ADDR + DT_CHOSEN_SRAM_SIZE above.
	 */
	MPU_REGION_ENTRY(
		"sram",
		ZYNQMP_SRAM_REGION_BASE,
		ZYNQMP_SRAM_REGION_SIZE,
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE |
			 NOT_EXEC}),
#if defined(CONFIG_XIP)
	/* .text and .rodata (=rom_region) are in flash, must be RO + executable */
	MPU_REGION_ENTRY(
		"rom_region",
		CONFIG_FLASH_BASE_ADDRESS,
		REGION_FLASH_SIZE,
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE}),
#else /* !CONFIG_XIP */
#if CONFIG_FLASH_SIZE > 0
	/* Keep any real flash window RO + XN when executing from RAM. */
	MPU_REGION_ENTRY(
		"flash",
		CONFIG_FLASH_BASE_ADDRESS,
		REGION_FLASH_SIZE,
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_NON_SHAREABLE |
			 NOT_EXEC}),
#endif
	/* rom_region: code + rodata in SRAM — RO + executable */
	MPU_REGION_ENTRY(
		"rom_region",
		(uint32_t)(&__rom_region_start),
		(uint32_t)(&__rom_region_mpu_size_bits),
		{.rasr = P_RO_U_RO_Msk |
			 NORMAL_OUTER_INNER_WRITE_BACK_WRITE_READ_ALLOCATE_NON_SHAREABLE}),
#endif /* CONFIG_XIP */
	MPU_REGION_ENTRY(
		"peripherals",
		0xf8000000,
		REGION_128M,
		{.rasr = P_RW_U_NA_Msk |
			 DEVICE_SHAREABLE |
			 NOT_EXEC}),
#if (DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ocm), okay))
	MPU_REGION_ENTRY(
		"ocm",
		DT_REG_ADDR(DT_CHOSEN(zephyr_ocm)),
		REGION_256K,
		{.rasr = FULL_ACCESS_Msk |
			 STRONGLY_ORDERED_SHAREABLE |
			 NOT_EXEC}),
#endif
#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_ipc_shm), okay)
	MPU_REGION_ENTRY(
		"ipc_shm",
		DT_REG_ADDR(DT_CHOSEN(zephyr_ipc_shm)),
		ZYNQMP_IPC_SHM_REGION_SIZE,
		{.rasr = P_RW_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_SHAREABLE}),
#endif
	/*
	 * The address of the vectors is determined by arch/arm/core/cortex_a_r/prep_c.c
	 * -> for v7-R, there's no other option than 0x0, HIVECS always gets cleared
	 */
	MPU_REGION_ENTRY(
		"vectors",
		0x00000000,
		REGION_64B,
		{.rasr = P_RO_U_NA_Msk |
			 NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE}),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
