/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Shared Cortex-M7 MPU region table for the i.MX RT11xx EVK/FRDM boards
 * (RT1170/RT1160/RT1150). Boards select it through
 * CONFIG_BOARD_NXP_SPECIFIC_MPU_SETTINGS; the RAM regions are guarded on
 * their devicetree nodelabels so each board contributes only the regions
 * its SoC actually exposes:
 *
 *   - ocram   : CM4-shared OCRAM alias (dual-core RT1170/RT1160 only)
 *   - sdram0  : external SDRAM         (RT1170/RT1160 EVK)
 *   - hyperram0 : external HyperRAM    (single-core RT1150 FRDM)
 *
 * The #define for each RAM region lives inside its DT_NODE_HAS_STATUS_OKAY
 * block on purpose: the preprocessor drops the whole block on a board that
 * lacks the nodelabel, so DT_NODELABEL() is never expanded against a
 * missing node.
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arm/mpu/arm_mpu_mem_cfg.h>

#ifdef CONFIG_ARM_MPU_SRAM_WRITE_THROUGH
#define ARM_MPU_SRAM_REGION_ATTR  REGION_RAM_WT_ATTR
#else
#define ARM_MPU_SRAM_REGION_ATTR  REGION_RAM_ATTR
#endif

#define MEMORY_REGION_SIZE_KB(SIZE)    (SIZE / 1024)

#define ITCM_SIZE                       DT_REG_SIZE_BY_IDX(DT_NODELABEL(itcm), 0)
#define DTCM_SIZE                       DT_REG_SIZE_BY_IDX(DT_NODELABEL(dtcm), 0)
#define QSPI_FLASH_SIZE                 DT_REG_SIZE_BY_IDX(DT_NODELABEL(flexspi), 1)
#define PERIPHERAL_SIZE                 DT_REG_SIZE_BY_IDX(DT_NODELABEL(peripheral), 0)

#define REGION_ITCM_BASE_ADDRESS         DT_REG_ADDR_BY_IDX(DT_NODELABEL(itcm), 0)
#define REGION_ITCM_SIZE                 \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(ITCM_SIZE))
#define REGION_DTCM_BASE_ADDRESS         DT_REG_ADDR_BY_IDX(DT_NODELABEL(dtcm), 0)
#define REGION_DTCM_SIZE                 \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(DTCM_SIZE))
#define REGION_QSPI_FLASH_BASE_ADDRESS   DT_REG_ADDR_BY_IDX(DT_NODELABEL(flexspi), 1)
#define REGION_QSPI_FLASH_SIZE            \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(QSPI_FLASH_SIZE))
#define REGION_PERIPHERAL_BASE_ADDRESS   DT_REG_ADDR_BY_IDX(DT_NODELABEL(peripheral), 0)
#define REGION_PERIPHERAL_SIZE            \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(PERIPHERAL_SIZE))

static const struct arm_mpu_region mpu_regions[] = {
/*
 * The catch-all no-access region for unmapped addresses (Arm
 * Cortex-M7 erratum 1013783) is programmed by the MPU driver,
 * see CONFIG_ARM_MPU_CM7_UNMAPPED_REGION.
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(itcm))
	MPU_REGION_ENTRY("ITCM", REGION_ITCM_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_ITCM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dtcm))
	MPU_REGION_ENTRY("DTCM", REGION_DTCM_BASE_ADDRESS,
			 REGION_RAM_NOCACHE_ATTR(REGION_DTCM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ocram))
	/* M4-shared OCRAM alias: CM7 writes the CM4 image here (zephyr,cpu1-region). */
#define OCRAM_M4_SIZE                    DT_REG_SIZE_BY_IDX(DT_NODELABEL(ocram), 0)
#define REGION_OCRAM_M4_BASE_ADDRESS     DT_REG_ADDR_BY_IDX(DT_NODELABEL(ocram), 0)
#define REGION_OCRAM_M4_SIZE              \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(OCRAM_M4_SIZE))
	MPU_REGION_ENTRY("OCRAM_M4", REGION_OCRAM_M4_BASE_ADDRESS,
			 ARM_MPU_SRAM_REGION_ATTR(REGION_OCRAM_M4_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sdram0))
#define SDRAM_SIZE                       DT_REG_SIZE_BY_IDX(DT_NODELABEL(sdram0), 0)
#define REGION_SDRAM_BASE_ADDRESS        DT_REG_ADDR_BY_IDX(DT_NODELABEL(sdram0), 0)
#define REGION_SDRAM_SIZE                 \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(SDRAM_SIZE))
	MPU_REGION_ENTRY("SDRAM", REGION_SDRAM_BASE_ADDRESS,
			 ARM_MPU_SRAM_REGION_ATTR(REGION_SDRAM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hyperram0))
#define HYPER_RAM_SIZE                   DT_REG_SIZE_BY_IDX(DT_NODELABEL(hyperram0), 0)
#define REGION_HYPER_RAM_BASE_ADDRESS    DT_REG_ADDR_BY_IDX(DT_NODELABEL(hyperram0), 0)
#define REGION_HYPER_RAM_SIZE             \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(HYPER_RAM_SIZE))
	MPU_REGION_ENTRY("HYPER_RAM", REGION_HYPER_RAM_BASE_ADDRESS,
			 ARM_MPU_SRAM_REGION_ATTR(REGION_HYPER_RAM_SIZE)),
#endif

	/*
	 * Guard on the FlexSPI controller, which is also where the AMBA
	 * window base/size come from. Guarding on the flash part instead
	 * would silently drop this region on board revisions that fit a
	 * different part, and the core executes from this window.
	 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexspi))
	MPU_REGION_ENTRY("QSPI_FLASH", REGION_QSPI_FLASH_BASE_ADDRESS,
			 REGION_FLASH_ATTR(REGION_QSPI_FLASH_SIZE)),
#endif

	/*
	 * Device rather than Strongly-Ordered: this matches the memory type
	 * the ARMv7-M default memory map already assigned to the peripheral
	 * aperture, so peripheral writes stay bufferable. The Execute-Never
	 * attribute, not the memory type, is what stops the Cortex-M7 from
	 * speculatively fetching from this region.
	 */
	MPU_REGION_ENTRY("PERIPHERAL", REGION_PERIPHERAL_BASE_ADDRESS,
			 REGION_IO_ATTR(REGION_PERIPHERAL_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
