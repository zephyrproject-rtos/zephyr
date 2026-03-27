/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_BFLB_BL70X_L_PSRAM_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_BFLB_BL70X_L_PSRAM_H_

/**
 * @brief Initialize external SPI PSRAM via SF_CTRL bank 2.
 *
 * Configures GPIO pins for SF2 pad, initializes the PSRAM chip
 * (APMemory APS1604M-SQ), and programs the IAHB cache read/write
 * paths for memory-mapped access at 0x26000000.
 *
 * Must be called with SF_CTRL in SAHB ownership (interrupts locked).
 * Restores IAHB ownership before returning.
 *
 * @param sf_ctrl_base SF_CTRL register base address (0x4000b000)
 * @return 0 on success, negative errno on failure
 */
int flash_bflb_bl70x_l_psram_init(uint32_t sf_ctrl_base);

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_BFLB_BL70X_L_PSRAM_H_ */
