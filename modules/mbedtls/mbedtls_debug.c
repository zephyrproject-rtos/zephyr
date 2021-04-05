/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2021 Emil Lindqvist
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#include "mbedtls_debug.h"

LOG_MODULE_REGISTER(mbedtls, CONFIG_MBEDTLS_DEBUG_LEVEL);

static void debug_func(void *ctx, int level, const char *file,
		int line, const char *str)
{
	const char *p, *basename;

	ARG_UNUSED(ctx);

	if (!file || !str) {
		return;
	}

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

#define LOG_ARGS "%s:%04d: %s", basename, line, log_strdup(str)
	switch (level) {
	case 1:
		LOG_ERR(LOG_ARGS);
		break;
	case 2:
		LOG_WRN(LOG_ARGS);
		break;
	case 3:
		LOG_INF(LOG_ARGS);
		break;
	case 4:
		LOG_DBG(LOG_ARGS);
		break;
	default:
		LOG_ERR("mbedTLS invalid debug level (%d)", level);
		break;
	}
}

void mbedtls_debug_init(mbedtls_ssl_config *config)
{
	mbedtls_ssl_conf_dbg(config, debug_func, NULL);
}
