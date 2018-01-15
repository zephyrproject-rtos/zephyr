/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdarg.h>
#include <stdio.h>

#include <openthread/platform/logging.h>

#define SYS_LOG_DOMAIN "openthread"
#define SYS_LOG_LEVEL 4
#include <logging/sys_log.h>

#include "platform-zephyr.h"

#define LOG_PARSE_BUFFER_SIZE  128

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion,
	       const char *aFormat, ...)
{
	ARG_UNUSED(aLogRegion);

	char logString[LOG_PARSE_BUFFER_SIZE + 1];
	u16_t length = 0;

	/* Parse user string. */
	va_list paramList;

	va_start(paramList, aFormat);
	length += vsnprintf(&logString[length],
			    (LOG_PARSE_BUFFER_SIZE - length),
			    aFormat, paramList);
	va_end(paramList);


	switch (aLogLevel) {
	case OT_LOG_LEVEL_CRIT:
		SYS_LOG_ERR("%s", logString);
		break;
	case OT_LOG_LEVEL_WARN:
		SYS_LOG_WRN("%s", logString);
		break;
	case OT_LOG_LEVEL_INFO:
		SYS_LOG_INF("%s", logString);
		break;
	case OT_LOG_LEVEL_DEBG:
		SYS_LOG_DBG("%s", logString);
		break;
	default:
		break;
	}
}

