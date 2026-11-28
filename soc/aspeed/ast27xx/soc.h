/*
 * Copyright (c) 2026 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ASPEED_AST27XX_SOC_H_
#define ZEPHYR_SOC_ASPEED_AST27XX_SOC_H_

#include <stdint.h>

/**
 * @brief Translate a BootMCU address to a DMA physical address.
 *
 * @param addr Address in the BootMCU address space.
 * @return Physical address on the system bus.
 */
static inline uint64_t ast27xx_soc_virt_addr_to_phy_addr(uintptr_t addr)
{
	if (addr < 0x80000000U) {
		return addr;
	}

	return 0x400000000ULL + (addr - 0x80000000U);
}

#endif /* ZEPHYR_SOC_ASPEED_AST27XX_SOC_H_ */
