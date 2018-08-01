/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_SETTINGS)
#include "common/log.h"

#include "hci_core.h"
#include "settings.h"

/* Linker-defined symbols bound to the bt_settings_handler structs */
extern const struct bt_settings_handler _bt_settings_start[];
extern const struct bt_settings_handler _bt_settings_end[];

void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    bt_addr_le_t *addr, const char *key)
{
	if (key) {
		snprintk(path, path_size,
			 "bt/%s/%02x%02x%02x%02x%02x%02x%u/%s", subsys,
			 addr->a.val[5], addr->a.val[4], addr->a.val[3],
			 addr->a.val[2], addr->a.val[1], addr->a.val[0],
			 addr->type, key);
	} else {
		snprintk(path, path_size,
			 "bt/%s/%02x%02x%02x%02x%02x%02x%u", subsys,
			 addr->a.val[5], addr->a.val[4], addr->a.val[3],
			 addr->a.val[2], addr->a.val[1], addr->a.val[0],
			 addr->type);
	}

	BT_DBG("Encoded path %s", path);
}

int bt_settings_decode_key(char *key, bt_addr_le_t *addr)
{
	bool high;
	int i;

	if (strlen(key) != 13) {
		return -EINVAL;
	}

	if (key[12] == '0') {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (key[12] == '1') {
		addr->type = BT_ADDR_LE_RANDOM;
	} else {
		return -EINVAL;
	}

	for (i = 5, high = true; i >= 0; key++) {
		u8_t nibble;

		if (*key >= '0' && *key <= '9') {
			nibble = *key - '0';
		} else if (*key >= 'a' && *key <= 'f') {
			nibble = *key - 'a' + 10;
		} else {
			return -EINVAL;
		}

		if (high) {
			addr->a.val[i] = nibble << 4;
			high = false;
		} else {
			addr->a.val[i] |= nibble;
			high = true;
			i--;
		}
	}

	BT_DBG("Decoded %s as %s", key, bt_addr_le_str(addr));

	return 0;
}

static int set(int argc, char **argv, char *val)
{
	int len;

	BT_DBG("argc %d argv[0] %s argv[1] %s val %s", argc, argv[0],
	       argc > 1 ? argv[1] : "(null)", val ? val : "(null)");

	if (argc > 1) {
		const struct bt_settings_handler *h;

		for (h = _bt_settings_start; h < _bt_settings_end; h++) {
			if (!strcmp(argv[0], h->name)) {
				argc--;
				argv++;

				return h->set(argc, argv, val);
			}
		}

		return -ENOENT;
	}

	if (!strcmp(argv[0], "id")) {
		len = sizeof(bt_dev.id_addr);
		settings_bytes_from_str(val, &bt_dev.id_addr, &len);
		if (len != sizeof(bt_dev.id_addr)) {
			BT_ERR("Invalid length ID address in storage");
			bt_addr_le_copy(&bt_dev.id_addr, BT_ADDR_LE_ANY);
		} else {
			BT_DBG("ID Addr set to %s",
			       bt_addr_le_str(&bt_dev.id_addr));
		}

		return 0;
	}

	if (!strcmp(argv[0], "name")) {
		len = sizeof(bt_dev.name) - 1;
		settings_bytes_from_str(val, &bt_dev.name, &len);
		bt_dev.name[len] = '\0';

		BT_DBG("Name set to %s", bt_dev.name);
		return 0;
	}

#if defined(CONFIG_BT_PRIVACY)
	if (!strcmp(argv[0], "irk")) {
		len = sizeof(bt_dev.irk);
		settings_bytes_from_str(val, bt_dev.irk, &len);
		if (len != sizeof(bt_dev.irk)) {
			BT_ERR("Invalid length IRK in storage");
			memset(bt_dev.irk, 0, sizeof(bt_dev.irk));
		} else {
			BT_DBG("IRK set to %s", bt_hex(bt_dev.irk, 16));
		}

		return 0;
	}
#endif /* CONFIG_BT_PRIVACY */

	return 0;
}

static void generate_static_addr(void)
{
	char buf[BT_SETTINGS_SIZE(sizeof(bt_dev.id_addr))];
	char *str;

	BT_DBG("Generating new static random address");

	if (bt_addr_le_create_static(&bt_dev.id_addr)) {
		BT_ERR("Failed to generate static addr");
		return;
	}

	bt_set_static_addr();

	BT_DBG("New ID Addr: %s", bt_addr_le_str(&bt_dev.id_addr));

	str = settings_str_from_bytes(&bt_dev.id_addr, sizeof(bt_dev.id_addr),
				      buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode ID Addr as value");
		return;
	}

	BT_DBG("Saving ID addr as value %s", str);
	settings_save_one("bt/id", str);
}

#if defined(CONFIG_BT_PRIVACY)
static void generate_irk(void)
{
	char buf[BT_SETTINGS_SIZE(sizeof(bt_dev.irk))];
	char *str;

	BT_DBG("Generating new IRK");

	if (bt_rand(bt_dev.irk, sizeof(bt_dev.irk))) {
		BT_ERR("Failed to generate IRK");
		return;
	}

	BT_DBG("New local IRK: %s", bt_hex(bt_dev.irk, 16));

	str = settings_str_from_bytes(bt_dev.irk, 16, buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode IRK as value");
		return;
	}

	BT_DBG("Saving IRK as value %s", str);
	settings_save_one("bt/irk", str);
}
#endif /* CONFIG_BT_PRIVACY */

static int commit(void)
{
	const struct bt_settings_handler *h;

	BT_DBG("");

	if (!bt_addr_le_cmp(&bt_dev.id_addr, BT_ADDR_LE_ANY) ||
	    !bt_addr_le_cmp(&bt_dev.id_addr, BT_ADDR_LE_NONE)) {
		generate_static_addr();
	}

#if CONFIG_BT_DEVICE_NAME_MAX > 0
	if (bt_dev.name[0] == '\0') {
		bt_set_name(CONFIG_BT_DEVICE_NAME);
	}
#endif

#if defined(CONFIG_BT_PRIVACY)
	{
		u8_t zero[16] = { 0 };

		if (!memcmp(bt_dev.irk, zero, 16)) {
			generate_irk();
		}
	}
#endif /* CONFIG_BT_PRIVACY */

	for (h = _bt_settings_start; h < _bt_settings_end; h++) {
		if (h->commit) {
			h->commit();
		}
	}

	bt_dev_show_info();

	return 0;
}

static int export(int (*func)(const char *name, char *val),
		  enum settings_export_tgt tgt)
{
	const struct bt_settings_handler *h;

	if (tgt != SETTINGS_EXPORT_PERSIST) {
		BT_WARN("Only persist target supported");
		return -ENOTSUP;
	}

	for (h = _bt_settings_start; h < _bt_settings_end; h++) {
		if (h->export) {
			h->export(func);
		}
	}

	return 0;
}

static struct settings_handler bt_settings = {
	.name = "bt",
	.h_set = set,
	.h_commit = commit,
	.h_export = export,
};

int bt_settings_init(void)
{
	int err;

	BT_DBG("");

	err = settings_subsys_init();
	if (err) {
		BT_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	err = settings_register(&bt_settings);
	if (err) {
		BT_ERR("settings_register failed (err %d)", err);
		return err;
	}

	return 0;
}
