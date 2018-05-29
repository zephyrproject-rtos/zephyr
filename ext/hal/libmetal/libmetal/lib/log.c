/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdarg.h>
#include <stdio.h>

#include <metal/log.h>
#include <metal/sys.h>

void metal_default_log_handler(enum metal_log_level level,
			       const char *format, ...)
{
#ifdef DEFAULT_LOGGER_ON
	char msg[1024];
	va_list args;
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

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	if (level <= METAL_LOG_EMERGENCY || level > METAL_LOG_DEBUG)
		level = METAL_LOG_EMERGENCY;

	fprintf(stderr, "%s%s", level_strs[level], msg);
#else
	(void)level;
	(void)format;
#endif
}

void metal_set_log_handler(metal_log_handler handler)
{
	_metal.common.log_handler = handler;
}

metal_log_handler metal_get_log_handler(void)
{
	return _metal.common.log_handler;
}

void metal_set_log_level(enum metal_log_level level)
{
	_metal.common.log_level = level;
}

enum metal_log_level metal_get_log_level(void)
{
	return _metal.common.log_level;
}
