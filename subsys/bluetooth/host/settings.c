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
		/* Any previously provided identities supersede flash */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_PRESET_ID)) {
			BT_WARN("Ignoring identities stored in flash");
			return 0;
		}

		len = sizeof(bt_dev.id_addr);
		settings_bytes_from_str(val, &bt_dev.id_addr, &len);
		if (len < sizeof(bt_dev.id_addr[0])) {
			BT_ERR("Invalid length ID address in storage");
			(void)memset(bt_dev.id_addr, 0,
				     sizeof(bt_dev.id_addr));
			bt_dev.id_count = 0;
		} else {
			int i;

			bt_dev.id_count = len / sizeof(bt_dev.id_addr[0]);
			for (i = 0; i < bt_dev.id_count; i++) {
				BT_DBG("ID Addr %d %s", i,
				       bt_addr_le_str(&bt_dev.id_addr[i]));
			}
		}

		return 0;
	}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	if (!strcmp(argv[0], "name")) {
		len = sizeof(bt_dev.name) - 1;
		settings_bytes_from_str(val, &bt_dev.name, &len);
		bt_dev.name[len] = '\0';

		BT_DBG("Name set to %s", bt_dev.name);
		return 0;
	}
#endif

#if defined(CONFIG_BT_PRIVACY)
	if (!strcmp(argv[0], "irk")) {
		len = sizeof(bt_dev.irk);
		settings_bytes_from_str(val, bt_dev.irk, &len);
		if (len < sizeof(bt_dev.irk[0])) {
			BT_ERR("Invalid length IRK in storage");
			(void)memset(bt_dev.irk, 0, sizeof(bt_dev.irk));
		} else {
			BT_DBG("IRK set to %s", bt_hex(bt_dev.irk[0], 16));
		}

		return 0;
	}
#endif /* CONFIG_BT_PRIVACY */

	return 0;
}

#if defined(CONFIG_BT_PRIVACY)
#define ID_SIZE_MAX sizeof(bt_dev.irk)
#else
#define ID_SIZE_MAX sizeof(bt_dev.id_addr)
#endif

#define ID_DATA_LEN(array) (bt_dev.id_count * sizeof(array[0]))

static void save_id(struct k_work *work)
{
	char buf[BT_SETTINGS_SIZE(ID_SIZE_MAX)];
	char *str;

	str = settings_str_from_bytes(&bt_dev.id_addr,
				      ID_DATA_LEN(bt_dev.id_addr),
				      buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode ID Addr as value");
		return;
	}

	BT_DBG("Saving ID addr as value %s", str);
	settings_save_one("bt/id", str);

#if defined(CONFIG_BT_PRIVACY)
	str = settings_str_from_bytes(bt_dev.irk, ID_DATA_LEN(bt_dev.irk),
				      buf, sizeof(buf));
	if (!str) {
		BT_ERR("Unable to encode IRK as value");
		return;
	}

	BT_DBG("Saving IRK as value %s", str);
	settings_save_one("bt/irk", str);
#endif
}

K_WORK_DEFINE(save_id_work, save_id);

void bt_settings_save_id(void)
{
	k_work_submit(&save_id_work);
}

static int commit(void)
{
	const struct bt_settings_handler *h;

	BT_DBG("");

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	if (bt_dev.name[0] == '\0') {
		bt_set_name(CONFIG_BT_DEVICE_NAME);
	}
#endif
	if (!bt_dev.id_count) {
		int err;

		err = bt_setup_id_addr();
		if (err) {
			BT_ERR("Unable to setup an identity address");
			return err;
		}
	}

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
