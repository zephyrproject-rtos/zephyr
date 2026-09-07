/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RA_RA6B1_SYSTEM_H_
#define ZEPHYR_SOC_ARM_RENESAS_RA_RA6B1_SYSTEM_H_

#include <zephyr/sys/util.h>
#include <bsp_feature.h>
#include <bsp_cfg_ra6b1.h>

#define __FLASH_NVMC_NSC_CODE_END \
	(BSP_FEATURE_FLASH_NVMC_W_NSC_CODE_START + BSP_FEATURE_FLASH_NVMC_W_SIZE_BYTES)

#define __SYSTEM_ADDRESS_OFFSET \
	(BSP_FEATURE_FLASH_NVMC_W_NSC_CODE_START ^ BSP_FEATURE_FLASH_NVMC_W_S_DATA_START)

#define __CODE_TO_SYSTEM_ADDRESS(_a) ((_a) + __SYSTEM_ADDRESS_OFFSET)

#define IS_CODE_NSC_ADDRESS(_a) IN_RANGE((_a), BSP_FEATURE_FLASH_NVMC_W_NSC_CODE_START, \
					       __FLASH_NVMC_NSC_CODE_END)

/*
 * Helper function to check if an address reflects the code address space through
 * the I-Cache controller and translate it into the AHB-S(ystem) memory region.
 * This is mandatory as other masters on the AHB bus e.g. DMA accelerator
 * can only operate within the system address space. Failure to do so, should
 * result in the master failing to execute a given transaction.
 */
static inline uintptr_t translate_code_to_sys_addr(uintptr_t addr)
{
	uintptr_t sys_addr = addr;

	if (IS_CODE_NSC_ADDRESS(addr)) {
		sys_addr = __CODE_TO_SYSTEM_ADDRESS(addr);
	}

	return sys_addr;
}

#endif /* ZEPHYR_SOC_ARM_RENESAS_RA_RA6B1_SYSTEM_H_ */
