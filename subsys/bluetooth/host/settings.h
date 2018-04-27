/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bt_settings_handler {
	const char *name;
	int (*set)(int argc, char **argv, char *val);
	int (*commit)(void);
	int (*export)(int (*func)(const char *name, char *val));
};

#define BT_SETTINGS_DEFINE(_name, _set, _commit, _export)               \
	const struct bt_settings_handler _name __aligned(4)             \
			__in_section(_bt_settings, static, _name) = {   \
				.name = STRINGIFY(_name),               \
				.set = _set,                            \
				.commit = _commit,                      \
				.export = _export,                      \
			}

int bt_settings_init(void);
