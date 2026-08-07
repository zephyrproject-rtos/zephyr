/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the HAL driver sl_rail_util_rf_path_switch
 * from hal_silabs, invoked through SoC init when radio functionality is enabled.
 * DeviceTree and Kconfig options are converted to config macros expected by the HAL driver.
 */

#ifndef SL_RAIL_UTIL_RF_PATH_SWITCH_CONFIG_H
#define SL_RAIL_UTIL_RF_PATH_SWITCH_CONFIG_H

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/gpio/gpio.h>

#define SL_RAIL_UTIL_RF_PATH_SWITCH_RADIO_ACTIVE_DISABLE 0
#define SL_RAIL_UTIL_RF_PATH_SWITCH_RADIO_ACTIVE_COMBINE 1
#define SL_RAIL_UTIL_RF_PATH_SWITCH_RADIO_ACTIVE_MODE                                              \
	DT_PROP(DT_NODELABEL(radio), rf_path_switch_combine)

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), rf_path_switch_radio_active_gpios)
#define SL_RAIL_UTIL_RF_PATH_SWITCH_RADIO_ACTIVE_PORT                                              \
	DT_NODE_CHILD_IDX(DT_GPIO_CTLR(DT_NODELABEL(radio), rf_path_switch_radio_active_gpios))
#define SL_RAIL_UTIL_RF_PATH_SWITCH_RADIO_ACTIVE_PIN                                               \
	DT_GPIO_PIN(DT_NODELABEL(radio), rf_path_switch_radio_active_gpios)
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), rf_path_switch_control_gpios)
#if DT_GPIO_FLAGS(DT_NODELABEL(radio), rf_path_switch_control_gpios) & GPIO_ACTIVE_LOW
#define SL_RAIL_UTIL_RF_PATH_SWITCH_CONTROL_PORT                                                   \
	DT_NODE_CHILD_IDX(DT_GPIO_CTLR(DT_NODELABEL(radio), rf_path_switch_control_gpios))
#define SL_RAIL_UTIL_RF_PATH_SWITCH_CONTROL_PIN                                                    \
	DT_GPIO_PIN(DT_NODELABEL(radio), rf_path_switch_control_gpios)
#else
#define SL_RAIL_UTIL_RF_PATH_SWITCH_INVERTED_CONTROL_PORT                                          \
	DT_NODE_CHILD_IDX(DT_GPIO_CTLR(DT_NODELABEL(radio), rf_path_switch_control_gpios))
#define SL_RAIL_UTIL_RF_PATH_SWITCH_INVERTED_CONTROL_PIN                                           \
	DT_GPIO_PIN(DT_NODELABEL(radio), rf_path_switch_control_gpios)
#endif
#endif

#endif /* SL_RAIL_UTIL_RF_PATH_SWITCH_CONFIG_H */
