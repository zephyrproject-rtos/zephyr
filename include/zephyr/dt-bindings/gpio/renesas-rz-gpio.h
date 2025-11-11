/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_

/*********************************RZ/A,G,V**************************************/

/**
 * @brief RZ/A,G,V-specific GPIO Flags
 * The pin driving ability flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 * - Bit 8..9: Pin driving ability value
 * - Bit 10: Digital Noise Filter ON/OFF
 * - Bit 11..12: Digital Noise Filter Number value
 * - Bit 13..14: Digital Noise Filter Clock Selection value
 * example:
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZ_GPIO_FILTER_SET(1, 3, 3))>;
 * };
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZ_GPIO_IOLH_SET(2))>;
 * };
 */

/* GPIO drive IOLH */
#define RZ_GPIO_IOLH_SHIFT         8U
#define RZ_GPIO_IOLH_SET(iolh_val) (iolh_val << RZ_GPIO_IOLH_SHIFT)

/* GPIO filter */
#define RZ_GPIO_FILTER_SHIFT    10U
#define RZ_GPIO_FILNUM_SHIFT    1U
#define RZ_GPIO_FILCLKSEL_SHIFT 3U
#define RZ_GPIO_FILTER_SET(fillonoff, filnum, filclksel)                                           \
	(((fillonoff) | ((filnum) << RZ_GPIO_FILNUM_SHIFT) |                                       \
	  ((filclksel) << RZ_GPIO_FILCLKSEL_SHIFT))                                                \
	 << RZ_GPIO_FILTER_SHIFT)

/*******************************************************************************/

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZ_GPIO_H_ */
