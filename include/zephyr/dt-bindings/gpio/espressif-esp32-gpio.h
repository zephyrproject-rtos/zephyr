/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ESPRESSIF_ESP32_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ESPRESSIF_ESP32_GPIO_H_

/**
 * @name GPIO drive strength flags
 *
 * The drive strength flags are a Zephyr specific extension of the standard GPIO
 * flags specified by the Linux GPIO binding. Only applicable for Espressif
 * ESP32 SoCs.
 *
 * The interface supports two different drive strengths:
 * `DFLT` - The highest drive strength supported by the HW
 * `ALT` - The lowest drive strength supported by the HW
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define ESP32_GPIO_DS_POS 9
#define ESP32_GPIO_DS_MASK (0x3U << ESP32_GPIO_DS_POS)
/** @endcond */

/** Default drive strength. */
#define ESP32_GPIO_DS_DFLT (0x0U << ESP32_GPIO_DS_POS)

/** Alternative drive strength. */
#define ESP32_GPIO_DS_ALT (0x3U << ESP32_GPIO_DS_POS)

/** @} */

/**
 * @name GPIO pin input/output enable flags
 *
 * These flags allow configuring a pin as input or output while keeping untouched
 * its complementary configuration. By instance, if we configure a GPIO pin as an
 * input and pass the flag ESP32_GPIO_PIN_OUT_EN, the driver will not disable the
 * pin's output buffer. This functionality can be useful to render a pin both an
 * input and output, for diagnose or testing purposes.
 *
 * @{
 */

/** Keep GPIO pin enabled as output */
#define ESP32_GPIO_PIN_OUT_EN (1 << 12)

/** Keep GPIO pin enabled as input */
#define ESP32_GPIO_PIN_IN_EN (1 << 13)

/** @} */

/**
 * @name GPIO sleep hold flag
 *
 * Enable the ESP GPIO pad hold feature for a pin during system sleep.
 *
 * When enabled, the pin state is latched right before the system enters
 * a low power mode, and remains unchanged while the system is sleeping.
 * This ensures the GPIO level is retained even if the peripheral
 * controlling the pin is powered down or its registers lose state.
 *
 * This function works in both input and output modes and is only
 * applicable to output-capable GPIOs.
 *
 * Note: For deep sleep cycles, the automatic hold functionality may not
 * cover the entire period from entering sleep until the application
 * resumes execution. During deep sleep, register states are lost due to
 * reset, and the hold configuration will be released during the driver
 * initialization phase. For outputs where maintaining a stable level is
 * critical (for example, edge-sensitive signals), the application may
 * need to explicitly enable the hold feature using gpio_hold_en() before
 * requesting a power-off. This ensures that the pin state remains held
 * throughout the deep sleep period and until the application is running
 * again.
 *
 * @{
 */

/** Enable GPIO state hold during sleep */
#define ESP32_GPIO_SLEEP_HOLD_EN (1 << 14)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ESPRESSIF_ESP32_GPIO_H_ */
