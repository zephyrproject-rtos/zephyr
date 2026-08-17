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

/*
 * The ITCM holds relocated code and data, so unlike a flash region it must be
 * writable and unlike the DTCM it must stay executable: these are the DTCM
 * attributes with XN left clear. REGION_RAM_ATTR does not fit, it sets XN when
 * CONFIG_XIP=y.
 */
#define REGION_ITCM_ATTR(size)                                                        \
	{(NORMAL_OUTER_INNER_NON_CACHEABLE_NON_SHAREABLE | P_RW_U_NA_Msk | (size))}

/*
 * An ARMv7-M MPU region must have a power-of-two size that is at least the
 * minimum region size, and a base address aligned to that size. Check the
 * raw devicetree bytes at build time so a bad reg (e.g. a board revision
 * fitting a differently sized part) fails to compile instead of faulting at
 * z_arm_mpu_init().
 */
#define MPU_REGION_BYTES_ASSERT(name, base, size)                                     \
	BUILD_ASSERT((size) >= CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE &&            \
			     ((size) & ((size) - 1)) == 0,                             \
		     #name " size must be a power of two >= the MPU minimum");        \
	BUILD_ASSERT(((base) & ((size) - 1)) == 0,                                    \
		     #name " base must be aligned to its size")

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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ocram))
/* M4-shared OCRAM alias: CM7 writes the CM4 image here (zephyr,cpu1-region). */
#define OCRAM_M4_SIZE                    DT_REG_SIZE_BY_IDX(DT_NODELABEL(ocram), 0)
#define REGION_OCRAM_M4_BASE_ADDRESS     DT_REG_ADDR_BY_IDX(DT_NODELABEL(ocram), 0)
#define REGION_OCRAM_M4_SIZE              \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(OCRAM_M4_SIZE))
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sdram0))
#define SDRAM_SIZE                       DT_REG_SIZE_BY_IDX(DT_NODELABEL(sdram0), 0)
#define REGION_SDRAM_BASE_ADDRESS        DT_REG_ADDR_BY_IDX(DT_NODELABEL(sdram0), 0)
#define REGION_SDRAM_SIZE                 \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(SDRAM_SIZE))
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hyperram0))
#define HYPER_RAM_SIZE                   DT_REG_SIZE_BY_IDX(DT_NODELABEL(hyperram0), 0)
#define REGION_HYPER_RAM_BASE_ADDRESS    DT_REG_ADDR_BY_IDX(DT_NODELABEL(hyperram0), 0)
#define REGION_HYPER_RAM_SIZE             \
			REGION_CUSTOMED_MEMORY_SIZE(MEMORY_REGION_SIZE_KB(HYPER_RAM_SIZE))
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(itcm))
MPU_REGION_BYTES_ASSERT(ITCM, REGION_ITCM_BASE_ADDRESS, ITCM_SIZE);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dtcm))
MPU_REGION_BYTES_ASSERT(DTCM, REGION_DTCM_BASE_ADDRESS, DTCM_SIZE);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(flexspi))
MPU_REGION_BYTES_ASSERT(QSPI_FLASH, REGION_QSPI_FLASH_BASE_ADDRESS, QSPI_FLASH_SIZE);
#endif
MPU_REGION_BYTES_ASSERT(PERIPHERAL, REGION_PERIPHERAL_BASE_ADDRESS, PERIPHERAL_SIZE);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ocram))
MPU_REGION_BYTES_ASSERT(OCRAM_M4, REGION_OCRAM_M4_BASE_ADDRESS, OCRAM_M4_SIZE);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sdram0))
MPU_REGION_BYTES_ASSERT(SDRAM, REGION_SDRAM_BASE_ADDRESS, SDRAM_SIZE);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hyperram0))
MPU_REGION_BYTES_ASSERT(HYPER_RAM, REGION_HYPER_RAM_BASE_ADDRESS, HYPER_RAM_SIZE);
#endif

static const struct arm_mpu_region mpu_regions[] = {
/*
 * The catch-all no-access region for unmapped addresses (Arm
 * Cortex-M7 erratum 1013783) is programmed by the MPU driver,
 * see CONFIG_ARM_MPU_CM7_UNMAPPED_REGION.
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(itcm))
	MPU_REGION_ENTRY("ITCM", REGION_ITCM_BASE_ADDRESS,
			 REGION_ITCM_ATTR(REGION_ITCM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dtcm))
	MPU_REGION_ENTRY("DTCM", REGION_DTCM_BASE_ADDRESS,
			 REGION_RAM_NOCACHE_ATTR(REGION_DTCM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ocram))
	MPU_REGION_ENTRY("OCRAM_M4", REGION_OCRAM_M4_BASE_ADDRESS,
			 ARM_MPU_SRAM_REGION_ATTR(REGION_OCRAM_M4_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sdram0))
	MPU_REGION_ENTRY("SDRAM", REGION_SDRAM_BASE_ADDRESS,
			 ARM_MPU_SRAM_REGION_ATTR(REGION_SDRAM_SIZE)),
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(hyperram0))
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
	 * aperture, so peripheral writes stay bufferable. Both halves of
	 * REGION_IO_ATTR are needed to keep the Cortex-M7 from touching this
	 * region speculatively: the Device memory type forbids speculative
	 * data reads, cache linefills and preloads, while Execute-Never
	 * forbids the speculative instruction fetches that branch prediction
	 * and the prefetch unit would otherwise issue.
	 */
	MPU_REGION_ENTRY("PERIPHERAL", REGION_PERIPHERAL_BASE_ADDRESS,
			 REGION_IO_ATTR(REGION_PERIPHERAL_SIZE)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
