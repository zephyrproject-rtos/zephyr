/*
 * Copyright (c) 2019 Antmicro Ltd
 * Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(mg_cry_internal_impl, LOG_LEVEL_DBG);

#include "helper.h"

static void mg_cry_internal_impl(const struct mg_connection *conn,
                                 const char *func,
                                 unsigned line,
                                 const char *fmt,
                                 va_list ap)
{
	(void)conn;

	LOG_ERR("%s @ %d in civetweb.c", STR_LOG_ALLOC(func), line);
	vprintf(fmt, ap);
}
