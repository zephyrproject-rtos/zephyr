/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEMO_RANGING_CONFIG_H__
#define __DEMO_RANGING_CONFIG_H__

#include <zephyr/uwb/types.h>

/** Define if demo supplied configuration should be used */
#define DEMO_APP_CONFIGS demo_app_configs

static uwb_config_t demo_app_configs[] = {
	{UWB_APP_CONFIG_RANGING_ROUND_USAGE, 1, ARR(2), 0},
	{UWB_APP_CONFIG_AOA_RESULT_REQ, 1, ARR(0), 0},
	{UWB_APP_CONFIG_PREAMBLE_CODE_INDEX, 1, ARR(10), 0},
	{UWB_APP_CONFIG_PREAMBLE_DURATION, 1, ARR(1), 0},
	{UWB_APP_CONFIG_PSDU_DATA_RATE, 1, ARR(0), 0},
	{UWB_APP_CONFIG_PRF_MODE, 1, ARR(0), 0},
	{UWB_APP_CONFIG_BPRF_PHR_DATA_RATE, 1, ARR(0), 0},
	{UWB_APP_CONFIG_STS_LENGTH, 1, ARR(1), 0},
};

#endif /** __DEMO_RANGING_CONFIG_H__ */
