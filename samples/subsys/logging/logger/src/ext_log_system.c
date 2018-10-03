/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "ext_log_system.h"

static ext_log_handler log_handler;

void ext_log_handler_set(ext_log_handler handler)
{
	log_handler = handler;
}

void ext_log_system_foo(void)
{
	ext_log(EXT_LOG_CRITICAL, "critical level log");

	ext_log(EXT_LOG_ERROR, "error level log, 1 arguments: %d", 1);

	ext_log(EXT_LOG_WARNING, "warning level log, 2 arguments: %d %d", 1, 2);

	ext_log(EXT_LOG_NOTICE, "notice level log, 3 arguments: %d, %s, 0x%08x",
							100, "string", 0x255);

	ext_log(EXT_LOG_INFO, "info level log, 4 arguments : %d %d %d %d",
								1, 2, 3, 4);

	ext_log(EXT_LOG_DEBUG, "debug level log, 5 arguments: %d %d %d %d %d",
								1, 2, 3, 4, 5);
}
