/**
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_LOG_H_
#define ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_MODULE_NAME wifi_simplelink
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL

#include <logging/log.h>

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_WIFI_SIMPLELINK_SIMPLELINK_LOG_H_ */
