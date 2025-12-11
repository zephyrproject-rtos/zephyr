/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_
#define ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_

/**
 * @brief Configures the Power Controller as necessary
 * for proper operation of the USB controllers
 *
 * @returns 0 on success, negative error code otherwise.
 */
int stm32_usb_pwr_enable(void);

/**
 * @brief Configures the Power Controller to disable
 * USB-related regulators/etc if no controller is
 * still active (refcounted).
 *
 * @returns 0 on success, negative error code otherwise.
 */
int stm32_usb_pwr_disable(void);

#endif /* ZEPHYR_DRIVERS_USB_COMMON_STM32_STM32_USB_COMMON_H_ */
