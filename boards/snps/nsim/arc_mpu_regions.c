/*
 * Copyright (c) 2017 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/arch/arc/v2/mpu/arc_mpu.h>
#include <zephyr/arch/arc/arch.h>
#include <zephyr/linker/linker-defs.h>

/*
 * for secure firmware, MPU entries are only set up for secure world.
 * All regions not listed here are shared by secure world and normal world.
 */
static struct arc_mpu_region mpu_regions[] = {

#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
	/* Region Coverage */
	MPU_REGION_ENTRY("COVERAGE",
			 (uint32_t)&(__gcov_bss_start),
			 (uint32_t)&__gcov_bss_size,
			 REGION_IO_ATTR),
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */

#ifdef CONFIG_HARVARD
#if DT_REG_SIZE(DT_INST(0, arc_iccm)) > 0
	/* Region ICCM */
	MPU_REGION_ENTRY("ICCM",
			 DT_REG_ADDR(DT_INST(0, arc_iccm)),
			 DT_REG_SIZE(DT_INST(0, arc_iccm)),
			 REGION_ROM_ATTR),
#endif
#if DT_REG_SIZE(DT_INST(0, arc_dccm)) > 0
	/* Region DCCM */
	MPU_REGION_ENTRY("DCCM",
			 DT_REG_ADDR(DT_INST(0, arc_dccm)),
			 DT_REG_SIZE(DT_INST(0, arc_dccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif
#if DT_REG_SIZE(DT_INST(0, arc_xccm)) > 0
	/* Region XCCM */
	MPU_REGION_ENTRY("XCCM",
			 DT_REG_ADDR(DT_INST(0, arc_xccm)),
			 DT_REG_SIZE(DT_INST(0, arc_xccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif
#if DT_REG_SIZE(DT_INST(0, arc_yccm)) > 0
	/* Region YCCM */
	MPU_REGION_ENTRY("YCCM",
			 DT_REG_ADDR(DT_INST(0, arc_yccm)),
			 DT_REG_SIZE(DT_INST(0, arc_yccm)),
			 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
#endif

#else /* !CONFIG_HARVARD */

#if DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) > 0
	#if CONFIG_XIP
		/* Region RAM */
		MPU_REGION_ENTRY("RAM",
				 DT_REG_ADDR(DT_CHOSEN(zephyr_sram)),
				 DT_REG_SIZE(DT_CHOSEN(zephyr_sram)),
				 REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
	#else /* !CONFIG_XIP */
		MPU_REGION_ENTRY("RAM_RX",
				 (uintptr_t)__rom_region_start,
				 (uintptr_t)__rom_region_size,
				 REGION_ROM_ATTR),

		 MPU_REGION_ENTRY("RAM_RW",
				(uintptr_t)_image_ram_start,
				(uintptr_t)__arc_rw_sram_size,
				REGION_KERNEL_RAM_ATTR | REGION_DYNAMIC),
	#endif /* CONFIG_XIP */
#endif /* zephyr_sram > 0 */

#if DT_REG_SIZE(DT_CHOSEN(zephyr_flash)) > 0
	/* Region FLASH */
	MPU_REGION_ENTRY("FLASH",
			 DT_REG_ADDR(DT_CHOSEN(zephyr_flash)),
			 DT_REG_SIZE(DT_CHOSEN(zephyr_flash)),
			 REGION_ROM_ATTR),
#endif

#endif /* CONFIG_HARVARD */

/*
 * Region peripheral is shared by secure world and normal world by default,
 * no need a static mpu entry. If some peripherals belong to secure world,
 * add it here.
 */
#ifndef CONFIG_ARC_SECURE_FIRMWARE
	/* Region Peripheral */
	MPU_REGION_ENTRY("PERIPHERAL",
			 0xF0000000,
			 64 * 1024,
			 REGION_KERNEL_RAM_ATTR),
#endif
};

struct arc_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
