/* Copyright (c) 2019 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <nsettings/settings.h>
#include "settings_priv.h"

static ssize_t settings_runtime_read_fn(void *data, size_t len,
					void *read_fn_arg)
{
	memcpy(data, read_fn_arg, len);
	return len;
}

int settings_runtime_set(const char *name, void *data, size_t len)
{
	struct settings_handler *ch;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	int name_argc;

	strcpy(name1, name);
	ch = settings_parse_and_lookup(name1, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_set(name_argc - 1, &name_argv[1], len,
			 settings_runtime_read_fn, data);
}

int settings_runtime_get(const char *name, void *data, size_t len)
{
	struct settings_handler *ch;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	int name_argc;

	strcpy(name1, name);
	ch = settings_parse_and_lookup(name1, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_get(name_argc - 1, &name_argv[1], data, len);
}
