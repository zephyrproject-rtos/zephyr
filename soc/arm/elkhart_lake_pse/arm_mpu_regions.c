/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr.h>
#include <init.h>

#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu.h>
#include "arm_mpu_mem_cfg.h"
#if defined(CONFIG_ENABLE_SOC_NMI)
#include "soc_nmi.h"
#endif

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(arm_mpu_regions);


#define REGION_RWEX_ATTR(size)						 \
	{								 \
		(NORMAL_OUTER_INNER_WRITE_THROUGH_NON_SHAREABLE | size | \
		 P_RW_U_NA_Msk)						 \
	}

#define REGION_PPB_XN_ATTR(size) { (STRONGLY_ORDERED_SHAREABLE | size |	\
				    MPU_RASR_XN_Msk | P_RW_U_NA_Msk) }

extern char _ram_ro_start[];
extern char _ram_ro_end[];

extern char _l2sram_mpu_ro_region_end[];
extern char __l2sram_text_start[];

static struct arm_mpu_region mpu_regions[] = {
	MPU_REGION_ENTRY("BACK_NA", ARM_ADDRESS_SPACE,
			 REGION_NO_ACCESS_ATTR(REGION_4G)),

	MPU_REGION_ENTRY("PERIPHERAL", IO_ADDRESS_SPACE,
			 REGION_PPB_XN_ATTR(REGION_512M)),

	MPU_REGION_ENTRY("AONRF", CONFIG_AONRF_MEMORY_BASE_ADDRESS,
			 REGION_RWEX_ATTR(PSE_AONRF_SIZE)),

#if  defined(CONFIG_EN_MPU_L2SRAM_MEM_ACCESS)
	MPU_REGION_ENTRY("L2SRAM", CONFIG_L2SRAM_RW_MEMORY_BASE_ADDRESS,
			 REGION_RAM_ATTR(L2SRAM_RW_SIZE)),
#endif

#if  defined(CONFIG_EN_MPU_ICCM_MEM_ACCESS)
	MPU_REGION_ENTRY("ICCM", ICCM_MEM_START_ADDR,
			 REGION_FLASH_ATTR(REGION_512K | PSE_ICCM_RASR_SRD)),
#endif

#if  defined(CONFIG_EN_MPU_DCCM_MEM_ACCESS)
	MPU_REGION_ENTRY("DCCM", DCCM_MEM_START_ADDR,
			 REGION_RAM_ATTR(REGION_512K | PSE_DCCM_RASR_SRD)),
#endif

#if defined(CONFIG_EN_L2SRAM_RELOCATION_MEM)
#error "L2SRAM relocation not supported"
#else
	MPU_REGION_ENTRY("SRAM_RO", CONFIG_L2SRAM_RO_MEMORY_BASE_ADDRESS,
			 REGION_FLASH_ATTR(L2SRAM_RO_SIZE)),
#endif
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions), .mpu_regions = mpu_regions,
};
