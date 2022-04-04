/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BOOT_MODE_H_

#define BOOT_MODE_REQ_NORMAL		0x00
#define BOOT_MODE_REQ_RECOVERY_DFU	0x01

/**
 * Set bootloader mode
 *
 * Using this the application can request specific bootloader mode.
 * Usually the bootloader might enter this mode once system reboot.
 *
 * @param mode requested mode
 *
 * @retval 0 on success, negative errno code on fail.
 */
int boot_mode_set(uint8_t mode);

/**
 * Get bootloader mode
 *
 * Using this the bootloader might obtain the mode requested by the application.
 * Only first call returns valid value as the implementation should cleanup
 * hardware resource used for transferring the mode from the application.
 *
 * @retval bootloader mode
 */
uint8_t boot_mode_get(void);

#endif /* ZEPHYR_INCLUDE_BOOT_MODE_H_ */
