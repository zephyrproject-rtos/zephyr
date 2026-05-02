/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2022 Marcin Niestroj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbedtls, CONFIG_MBEDTLS_LOG_LEVEL);

#include "zephyr_mbedtls_priv.h"

void zephyr_mbedtls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
	const char *p, *basename = file;
	int str_len;

	ARG_UNUSED(ctx);

	if (!file || !str) {
		return;
	}

	/* Extract basename from file */
	if (IS_ENABLED(CONFIG_MBEDTLS_DEBUG_EXTRACT_BASENAME_AT_RUNTIME)) {
		for (p = basename = file; *p != '\0'; p++) {
			if (*p == '/' || *p == '\\') {
				basename = p + 1;
			}
		}
	}

	str_len = strlen(str);

	if (IS_ENABLED(CONFIG_MBEDTLS_DEBUG_STRIP_NEWLINE)) {
		/* Remove newline only when it exists */
		if (str_len > 0 && str[str_len - 1] == '\n') {
			str_len--;
		}
	}

	switch (level) {
	case 0:
	case 1:
		LOG_ERR("%s:%04d: %.*s", basename, line, str_len, str);
		break;
	case 2:
		LOG_WRN("%s:%04d: %.*s", basename, line, str_len, str);
		break;
	case 3:
		LOG_INF("%s:%04d: %.*s", basename, line, str_len, str);
		break;
	default:
		LOG_DBG("%s:%04d: %.*s", basename, line, str_len, str);
		break;
	}
}
