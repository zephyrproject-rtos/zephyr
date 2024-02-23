/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SL_DEVICE_INIT_HFXO_CONFIG_H
#define SL_DEVICE_INIT_HFXO_CONFIG_H

#include <zephyr/devicetree.h>

#define SL_DEVICE_INIT_HFXO_MODE           cmuHfxoOscMode_Crystal
#define SL_DEVICE_INIT_HFXO_FREQ           DT_PROP(DT_NODELABEL(clk_hfxo), clock_frequency)
#define SL_DEVICE_INIT_HFXO_CTUNE          DT_PROP(DT_NODELABEL(clk_hfxo), ctune)
#define SL_DEVICE_INIT_HFXO_PRECISION      DT_PROP(DT_NODELABEL(clk_hfxo), precision)

#endif /* SL_DEVICE_INIT_HFXO_CONFIG_H */
