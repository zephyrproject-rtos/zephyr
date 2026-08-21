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
	{kUwb_AppConfig_RangingRoundUsage, 1, ARR(2), 0},
	{kUwb_AppConfig_AoaResultReq, 1, ARR(0), 0},
	{kUwb_AppConfig_PreambleCodeIndex, 1, ARR(10), 0},
	{kUwb_AppConfig_PreambleDuration, 1, ARR(1), 0},
	{kUwb_AppConfig_PsduDataRate, 1, ARR(0), 0},
	{kUwb_AppConfig_PrfMode, 1, ARR(0), 0},
	{kUwb_AppConfig_BprfPhrDataRate, 1, ARR(0), 0},
	{kUwb_AppConfig_StsLength, 1, ARR(1), 0},
};

#endif /** __DEMO_RANGING_CONFIG_H__ */
