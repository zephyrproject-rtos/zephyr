/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#if !((defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE > 0)) || (DT_CHOSEN_SRAM_SIZE > 0))
#error "NPCX9 fixed MPU regions require flash0 or sram0"
#endif

#ifdef CONFIG_ARM_MPU_SRAM_WRITE_THROUGH
#define NPCX9_SRAM_REGION_ATTR REGION_RAM_WT_ATTR
#else
#define NPCX9_SRAM_REGION_ATTR REGION_RAM_ATTR
#endif

/*
 * ARMv7-M PMSAv7 requires every MPU region base address to be aligned to its
 * power-of-two region size. NPCX9 flash0 and sram0 nodes use 32 KiB-aligned
 * addresses and span 256 KiB boundaries on several variants (e.g. npcx9m7f,
 * npcx9m7fb, npcx9mfp). Cover each active range with up to two 256 KiB aligned
 * blocks and 32 KiB subregion disable masks (MPU_RASR_SRD).
 */
#define NPCX9_CODE_RAM_BG_BASE 0x10000000UL
#define NPCX9_CODE_RAM_BG_SIZE 0x00100000UL
#define NPCX9_CODE_RAM_BG_END  (NPCX9_CODE_RAM_BG_BASE + NPCX9_CODE_RAM_BG_SIZE)

#define NPCX9_MPU_BLOCK_SIZE  0x00040000UL /* 256 KiB */
#define NPCX9_MPU_BLOCK_MASK  (~(NPCX9_MPU_BLOCK_SIZE - 1UL))
#define NPCX9_MPU_SUBREG_SIZE (NPCX9_MPU_BLOCK_SIZE / 8UL) /* 32 KiB */

#define NPCX9_MPU_SUBREG_DIS(block, idx, start, end)                                               \
	((((block) + ((idx) + 1UL) * NPCX9_MPU_SUBREG_SIZE <= (start)) ||                          \
	  ((block) + (idx) * NPCX9_MPU_SUBREG_SIZE >= (end)))                                      \
		 ? BIT(idx)                                                                        \
		 : 0UL)

#define NPCX9_MPU_SRD_MASK(block, start, end)                                                      \
	((NPCX9_MPU_SUBREG_DIS(block, 0UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 1UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 2UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 3UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 4UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 5UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 6UL, start, end) |                                           \
	  NPCX9_MPU_SUBREG_DIS(block, 7UL, start, end))                                            \
	 << MPU_RASR_SRD_Pos)

#if defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE > 0)
BUILD_ASSERT((CONFIG_FLASH_SIZE + 0UL) <= (NPCX9_CODE_RAM_BG_SIZE / 1024UL),
	     "NPCX9 flash0 size exceeds the 1 MiB code RAM window");

#define NPCX9_FLASH_START (CONFIG_FLASH_BASE_ADDRESS + 0UL)
#define NPCX9_FLASH_SIZE  ((CONFIG_FLASH_SIZE + 0UL) * 1024UL)
#define NPCX9_FLASH_END   (NPCX9_FLASH_START + NPCX9_FLASH_SIZE)
#define NPCX9_FLASH_BLK0  (NPCX9_FLASH_START & NPCX9_MPU_BLOCK_MASK)
#define NPCX9_FLASH_BLK1  (NPCX9_FLASH_BLK0 + NPCX9_MPU_BLOCK_SIZE)

BUILD_ASSERT((NPCX9_FLASH_START % NPCX9_MPU_SUBREG_SIZE) == 0UL,
	     "NPCX9 flash0 base must be 32 KiB aligned");
BUILD_ASSERT((NPCX9_FLASH_SIZE % NPCX9_MPU_SUBREG_SIZE) == 0UL,
	     "NPCX9 flash0 size must be 32 KiB aligned");
BUILD_ASSERT(NPCX9_FLASH_START >= NPCX9_CODE_RAM_BG_BASE &&
		     NPCX9_FLASH_START <= NPCX9_CODE_RAM_BG_END &&
		     NPCX9_FLASH_SIZE <= (NPCX9_CODE_RAM_BG_END - NPCX9_FLASH_START),
	     "NPCX9 flash0 must reside within the 1 MiB code RAM window");
BUILD_ASSERT(NPCX9_FLASH_SIZE <=
		     ((2UL * NPCX9_MPU_BLOCK_SIZE) - (NPCX9_FLASH_START - NPCX9_FLASH_BLK0)),
	     "NPCX9 flash0 exceeds two 256 KiB MPU regions");
#endif

#if DT_CHOSEN_SRAM_SIZE > 0
#define NPCX9_SRAM_START (DT_CHOSEN_SRAM_ADDR + 0UL)
#define NPCX9_SRAM_SIZE  (DT_CHOSEN_SRAM_SIZE + 0UL)
#define NPCX9_SRAM_END   (NPCX9_SRAM_START + NPCX9_SRAM_SIZE)
#define NPCX9_SRAM_BLK0  (NPCX9_SRAM_START & NPCX9_MPU_BLOCK_MASK)
#define NPCX9_SRAM_BLK1  (NPCX9_SRAM_BLK0 + NPCX9_MPU_BLOCK_SIZE)

/*
 * NPCX9 sram0 start is always 32 KiB aligned, whereas its size may end 4 KiB
 * short of a 32 KiB boundary (e.g. 0x200d7000 on npcx9mfp) due to the trailing
 * 4 KiB bootloader_ram node. Rounding coverage up to the enclosing 32 KiB
 * subregion keeps that tail privileged RW and execute-never without bleeding
 * below sram0.start.
 */
BUILD_ASSERT((NPCX9_SRAM_START % NPCX9_MPU_SUBREG_SIZE) == 0UL,
	     "NPCX9 sram0 base must be 32 KiB aligned");
BUILD_ASSERT(NPCX9_SRAM_SIZE <= (UINT32_MAX - NPCX9_SRAM_START) &&
		     NPCX9_SRAM_BLK0 <= (UINT32_MAX - (2UL * NPCX9_MPU_BLOCK_SIZE)),
	     "NPCX9 sram0 address range overflows 32-bit address space");
BUILD_ASSERT(NPCX9_SRAM_SIZE <=
		     ((2UL * NPCX9_MPU_BLOCK_SIZE) - (NPCX9_SRAM_START - NPCX9_SRAM_BLK0)),
	     "NPCX9 sram0 exceeds two 256 KiB MPU regions");
#endif

static const struct arm_mpu_region mpu_regions[] = {
#if defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE > 0)
	/*
	 * Lowest-priority background region for the 1 MiB Code-bus RAM window
	 * (0x10000000 - 0x100fffff): deny access and execution outside flash0
	 * (including unused low code RAM and the Code-bus alias of Data RAM)
	 * instead of falling through to the privileged default memory map.
	 */
	MPU_REGION_ENTRY("CODE_RAM_BG", NPCX9_CODE_RAM_BG_BASE,
			 {REGION_1M | STRONGLY_ORDERED_SHAREABLE | NOT_EXEC | NO_ACCESS_Msk}),
	MPU_REGION_ENTRY("FLASH_0_0", NPCX9_FLASH_BLK0,
			 REGION_FLASH_ATTR((REGION_256K |
					    NPCX9_MPU_SRD_MASK(NPCX9_FLASH_BLK0, NPCX9_FLASH_START,
							       NPCX9_FLASH_END)))),
#if NPCX9_FLASH_SIZE > (NPCX9_MPU_BLOCK_SIZE - (NPCX9_FLASH_START - NPCX9_FLASH_BLK0))
	MPU_REGION_ENTRY("FLASH_0_1", NPCX9_FLASH_BLK1,
			 REGION_FLASH_ATTR((REGION_256K |
					    NPCX9_MPU_SRD_MASK(NPCX9_FLASH_BLK1, NPCX9_FLASH_START,
							       NPCX9_FLASH_END)))),
#endif
#endif
#if DT_CHOSEN_SRAM_SIZE > 0
	MPU_REGION_ENTRY("SRAM_0_0", NPCX9_SRAM_BLK0,
			 NPCX9_SRAM_REGION_ATTR((
				 REGION_256K | NPCX9_MPU_SRD_MASK(NPCX9_SRAM_BLK0, NPCX9_SRAM_START,
								  NPCX9_SRAM_END)))),
#if NPCX9_SRAM_SIZE > (NPCX9_MPU_BLOCK_SIZE - (NPCX9_SRAM_START - NPCX9_SRAM_BLK0))
	MPU_REGION_ENTRY("SRAM_0_1", NPCX9_SRAM_BLK1,
			 NPCX9_SRAM_REGION_ATTR((
				 REGION_256K | NPCX9_MPU_SRD_MASK(NPCX9_SRAM_BLK1, NPCX9_SRAM_START,
								  NPCX9_SRAM_END)))),
#endif
#endif
};

/*
 * Cortex-M4 has 8 MPU regions; keep fixed entries <= 5 so up to 1 null-pointer
 * detection region, 1 dynamic stack guard, and 1 chip-reserved slot remain free.
 */
BUILD_ASSERT(ARRAY_SIZE(mpu_regions) > 0 && ARRAY_SIZE(mpu_regions) <= 5,
	     "NPCX9 fixed MPU regions must leave room for dynamic regions");

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
