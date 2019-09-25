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
#define LOG_MODULE_NAME bt_settings
#include "common/log.h"

#include "hci_core.h"
#include "settings.h"

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

	BT_DBG("Encoded path %s", log_strdup(path));
}

int bt_settings_decode_key(const char *key, bt_addr_le_t *addr)
{
	bool high;
	int i;

	if (settings_name_next(key, NULL) != 13) {
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

	BT_DBG("Decoded %s as %s", log_strdup(key), bt_addr_le_str(addr));

	return 0;
}

static int set(const char *name, size_t len_rd, settings_read_cb read_cb,
	       void *cb_arg)
{
	ssize_t len;
	const char *next;

	if (!name) {
		BT_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	len = settings_name_next(name, &next);

	if (!strncmp(name, "id", len)) {
		/* Any previously provided identities supersede flash */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_PRESET_ID)) {
			BT_WARN("Ignoring identities stored in flash");
			return 0;
		}

		len = read_cb(cb_arg, &bt_dev.id_addr, sizeof(bt_dev.id_addr));
		if (len < sizeof(bt_dev.id_addr[0])) {
			if (len < 0) {
				BT_ERR("Failed to read ID address from storage"
				       " (err %zu)", len);
			} else {
				BT_ERR("Invalid length ID address in storage");
				BT_HEXDUMP_DBG(&bt_dev.id_addr, len,
					       "data read");
			}
			(void)memset(bt_dev.id_addr, 0,
				     sizeof(bt_dev.id_addr));
			bt_dev.id_count = 0U;
		} else {
			int i;

			bt_dev.id_count = len / sizeof(bt_dev.id_addr[0]);
			for (i = 0; i < bt_dev.id_count; i++) {
				BT_DBG("ID[%d] %s", i,
				       bt_addr_le_str(&bt_dev.id_addr[i]));
			}
		}

		return 0;
	}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	if (!strncmp(name, "name", len)) {
		len = read_cb(cb_arg, &bt_dev.name, sizeof(bt_dev.name) - 1);
		if (len < 0) {
			BT_ERR("Failed to read device name from storage"
			       " (err %zu)", len);
		} else {
			bt_dev.name[len] = '\0';

			BT_DBG("Name set to %s", bt_dev.name);
		}
		return 0;
	}
#endif

#if defined(CONFIG_BT_PRIVACY)
	if (!strncmp(name, "irk", len)) {
		len = read_cb(cb_arg, bt_dev.irk, sizeof(bt_dev.irk));
		if (len < sizeof(bt_dev.irk[0])) {
			if (len < 0) {
				BT_ERR("Failed to read IRK from storage"
				       " (err %zu)", len);
			} else {
				BT_ERR("Invalid length IRK in storage");
				(void)memset(bt_dev.irk, 0, sizeof(bt_dev.irk));
			}
		} else {
			int i, count;

			count = len / sizeof(bt_dev.irk[0]);
			for (i = 0; i < count; i++) {
				BT_DBG("IRK[%d] %s", i,
				       bt_hex(bt_dev.irk[i], 16));
			}
		}

		return 0;
	}
#endif /* CONFIG_BT_PRIVACY */

	return -ENOENT;
}

#define ID_DATA_LEN(array) (bt_dev.id_count * sizeof(array[0]))

static void save_id(struct k_work *work)
{
	int err;
	BT_INFO("Saving ID");
	err = settings_save_one("bt/id", &bt_dev.id_addr,
				ID_DATA_LEN(bt_dev.id_addr));
	if (err) {
		BT_ERR("Failed to save ID (err %d)", err);
	}

#if defined(CONFIG_BT_PRIVACY)
	err = settings_save_one("bt/irk", bt_dev.irk, ID_DATA_LEN(bt_dev.irk));
	if (err) {
		BT_ERR("Failed to save IRK (err %d)", err);
	}
#endif
}

K_WORK_DEFINE(save_id_work, save_id);

void bt_settings_save_id(void)
{
	k_work_submit(&save_id_work);
}

static int commit(void)
{
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

	/* Make sure that the identities created by bt_id_create after
	 * bt_enable is saved to persistent storage. */
	if (!atomic_test_bit(bt_dev.flags, BT_DEV_PRESET_ID)) {
		bt_settings_save_id();
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_finalize_init();
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt, "bt", NULL, set, commit, NULL);

int bt_settings_init(void)
{
	int err;

	BT_DBG("");

	err = settings_subsys_init();
	if (err) {
		BT_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	return 0;
}
