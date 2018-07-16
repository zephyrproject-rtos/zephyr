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

/* Max settings key length (with all components) */
#define BT_SETTINGS_KEY_MAX 36

/* Base64-encoded string buffer size of in_size bytes */
#define BT_SETTINGS_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

/* Helpers for keys containing a bdaddr */
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    bt_addr_le_t *addr, const char *key);
int bt_settings_decode_key(char *key, bt_addr_le_t *addr);

void bt_settings_save_id(void);

int bt_settings_init(void);
