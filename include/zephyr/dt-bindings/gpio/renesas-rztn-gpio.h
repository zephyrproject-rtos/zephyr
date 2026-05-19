/*
 * Copyright (c) 2025-2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RZ/T,N GPIO flags for Zephyr.
 *
 *  The additional flags are encoded in the 8 upper bits of @ref gpio_dt_flags_t as follows:
 * - bits [9:8] = Driving ability control (0=Low, 1=Middle, 2=High, 3=Ultral High)
 * - bits [12] = Schmitt trigger control (0=Disable, 1=Enable)
 * - bits [13] = Slew rate control (0=Slow, 1=Fast)
 * - bits [14] = Safety region (0=Safety, 1=Non safety)
 *
 * Example DT usage:
 * - Driving ability control: Middle
 * - Schmitt trigger control: Enabled
 * - Slew rate control: Slow
 * - Non safety
 *
 * @code{.dts}
 * gpio-consumer {
 *     out-gpios = <&port8 2 (GPIO_PULL_UP | RZTN_GPIO_CFG_SET(1, 1, 0) | RZTN_GPIO_NONSAFETY)>;
 * };
 * @endcode
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZTN_GPIO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZTN_GPIO_H_

/**
 * @name  Renesas RZ/T,N Region Select Options
 *
 * Set the safety attribute for I/O ports (RSELP)
 * @{
 */

#define RZTN_GPIO_SAFETY      0U /**< Safety region */
#define RZTN_GPIO_NONSAFETY   BIT(14) /**< Non safety region */

/** @} */

/**
 * @brief Encode settings for DRCTL register into one configuration value
 *
 * Encodings:
 * - bits [9:8] = Driving ability control (0=Low, 1=Middle, 2=High, 3=Ultra High)
 * - bits [12] = Schmitt trigger control (0=Disable, 1=Enable)
 * - bits [13] = Slew rate control (0=Slow, 1=Fast)
 *
 * @param drive_ability Driving ability control (0=Low, 1=Middle, 2=High, 3=Ultral High)
 * @param schmitt_trig Schmitt trigger control (0=Disable, 1=Enable)
 * @param slew_rate Slew rate control (0=Slow, 1=Fast)
 *
 * @return Encoded configuration value for DRCTL register
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/* GPIO DRCTL register */
#define RZTN_GPIO_DRCTL_SHIFT        8U /**< DRCTL shift within gpio_dt_flags_t */
#define RZTN_GPIO_SCHMITT_TRIG_SHIFT 4U /**< Schmitt trigger control shift within DRCTL field */
#define RZTN_GPIO_SLEW_RATE_SHIFT    5U /**< Slew rate control shift within DRCTL field */
/** @endcond */

#define RZTN_GPIO_DRCTL_SET(drive_ability, schmitt_trig, slew_rate)                                \
	(((drive_ability) | ((schmitt_trig) << RZTN_GPIO_SCHMITT_TRIG_SHIFT) |                     \
	  ((slew_rate) << RZTN_GPIO_SLEW_RATE_SHIFT))                                              \
	 << RZTN_GPIO_DRCTL_SHIFT)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_GPIO_RENESAS_RZTN_GPIO_H_ */
