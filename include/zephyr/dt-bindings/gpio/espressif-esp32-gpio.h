/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
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
 * `DFLT` - The lowest drive strength supported by the HW
 * `ALT` - The highest drive strength supported by the HW
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

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_ESPRESSIF_ESP32_GPIO_H_ */
