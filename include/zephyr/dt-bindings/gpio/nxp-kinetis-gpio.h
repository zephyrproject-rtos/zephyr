/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_KINETIS_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_KINETIS_GPIO_H_

/**
 * @name GPIO drive strength flags
 *
 * The drive strength flags are a Zephyr specific extension of the standard GPIO
 * flags specified by the Linux GPIO binding. Only applicable for NXP Kinetis
 * SoCs.
 *
 * The interface supports two different drive strengths:
 * `DFLT` - The lowest drive strength supported by the HW
 * `ALT` - The highest drive strength supported by the HW
 *
 * @{
 */
/** @cond INTERNAL_HIDDEN */
#define KINETIS_GPIO_DS_POS 9
#define KINETIS_GPIO_DS_MASK (0x3U << KINETIS_GPIO_DS_POS)
/** @endcond */

/** Default drive strength. */
#define KINETIS_GPIO_DS_DFLT (0x0U << KINETIS_GPIO_DS_POS)

/** Alternative drive strength. */
#define KINETIS_GPIO_DS_ALT (0x3U << KINETIS_GPIO_DS_POS)

/** @} */

/**
 * @brief Encodes information when GPIO is used as a wakeup source.
 *
 * @details On some NXP platforms, GPIO can wake up the MCU from low power
 * mode, but the wakeup capability is configured through a specific peripheral,
 * Such as the wakeup unit. We need to use the wakeup unit peripheral to configure
 * which GPIO and which signal can wake up the MCU from low power mode.
 *
 * example:
 * GPIO0_20 can be used as a wake-up source, and the rising edge, falling edge,
 * or both edges on this pin can be selected as the wake-up signal, but these
 * need to be configured through the 4th bitfield (bit8-9) of the WUU PE register,
 * so we need to get these three information in the driver:
 * 1. The gpio pin index signal => 20.
 * 2. The wuu control bit index information => 4.
 * 3. The wake-up signal information => rising/falling/both.
 *
 * We encode these three pieces of information into a 32-bit integer.
 * 1. 8 bit width [bit0-bit7] for gpio pin index.
 * 2. 8 bit width [bit8-bit15] for wuu control bit index.
 * 2. 2 bit width [bit16-bit17] for the wake-up signal.
 *      1 -- Rising edge wakeup.
 *      2 -- Falling edge wakeup.
 *      3 -- Both edge wakeup.
 *
 * Encode result:
 * @code{.c}
 * #define KINETIS_GPIO_20_RISING_EDGE	(20 & 0xFF) | ((4<<8) & 0xFF00) | ((1 << 16) & 0x30000)
 * #define KINETIS_GPIO_20_FALLING_EDGE	(20 & 0xFF) | ((4<<8) & 0xFF00) | ((2 << 16) & 0x30000)
 * #define KINETIS_GPIO_20_ANY_EDGE	(20 & 0xFF) | ((4<<8) & 0xFF00) | ((3 << 16) & 0x30000)
 * @endcode
 *
 * This macro is used to configure the wakeup-line of "nxp,kinetis-gpio",
 * so we need to define the macro as a pure literal value without any operation.
 * So the  final macro definition is as follows:
 * @code{.c}
 * #define KINETIS_GPIO_20_RISING_EDGE	0x10414
 * #define KINETIS_GPIO_20_FALLING_EDGE	0x20414
 * #define KINETIS_GPIO_20_ANY_EDGE	0x30414
 * @endcode
 *
 * Developers can continue to add macros according to their needs.
 */

#define KINETIS_GPIO_20_RISING_EDGE	0x10414
#define KINETIS_GPIO_20_FALLING_EDGE	0x20414
#define KINETIS_GPIO_20_ANY_EDGE	0x30414

/**
 * @brief Decode the gpio wakeup source information.
 *
 * This macro is used in device driver to decode the gpio wakeup source information.
 */
#define KINETIS_GPIO_WAKEUP_SOURCE_DECODE(code)			\
	pin_index = ((code)&0xFFUL);				\
	wuu_reg_index = ((code)&0xFF00UL) >> 8UL;		\
	wakeup_signal_type = ((code)&0x30000UL) >> 16UL;

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_NXP_KINETIS_GPIO_H_ */
