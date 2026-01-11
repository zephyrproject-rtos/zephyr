/*
 * Copyright (C) 2025 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for strtok_r() */
#include <string.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080_utils, CONFIG_MODEM_LOG_LEVEL);

#include "sim7080.h"

int sim7080_utils_parse_time(uint8_t *date, uint8_t *time_str, struct tm *t)
{
	char *saveptr;
	int ret = -1;

	if (!date || !time_str || !t) {
		ret = -EINVAL;
		goto out;
	}

	memset(t, 0, sizeof(*t));

	char *tmp = strtok_r(date, "/", &saveptr);

	if (tmp == NULL) {
		LOG_WRN("Failed to parse year");
		goto out;
	}

	t->tm_year = (int)strtol(tmp, NULL, 10) - 1900;

	tmp = strtok_r(NULL, "/", &saveptr);
	if (tmp == NULL) {
		LOG_WRN("Failed to parse month");
		goto out;
	}

	t->tm_mon = (int)strtol(tmp, NULL, 10) - 1;

	tmp = strtok_r(NULL, "", &saveptr);
	if (tmp == NULL) {
		LOG_WRN("Failed to parse day");
		goto out;
	}

	t->tm_mday = (int)strtol(tmp, NULL, 10);

	tmp = strtok_r(time_str, ":", &saveptr);
	if (tmp == NULL) {
		LOG_WRN("Failed to parse hour");
		goto out;
	}

	t->tm_hour = (int)strtol(tmp, NULL, 10);

	tmp = strtok_r(NULL, ":", &saveptr);
	if (tmp == NULL) {
		LOG_WRN("Failed to parse minute");
		goto out;
	}

	t->tm_min = (int)strtol(tmp, NULL, 10);

	tmp = strtok_r(NULL, "+", &saveptr);
	if (tmp == NULL) {
		LOG_WRN("Failed to parse second");
		goto out;
	}

	t->tm_sec = (int)strtol(tmp, NULL, 10);

	/* Mark dst as not available */
	t->tm_isdst = -1;

	ret = 0;
out:
	return ret;
}
