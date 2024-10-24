/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(settings_static_data, CONFIG_SETTINGS_LOG_LEVEL);

static ssize_t settings_static_data_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_static_data *static_data =
		(struct settings_static_data *)back_end;
	ssize_t rc = (len > static_data->size) ? -EINVAL : len;

	if (rc > 0) {
		memcpy(data, static_data->data, len);
	}

	return rc;
}

static bool settings_static_data_skip(const struct settings_static_data *data,
				      const struct settings_load_arg *arg)
{
	const size_t slen = ((arg == NULL) || (arg->subtree == NULL)) ?
			    0U : strlen(arg->subtree);

	if (strlen(data->name) < slen) {
		return true;
	}

	if (memcmp(data->name, arg->subtree, slen) != 0) {
		return true;
	}

	return false;
}

void settings_static_data_load(const struct settings_load_arg *arg)
{
	STRUCT_SECTION_FOREACH(settings_static_data, ch) {
		if (settings_static_data_skip(ch, arg)) {
			continue;
		}

		int rc = settings_call_set_handler(ch->name, ch->size,
						   settings_static_data_read_fn,
						   ch, (void *)arg);
		if (rc != 0) {
			LOG_DBG("set failed for %s", ch->name);
		}
	}
}
