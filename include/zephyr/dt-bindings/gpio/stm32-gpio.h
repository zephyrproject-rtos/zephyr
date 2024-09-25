/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STM32_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STM32_GPIO_H_

/**
 * @brief STM32 GPIO specific flags
 *
 * The driver flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 *
 * - Bit 8: Configure a GPIO pin to power on the system after Poweroff.
 *
 * @ingroup gpio_interface
 * @{
 */

/**
 * Configures a GPIO pin to power on the system after Poweroff.
 * This flag is reserved to GPIO pins that are associated with wake-up pins
 * in STM32 PWR devicetree node, through the property "wkup-gpios".
 */
#define STM32_GPIO_WKUP		(1 << 8)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STM32_GPIO_H_ */
