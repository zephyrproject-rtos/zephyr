/*
 * Copyright (c) 2018, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	zephyr/log.c
 * @brief	Zephyr libmetal log handler.
 */

#include <stdarg.h>
#include <metal/log.h>
#include <zephyr.h>

static const char *level_strs[] = {
	"metal: emergency: ",
	"metal: alert:     ",
	"metal: critical:  ",
	"metal: error:     ",
	"metal: warning:   ",
	"metal: notice:    ",
	"metal: info:      ",
	"metal: debug:     ",
};

void metal_zephyr_log_handler(enum metal_log_level level,
			      const char *format, ...)
{
	va_list args;

	if (level <= METAL_LOG_EMERGENCY || level > METAL_LOG_DEBUG)
		level = METAL_LOG_EMERGENCY;
	printk("%s", level_strs[level]);

	va_start(args, format);
	vprintk(format, args);
	va_end(args);
}

