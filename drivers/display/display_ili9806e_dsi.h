/*
 * SPDX-FileCopyrightText: 2024 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9806E_H_
#define ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9806E_H_

#include <zephyr/device.h>

/**
 * @brief Run the ILI9806E power-on command sequence.
 *
 * @param dev ILI9806E device instance
 * @return 0 on success, errno otherwise.
 */
int ili9806e_dsi_regs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9806E_H_ */
