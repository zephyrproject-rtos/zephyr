/*
 * Copyright (c) 2021 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MODULES_CHRE_TARGET_PLATFORM_H_
#define MODULES_CHRE_TARGET_PLATFORM_H_

#include <logging/log.h>
LOG_MODULE_DECLARE(chre, CONFIG_CHRE_LOG_LEVEL);

/** Map CHRE's LOGE to Zephyr's LOG_ERR */
#define LOGE(format, ...) LOG_ERR(format, __VA_ARGS__)
/** Map CHRE's LOGW to Zephyr's LOG_WRN */
#define LOGW(format, ...) LOG_WRN(format, __VA_ARGS__)
/** Map CHRE's LOGI to Zephyr's LOG_INF */
#define LOGI(format, ...) LOG_INF(format, __VA_ARGS__)
/** Map CHRE's LOGD to Zephyr's LOG_DBG */
#define LOGD(format, ...) LOG_DBG(format, __VA_ARGS__)

#endif /* MODULES_CHRE_TARGET_PLATFORM_H_ */
