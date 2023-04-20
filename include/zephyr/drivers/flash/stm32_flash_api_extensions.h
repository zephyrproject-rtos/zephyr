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
	/*
	 * STM32 sector readout protection control.
	 *
	 * As an input this operation takes structure with information about
	 * desired RDP state. As an output the status after applying changes
	 * is returned.
	 */
	FLASH_STM32_EX_OP_RDP,
	/*
	 * STM32 block option register.
	 *
	 * This operation causes option register to be locked until next boot.
	 * After calling, it's not possible to change option bytes (WP, RDP,
	 * user bytes).
	 */
	FLASH_STM32_EX_OP_BLOCK_OPTION_REG,
	/*
	 * STM32 block control register.
	 *
	 * This operation causes control register to be locked until next boot.
	 * After calling, it's not possible to perform basic operation like
	 * erasing or writing.
	 */
	FLASH_STM32_EX_OP_BLOCK_CONTROL_REG,
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

#if defined(CONFIG_FLASH_STM32_READOUT_PROTECTION)
struct flash_stm32_ex_op_rdp {
	bool enable;
	bool permanent;
};
#endif /* CONFIG_FLASH_STM32_READOUT_PROTECTION */
