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

static int set(int argc, char **argv, char *val)
{
	int len;

	BT_DBG("argc %d argv[0] %s val %s", argc, argv[0], val);

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
		BT_DBG("ID Addr set to %s", bt_addr_le_str(&bt_dev.id_addr));
		return 0;
	}

#if defined(CONFIG_BT_PRIVACY)
	if (!strcmp(argv[0], "irk")) {
		len = sizeof(bt_dev.irk);
		settings_bytes_from_str(val, bt_dev.irk, &len);
		BT_DBG("IRK set to %s", bt_hex(bt_dev.irk, 16));
		return 0;
	}
#endif /* CONFIG_BT_PRIVACY */

	return 0;
}

static void generate_static_addr(void)
{
	char buf[13];
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
	char buf[25];
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
