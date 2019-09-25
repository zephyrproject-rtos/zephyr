/* Copyright (c) 2019 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <settings/settings.h>
#include "settings_priv.h"

static ssize_t settings_runtime_read_cb(void *cb_arg, void *data, size_t len)
{
	memcpy(data, cb_arg, len);
	return len;
}

int settings_runtime_set(const char *name, void *data, size_t len)
{
	struct settings_handler_static *ch;
	const char *name_key;

	ch = settings_parse_and_lookup(name, &name_key);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_set(name_key, len, settings_runtime_read_cb, data);
}

int settings_runtime_get(const char *name, void *data, size_t len)
{
	struct settings_handler_static *ch;
	const char *name_key;

	ch = settings_parse_and_lookup(name, &name_key);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_get(name_key, data, len);
}

int settings_runtime_commit(const char *name)
{
	struct settings_handler_static *ch;
	const char *name_key;

	ch = settings_parse_and_lookup(name, &name_key);
	if (!ch) {
		return -EINVAL;
	}

	if (ch->h_commit) {
		return ch->h_commit();
	} else {
		return 0;
	}
}
