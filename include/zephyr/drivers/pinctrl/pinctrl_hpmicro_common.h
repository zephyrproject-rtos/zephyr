/*
 * Copyright (c) 2022-2025 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HPMicro pinctrl pin encoding helpers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_HPMICRO_COMMON_H_
#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_HPMICRO_COMMON_H_

/** @cond INTERNAL_HIDDEN */

/** @brief Extract the PAD index from an encoded pinmux value. */
#define HPMICRO_PAD_NUM(_pinmux) (((_pinmux) >> HPMICRO_PIN_NUM_SHIFT) & HPMICRO_PIN_NUM_MASK)

/** @brief Extract the analog-enable field from an encoded pinmux value. */
#define HPMICRO_FUNC_ANALOG(_pinmux)                                                               \
	(((_pinmux) >> HPMICRO_PIN_ANALOG_SHIFT) & HPMICRO_PIN_ANALOG_MASK)

/** @brief Extract the alternate-function field from an encoded pinmux value. */
#define HPMICRO_FUNC_ALT_SELECT(_pinmux)                                                           \
	(((_pinmux) >> HPMICRO_PIN_ALT_SHIFT) & HPMICRO_PIN_ALT_MASK)

/** @brief Extract the IOC instance selector from an encoded pinmux value. */
#define HPMICRO_FUNC_IOC_SELECT(_pinmux)                                                           \
	(((_pinmux) >> HPMICRO_PIN_IOC_SHIFT) & HPMICRO_PIN_IOC_MASK)

/** @brief Extract the loopback-enable field from encoded pin configuration. */
#define HPMICRO_FUNC_LOOPBACK(_pincfg)                                                             \
	(((_pincfg) >> HPMICRO_FORCE_INPUT_SHIFT) & HPMICRO_FORCE_INPUT_MASK)

/** @brief Extract the open-drain field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_OD(_pincfg)                                                                \
	(((_pincfg) >> HPMICRO_OPEN_DRAIN_SHIFT) & HPMICRO_OPEN_DRAIN_MASK)

/** @brief Extract the Schmitt-trigger field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_HYS(_pincfg)                                                               \
	(((_pincfg) >> HPMICRO_SCHMITT_ENABLE_SHIFT) & HPMICRO_SCHMITT_ENABLE_MASK)

/** @brief Extract the pull-up field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_PULL_UP(_pincfg)                                                           \
	(((_pincfg) >> HPMICRO_PULL_UP_SHIFT) & HPMICRO_PULL_UP_MASK)

/** @brief Extract the pull-down field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_PULL_DN(_pincfg)                                                           \
	(((_pincfg) >> HPMICRO_PULL_DOWN_SHIFT) & HPMICRO_PULL_DOWN_MASK)

/** @brief Extract the pull-select value from encoded pin configuration. */
#define HPMICRO_PAD_CTL_PS(_pincfg) HPMICRO_PAD_CTL_PULL_UP(_pincfg)

/** @brief Extract the pull-enable value from encoded pin configuration. */
#define HPMICRO_PAD_CTL_PE(_pincfg)                                                                \
	(!(((_pincfg) >> HPMICRO_NO_PULL_SHIFT) & HPMICRO_NO_PULL_MASK) &&                         \
	 (HPMICRO_PAD_CTL_PULL_UP(_pincfg) ^ HPMICRO_PAD_CTL_PULL_DN(_pincfg)))

/** @brief Extract the drive-strength field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_DS(_pincfg)                                                                \
	(((_pincfg) >> HPMICRO_DRIVER_STRENGTH_SHIFT) & HPMICRO_DRIVER_STRENGTH_MASK)

/** @brief Extract the frequency slew-rate field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_SPD(_pincfg) (((_pincfg) >> HPMICRO_SLEW_PD_SHIFT) & HPMICRO_SLEW_PD_MASK)

/** @brief Extract the pull-resistor strength field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_PRS(_pincfg)                                                               \
	(((_pincfg) >> HPMICRO_PULL_STRENGTH_SHIFT) & HPMICRO_PULL_STRENGTH_MASK)

/** @brief Extract the output slew-rate field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_SR(_pincfg)                                                                \
	(((_pincfg) >> HPMICRO_SLEW_RATE_SHIFT) & HPMICRO_SLEW_RATE_MASK)

/** @brief Extract the keeper-disable field from encoded pin configuration. */
#define HPMICRO_PAD_CTL_KE_DISABLE(_pincfg)                                                        \
	(((_pincfg) >> HPMICRO_KEEPER_DISABLE_SHIFT) & HPMICRO_KEEPER_DISABLE_MASK)

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_PINCTRL_PINCTRL_HPMICRO_COMMON_H_ */
