/*
 * Copyright (c) 2023 Texas Instruments Incorporated
 * Copyright (c) 2023 L Lakshmanan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver handling Region based Address Translation (RAT)
 * related functions
 *
 * RAT is a module that is used by certain Texas Instruments SoCs
 * to allow some cores with a 32 bit address space to access
 * the full 48 bit SoC address space. This is required for the
 * core to be able to use peripherals.
 *
 * The driver uses the sys_mm_drv_page_phys_get() API to access
 * the address space.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mm/rat.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <zephyr/sys/__assert.h>

#define ADDR_TRANSLATE_MAX_REGIONS (16u)
#define RAT_CTRL(base_addr, i)     (base_addr + 0x20 + 0x10 * (i))
#define RAT_BASE(base_addr, i)     (base_addr + 0x24 + 0x10 * (i))
#define RAT_TRANS_L(base_addr, i)  (base_addr + 0x28 + 0x10 * (i))
#define RAT_TRANS_H(base_addr, i)  (base_addr + 0x2C + 0x10 * (i))
#define RAT_CTRL_W(enable, size)   (((enable & 0x1) << 31u) | (size & 0x3F))

/**
 * @brief Parameters for address_trans_init
 */
static struct address_trans_params {
	uint32_t num_regions;
	uint32_t rat_base_addr;
	struct address_trans_region_config *region_config;
} translate_config;

/**
 * @brief Set registers for the address regions being used
 *
 * @param addr_translate_config Pointer to config struct for the RAT module
 * @param region_num Number of regions being initialised
 * @param enable Region status
 */

static void address_trans_set_region(struct address_trans_region_config *region_config,
			uint16_t region_num, uint32_t enable)
{
	uint32_t rat_base_addr = translate_config.rat_base_addr;
	uint64_t system_addr = region_config->system_addr;
	uint32_t local_addr = region_config->local_addr;
	uint32_t size = region_config->size;
	uint32_t system_addrL, system_addrH;

	if (size > address_trans_region_size_4G) {
		size = address_trans_region_size_4G;
	}
	system_addrL = (uint32_t)(system_addr & ~((uint32_t)((BIT64_MASK(size)))));
	system_addrH = (uint32_t)((system_addr >> 32) & 0xFFFF);
	local_addr = local_addr & ~((uint32_t)(BIT64_MASK(size)));

	sys_write32(0, RAT_CTRL(rat_base_addr, region_num));
	sys_write32(local_addr, RAT_BASE(rat_base_addr, region_num));
	sys_write32(system_addrL, RAT_TRANS_L(rat_base_addr, region_num));
	sys_write32(system_addrH, RAT_TRANS_H(rat_base_addr, region_num));
	sys_write32(RAT_CTRL_W(enable, size), RAT_CTRL(rat_base_addr, region_num));
}

/**
 * @brief Initialise RAT module
 *
 * @param region_config Pointer to config struct for the regions
 * @param rat_base_addr Base address for the RAT module
 * @param translate_regions Number of regions being initialised
 */

void sys_mm_drv_ti_rat_init(struct address_trans_region_config *region_config,
			    uint64_t rat_base_addr, uint8_t translate_regions)
{
	uint32_t i;

	__ASSERT(translate_regions < ADDR_TRANSLATE_MAX_REGIONS, "Maximum regions exceeded");
	__ASSERT(rat_base_addr != 0, "RAT base address cannot be 0");
	__ASSERT(region_config != NULL, "RAT region config cannot be NULL");

	translate_config.num_regions = translate_regions;
	translate_config.rat_base_addr = rat_base_addr;
	translate_config.region_config = region_config;

	/* enable regions setup by user */
	for (i = 0; i < translate_regions; i++) {
		address_trans_set_region(&region_config[i], i, 1);
	}
}

int sys_mm_drv_page_phys_get(void *virt, uintptr_t *phys)
{
	if (phys == NULL) {
		/*
		 * NULL phys means the caller wants to check if a virtual address is
		 * valid. For RAT, even addresses without a mapping are still valid and
		 * are mapped as pass-through in HW, so we always return 0(valid) here.
		 */
		return 0;
	}
	uintptr_t pa = (uintptr_t) virt;
	uintptr_t *va = phys;

	uint32_t regionId;

	for (regionId = 0; regionId < translate_config.num_regions; regionId++) {
		struct address_trans_region_config *region_config =
			&translate_config.region_config[regionId];

		uint64_t start_addr = region_config->system_addr;
		uint64_t end_addr = start_addr + BIT64_MASK(region_config->size);

		if (pa < start_addr || pa > end_addr) {
			continue;
		}

		/* translate input address to output address */
		uint32_t offset = pa - start_addr;
		*va = region_config->local_addr + offset;

		return 0;
	}

	/* no mapping found, set output = input with 32b truncation */
	*va = pa;

	return 0;
}
