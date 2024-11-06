/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_

/*********************************RZG3S*****************************************/

/**
 * @brief RZ G3S specific GPIO Flags
 * The pin driving ability flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 * - Bit 9..8: Pin driving ability value
 * - Bit 11..10: Digital Noise Filter Clock Selection value
 * - Bit 13..12: Digital Noise Filter Number value
 * - Bit 14: Digital Noise Filter ON/OFF
 * example:
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZG3S_GPIO_FILTER_SET(1, 3, 3))>;
 * };
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZG3S_GPIO_IOLH_SET(2))>;
 * };
 */

/* GPIO drive IOLH */
#define RZG3S_GPIO_IOLH_SHIFT         7U
#define RZG3S_GPIO_IOLH_SET(iolh_val) (iolh_val << RZG3S_GPIO_IOLH_SHIFT)

/* GPIO filter */
#define RZG3S_GPIO_FILTER_SHIFT    9U
#define RZG3S_GPIO_FILNUM_SHIFT    1U
#define RZG3S_GPIO_FILCLKSEL_SHIFT 3U
#define RZG3S_GPIO_FILTER_SET(fillonoff, filnum, filclksel)                                        \
	(((fillonoff) | ((filnum) << RZG3S_GPIO_FILNUM_SHIFT) |                                    \
	  ((filclksel) << RZG3S_GPIO_FILCLKSEL_SHIFT))                                             \
	 << RZG3S_GPIO_FILTER_SHIFT)

/*******************************************************************************/

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_ */
