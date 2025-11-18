/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(getopt, CONFIG_GETOPT_LOG_LEVEL);

int getsubopt(char **optionp, char *const *keylist, char **valuep)
{
	size_t i, len;
	char *begin, *end, *eq, *key, *val;

	if (optionp == NULL || keylist == NULL) {
		LOG_DBG("optionp or keylist is NULL");
		return -1;
	}

	if (valuep != NULL) {
		*valuep = NULL;
	}

	for (i = 0, begin = *optionp, end = begin, eq = NULL, len = strlen(*optionp); i <= len;
	     ++i, ++end) {

		char c = *end;

		if ((c == '=') && (eq == NULL)) {
			eq = end;
		} else if (c == ',') {
			*end = '\0';
			*optionp = end + 1;
			break;
		} else if (c == '\0') {
			*optionp = end;
			break;
		}
	}

	key = begin;
	if (eq == NULL) {
		len = end - begin;
		if (valuep != NULL) {
			*valuep = NULL;
		}
		LOG_DBG("key: %.*s", (int)len, key);
	} else {
		len = eq - begin;
		val = eq + 1;
		if (valuep != NULL) {
			*valuep = val;
		}
		LOG_DBG("key: %.*s val: %.*s", (int)len, key, (int)(end - val), val);
	}

	if (len == 0) {
		LOG_DBG("zero-length key");
		return -1;
	}

	for (i = 0; true; ++i) {
		if (keylist[i] == NULL) {
			LOG_DBG("end of keylist of length %zu reached", i);
			break;
		}

		if (strncmp(key, keylist[i], len) != 0) {
			continue;
		}

		if (keylist[i][len] != '\0') {
			LOG_DBG("key %.*s does not match keylist[%zu]: %s", (int)len, key, i,
				keylist[i]);
			continue;
		}

		return (int)i;
	}

	return -1;
}
