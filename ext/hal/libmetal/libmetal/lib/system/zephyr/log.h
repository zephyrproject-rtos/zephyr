/*
 * Copyright (c) 2018, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/log.h
 * @brief	Zephyr libmetal log handler definition.
 */

#ifndef __METAL_METAL_LOG__H__
#error "Include metal/log.h instead of metal/zephyr/log.h"
#endif

#ifndef __METAL_ZEPHYR_LOG__H__
#define __METAL_ZEPHYR_LOG__H__

#ifdef __cplusplus
extern "C" {
#endif

void metal_zephyr_log_handler(enum metal_log_level level,
			      const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __METAL_ZEPHYR_LOG__H__ */
