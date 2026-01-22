/*
 * Copyright (c) 2025 Vincent Jardin, Free Mobile
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

FLASH_START = CONFIG_FLASH_BASE_ADDRESS;
FLASH_LENGTH = (CONFIG_FLASH_SIZE * 1K);
FLASH_END = FLASH_START + FLASH_LENGTH;

DATA_FLASH_START = DT_REG_ADDR(DT_NODELABEL(flash1));
DATA_FLASH_LENGTH = DT_REG_SIZE(DT_NODELABEL(flash1));
DATA_FLASH_END = DATA_FLASH_START + DATA_FLASH_LENGTH;

RAM_START = CONFIG_SRAM_BASE_ADDRESS;
RAM_LENGTH = (CONFIG_SRAM_SIZE * 1K);
RAM_END = RAM_START + RAM_LENGTH;

#if !defined(CONFIG_ARM_SECURE_FIRMWARE) && !defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	/*
	 * Boundaries setting in flat project.
	 * Set entire code flash as secure.
	 * Set entire data flash as secure.
	 * These symbols used by partition tool (gen_rpd.py).
	 */

	__FLASH_S = ABSOLUTE(FLASH_START);
	__FLASH_NSC = ABSOLUTE(FLASH_END);
	__FLASH_NS = ABSOLUTE(FLASH_END);

	__DATA_FLASH_S = ABSOLUTE(DATA_FLASH_START);
	__DATA_FLASH_NS = ABSOLUTE(DATA_FLASH_END);

	__RAM_S = ORIGIN(RAM);
	__RAM_NSC = ABSOLUTE(RAM_END);
	__RAM_NS = ABSOLUTE(RAM_END);
#else
	/*
	 * Boundaries setting in TrustZone project is not implemented.
	 * TODO: Define secure/non-secure callable/non-secure regions
	 * for proper TrustZone partitioning.
	 */
#endif
