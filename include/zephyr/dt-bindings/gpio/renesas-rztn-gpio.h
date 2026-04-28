/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZTN_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZTN_GPIO_H_

/*********************************RZTN*****************************************/

/**
 * @brief RZTN specific GPIO Flags
 * The pin driving ability flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as
 * follows:
 * - Bit 9..8: Driving ability control
 * - Bit 12: Schmitt trigger control
 * - Bit 13: Slew rate control
 * Example:
 * Driving ability control: Middle
 * Schmitt trigger control: Enabled
 * Slew rate control: Slow
 * gpio-consumer {
 *	out-gpios = <&port8 2 (GPIO_PULL_UP | RZTN_GPIO_CFG_SET(1, 1, 0))>;
 * };
 */

/* GPIO DRCTL register */
#define RZTN_GPIO_DRCTL_SHIFT        8U
#define RZTN_GPIO_SCHMITT_TRIG_SHIFT 4U
#define RZTN_GPIO_SLEW_RATE_SHIFT    5U
#define RZTN_GPIO_DRCTL_SET(drive_ability, schmitt_trig, slew_rate)                                \
	(((drive_ability) | ((schmitt_trig) << RZTN_GPIO_SCHMITT_TRIG_SHIFT) |                     \
	  ((slew_rate) << RZTN_GPIO_SLEW_RATE_SHIFT))                                              \
	 << RZTN_GPIO_DRCTL_SHIFT)

/*******************************************************************************/

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZTN_GPIO_H_ */
