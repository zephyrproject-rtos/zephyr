/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This configuration header is used by the HAL driver sl_rail_util_sequencer
 * from hal_silabs when radio functionality is enabled.
 * DeviceTree and Kconfig options are converted to config macros expected by the HAL driver.
 */

#ifndef SL_RAIL_UTIL_SEQUENCER_CONFIG_H
#define SL_RAIL_UTIL_SEQUENCER_CONFIG_H

#include <zephyr/devicetree.h>

#if CONFIG_SOC_SILABS_XG24 || CONFIG_SOC_SILABS_XG26
#define SL_RAIL_UTIL_SEQUENCER_RUNTIME_IMAGE_SELECTION 0

#if DT_PROP(DT_NODELABEL(radio), pa_max_power_dbm) > 10
#define SL_RAIL_UTIL_SEQUENCER_IMAGE RAIL_SEQ_IMAGE_PA_20_DBM
#else
#define SL_RAIL_UTIL_SEQUENCER_IMAGE RAIL_SEQ_IMAGE_PA_10_DBM
#endif

#else
#define SL_RAIL_UTIL_SEQUENCER_RUNTIME_IMAGE_SELECTION 1
#endif

#endif /* SL_RAIL_UTIL_SEQUENCER_CONFIG_H */
