/*
 * Copyright (c) 2018 - 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdarg.h>
#include <stdio.h>

#include <openthread/platform/logging.h>
#include "openthread-core-zephyr-config.h"

#define LOG_MODULE_NAME net_openthread
#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "platform-zephyr.h"

/* Convert OT log level to zephyr log level. */
static inline int log_translate(otLogLevel aLogLevel)
{
	switch (aLogLevel) {
	case OT_LOG_LEVEL_NONE:
	case OT_LOG_LEVEL_CRIT:
		return LOG_LEVEL_ERR;
	case OT_LOG_LEVEL_WARN:
		return LOG_LEVEL_WRN;
	case OT_LOG_LEVEL_NOTE:
	case OT_LOG_LEVEL_INFO:
		return LOG_LEVEL_INF;
	case OT_LOG_LEVEL_DEBG:
		return LOG_LEVEL_DBG;
	default:
		break;
	}

	return -1;
}

void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
	ARG_UNUSED(aLogRegion);

#if defined(CONFIG_LOG)
	int level = log_translate(aLogLevel);
	va_list param_list;

	if (level < 0) {
		return;
	}

	va_start(param_list, aFormat);
	log2_generic(level, aFormat, param_list);
	va_end(param_list);
#else
	ARG_UNUSED(aLogLevel);
	ARG_UNUSED(aFormat);
#endif

}
