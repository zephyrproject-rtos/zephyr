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
 * - Bit 10..9: Configure the output speed of a GPIO pin.
 * - Bit 11: GPIO PWR retention in Standby mode (0=Disable, 1=Enable)
 *
 * @ingroup gpio_interface
 * @{
 */

/**
 * Configures a GPIO pin to power on the system after Poweroff.
 * This flag is reserved to GPIO pins that are associated with wake-up pins
 * in STM32 PWR devicetree node, through the property "wkup-gpios".
 */
#define STM32_GPIO_WKUP (1 << 8)

/** @cond INTERNAL_HIDDEN */
#define STM32_GPIO_SPEED_SHIFT 9
#define STM32_GPIO_SPEED_MASK  0x3
/** @endcond */

/** Configure the GPIO pin output speed to be low */
#define STM32_GPIO_LOW_SPEED (0x0 << STM32_GPIO_SPEED_SHIFT)

/** Configure the GPIO pin output speed to be medium */
#define STM32_GPIO_MEDIUM_SPEED (0x1 << STM32_GPIO_SPEED_SHIFT)

/** Configure the GPIO pin output speed to be high */
#define STM32_GPIO_HIGH_SPEED (0x2 << STM32_GPIO_SPEED_SHIFT)

/** Configure the GPIO pin output speed to be very high */
#define STM32_GPIO_VERY_HIGH_SPEED (0x3 << STM32_GPIO_SPEED_SHIFT)

/** @} */

/**
 * @name GPIO PWR retention in Standby mode flags
 * @brief GPIO PWR retention in Standby mode flags
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define STM32_GPIO_PWR_RETENTION_STANDBY_SHIFT  11
#define STM32_GPIO_PWR_RETENTION_STANDBY_MASK   0x0800U
/** @endcond */

/** Disable GPIO PWR retention in Standby mode */
#define STM32_GPIO_PWR_RETENTION_STANDBY_OFF (0U << STM32_GPIO_PWR_RETENTION_STANDBY_SHIFT)
/** Enable GPIO PWR retention in Standby mode */
#define STM32_GPIO_PWR_RETENTION_STANDBY_ON (1U << STM32_GPIO_PWR_RETENTION_STANDBY_SHIFT)
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_STM32_GPIO_H_ */
