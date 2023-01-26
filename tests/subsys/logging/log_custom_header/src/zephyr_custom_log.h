/*
 * Copyright (c) 2022 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_CUSTOM_LOG_H__
#define __ZEPHYR_CUSTOM_LOG_H__

#define CUSTOM_LOG_PREFIX "[CUSTOM_PREFIX]"

#undef LOG_DBG
#undef LOG_INF
#undef LOG_WRN
#undef LOG_ERR

#define LOG_DBG(...) Z_LOG(LOG_LEVEL_DBG, CUSTOM_LOG_PREFIX __VA_ARGS__)
#define LOG_INF(...) Z_LOG(LOG_LEVEL_INF, CUSTOM_LOG_PREFIX __VA_ARGS__)
#define LOG_WRN(...) Z_LOG(LOG_LEVEL_WRN, CUSTOM_LOG_PREFIX __VA_ARGS__)
#define LOG_ERR(...) Z_LOG(LOG_LEVEL_ERR, CUSTOM_LOG_PREFIX __VA_ARGS__)

#endif /* __ZEPHYR_CUSTOM_LOG_H__ */
