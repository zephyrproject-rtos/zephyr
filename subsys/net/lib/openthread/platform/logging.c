/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdarg.h>
#include <stdio.h>

#include <openthread/platform/logging.h>

#define LOG_MODULE_NAME net_openthread
#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "platform-zephyr.h"

#define LOG_PARSE_BUFFER_SIZE  128

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion,
	       const char *aFormat, ...)
{
	ARG_UNUSED(aLogRegion);

	char logString[LOG_PARSE_BUFFER_SIZE + 1];
	u16_t length = 0U;

	/* Parse user string. */
	va_list paramList;

	va_start(paramList, aFormat);
	length += vsnprintf(&logString[length],
			    (LOG_PARSE_BUFFER_SIZE - length),
			    aFormat, paramList);
	va_end(paramList);


	switch (aLogLevel) {
	case OT_LOG_LEVEL_CRIT:
		LOG_ERR("%s", logString);
		break;
	case OT_LOG_LEVEL_WARN:
		LOG_WRN("%s", logString);
		break;
	case OT_LOG_LEVEL_INFO:
		LOG_INF("%s", logString);
		break;
	case OT_LOG_LEVEL_DEBG:
		LOG_DBG("%s", logString);
		break;
	default:
		break;
	}
}

