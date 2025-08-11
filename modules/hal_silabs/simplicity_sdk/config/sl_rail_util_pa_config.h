/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the HAL driver rail_util_pa from hal_silabs,
 * invoked through the init function of the hci_silabs_efr32 Bluetooth driver.
 * DeviceTree and Kconfig options are converted to config macros expected by the HAL driver.
 */

#ifndef SL_RAIL_UTIL_PA_CONFIG_H
#define SL_RAIL_UTIL_PA_CONFIG_H

#include <zephyr/devicetree.h>
#include "rail_types.h"

#define SL_RAIL_UTIL_PA_POWER_DECI_DBM (DT_PROP(DT_NODELABEL(radio), pa_initial_power_dbm) * 10)
#define SL_RAIL_UTIL_PA_RAMP_TIME_US   DT_PROP(DT_NODELABEL(radio), pa_ramp_time_us)
#define SL_RAIL_UTIL_PA_VOLTAGE_MV     DT_PROP(DT_NODELABEL(radio), pa_voltage_mv)

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), pa_2p4ghz)
#define SL_RAIL_UTIL_PA_SELECTION_2P4GHZ                                                           \
	CONCAT(RAIL_TX_POWER_MODE_2P4GIG_, DT_STRING_UPPER_TOKEN(DT_NODELABEL(radio), pa_2p4ghz))
#else
#define SL_RAIL_UTIL_PA_SELECTION_2P4GHZ RAIL_TX_POWER_MODE_NONE
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(radio), pa_subghz)
#define SL_RAIL_UTIL_PA_SELECTION_SUBGHZ                                                           \
	CONCAT(RAIL_TX_POWER_MODE_SUBGIG_, DT_STRING_UPPER_TOKEN(DT_NODELABEL(radio), pa_subghz))
#else
#define SL_RAIL_UTIL_PA_SELECTION_SUBGHZ RAIL_TX_POWER_MODE_NONE
#endif

#define SL_RAIL_UTIL_PA_CURVE_HEADER       CONFIG_SILABS_SISDK_RAIL_PA_CURVE_HEADER
#define SL_RAIL_UTIL_PA_CURVE_TYPES        CONFIG_SILABS_SISDK_RAIL_PA_CURVE_TYPES_HEADER
#define SL_RAIL_UTIL_PA_CALIBRATION_ENABLE CONFIG_SILABS_SISDK_RAIL_PA_ENABLE_CALIBRATION

#endif /* SL_RAIL_UTIL_PA_CONFIG_H */
