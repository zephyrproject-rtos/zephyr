/*
 * Copyright (c) 2026 Khai Do
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Allwinner Sunxi pinctrl devicetree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SUNXI_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SUNXI_PINCTRL_H_

/** @brief Disable pull-up and pull-down */
#define SUNXI_PULL_NONE          0
/** @brief Enable pull-up */
#define SUNXI_PULL_UP            1
/** @brief Enable pull-down */
#define SUNXI_PULL_DOWN          2
/** @brief Keep current pull state */
#define SUNXI_PULL_KEEP          3

/** @brief Drive strength 10mA */
#define SUNXI_DRIVE_10MA         0
/** @brief Drive strength 20mA */
#define SUNXI_DRIVE_20MA         1
/** @brief Drive strength 30mA */
#define SUNXI_DRIVE_30MA         2
/** @brief Drive strength 40mA */
#define SUNXI_DRIVE_40MA         3
/** @brief Keep current drive strength */
#define SUNXI_DRV_KEEP           4

/**
 * @brief Construct a Sunxi pinmux configuration
 * @param port Port ('A'-'Z')
 * @param pin Pin (0-31)
 * @param mux Mux (0-15)
 */
#define SUNXI_PINMUX(port, pin, mux) \
	(((pin) & 0xFF) | \
	((((port) - 'A') & 0xFF) << 8) | \
	(((mux) & 0xF) << 16))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_SUNXI_PINCTRL_H_ */
