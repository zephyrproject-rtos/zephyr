/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_CUSTOM_LOG_H__
#define __ZEPHYR_CUSTOM_LOG_H__

#include <zephyr/ztest.h>

#define CUSTOM_LOG_PREFIX "[DUT] "

#undef LOG_DBG
#undef LOG_INF
#undef LOG_WRN
#undef LOG_ERR

#define LOG_HELPER(fmt, ...) TC_PRINT(fmt "\n", ##__VA_ARGS__)
#define LOG_DBG(...)         LOG_HELPER(CUSTOM_LOG_PREFIX "<dbg> " __VA_ARGS__)
#define LOG_INF(...)         LOG_HELPER(CUSTOM_LOG_PREFIX "<inf> " __VA_ARGS__)
#define LOG_WRN(...)         LOG_HELPER(CUSTOM_LOG_PREFIX "<wrn> " __VA_ARGS__)
#define LOG_ERR(...)         LOG_HELPER(CUSTOM_LOG_PREFIX "<err> " __VA_ARGS__)

#endif /* __ZEPHYR_CUSTOM_LOG_H__ */
