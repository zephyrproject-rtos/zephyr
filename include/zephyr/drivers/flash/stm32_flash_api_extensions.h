/*
 * Copyright (c) 2023 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/flash.h>

enum stm32_ex_ops {
	/*
	 * STM32 sector write protection control.
	 *
	 * As an input this operation takes a structure with two sector masks,
	 * first mask is used to enable protection on sectors, while second mask
	 * is used to do the opposite. If both masks are 0, then protection will
	 * remain unchanged. If same sector is set on both masks, protection
	 * will be enabled.
	 *
	 * As an output, sector mask with enabled protection is returned.
	 * Input can be NULL if we only want to get protected sectors.
	 * Output can be NULL if not needed.
	 */
	FLASH_STM32_EX_OP_SECTOR_WP = FLASH_EX_OP_VENDOR_BASE,
};

#if defined(CONFIG_FLASH_STM32_WRITE_PROTECT)
struct flash_stm32_ex_op_sector_wp_in {
	uint32_t enable_mask;
	uint32_t disable_mask;
};

struct flash_stm32_ex_op_sector_wp_out {
	uint32_t protected_mask;
};
#endif /* CONFIG_FLASH_STM32_WRITE_PROTECT */
