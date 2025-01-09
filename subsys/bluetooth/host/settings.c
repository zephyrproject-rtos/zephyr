/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>

#include "common/bt_str.h"

#include "hci_core.h"
#include "settings.h"

#define LOG_LEVEL CONFIG_BT_SETTINGS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_settings);

#if defined(CONFIG_BT_SETTINGS_USE_PRINTK)
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key)
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

	LOG_DBG("Encoded path %s", path);
}
#else
void bt_settings_encode_key(char *path, size_t path_size, const char *subsys,
			    const bt_addr_le_t *addr, const char *key)
{
	size_t len = 3;

	/* Skip if path_size is less than 3; strlen("bt/") */
	if (len < path_size) {
		/* Key format:
		 *  "bt/<subsys>/<addr><type>/<key>", "/<key>" is optional
		 */
		strcpy(path, "bt/");
		strncpy(&path[len], subsys, path_size - len);
		len = strlen(path);
		if (len < path_size) {
			path[len] = '/';
			len++;
		}

		for (int8_t i = 5; i >= 0 && len < path_size; i--) {
			len += bin2hex(&addr->a.val[i], 1, &path[len],
				       path_size - len);
		}

		if (len < path_size) {
			/* Type can be either BT_ADDR_LE_PUBLIC or
			 * BT_ADDR_LE_RANDOM (value 0 or 1)
			 */
			path[len] = '0' + addr->type;
			len++;
		}

		if (key && len < path_size) {
			path[len] = '/';
			len++;
			strncpy(&path[len], key, path_size - len);
			len += strlen(&path[len]);
		}

		if (len >= path_size) {
			/* Truncate string */
			path[path_size - 1] = '\0';
		}
	} else if (path_size > 0) {
		*path = '\0';
	}

	LOG_DBG("Encoded path %s", path);
}
#endif

int bt_settings_decode_key(const char *key, bt_addr_le_t *addr)
{
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

	for (uint8_t i = 0; i < 6; i++) {
		hex2bin(&key[i * 2], 2, &addr->a.val[5 - i], 1);
	}

	LOG_DBG("Decoded %s as %s", key, bt_addr_le_str(addr));

	return 0;
}

static int set_setting(const char *name, size_t len_rd, settings_read_cb read_cb,
	       void *cb_arg)
{
	ssize_t len;
	const char *next;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		/* The Bluetooth settings loader needs to communicate with the Bluetooth
		 * controller to setup identities. This will not work before
		 * bt_enable(). The doc on @ref bt_enable requires the "bt/" settings
		 * tree to be loaded after @ref bt_enable is completed, so this handler
		 * will be called again later.
		 */
		return 0;
	}

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	len = settings_name_next(name, &next);

	if (!strncmp(name, "id", len)) {
		/* Any previously provided identities supersede flash */
		if (atomic_test_bit(bt_dev.flags, BT_DEV_PRESET_ID)) {
			LOG_WRN("Ignoring identities stored in flash");
			return 0;
		}

		len = read_cb(cb_arg, &bt_dev.id_addr, sizeof(bt_dev.id_addr));
		if (len < sizeof(bt_dev.id_addr[0])) {
			if (len < 0) {
				LOG_ERR("Failed to read ID address from storage"
				       " (err %zd)", len);
			} else {
				LOG_ERR("Invalid length ID address in storage");
				LOG_HEXDUMP_DBG(&bt_dev.id_addr, len, "data read");
			}
			(void)memset(bt_dev.id_addr, 0,
				     sizeof(bt_dev.id_addr));
			bt_dev.id_count = 0U;
		} else {
			int i;

			bt_dev.id_count = len / sizeof(bt_dev.id_addr[0]);
			for (i = 0; i < bt_dev.id_count; i++) {
				LOG_DBG("ID[%d] %s", i, bt_addr_le_str(&bt_dev.id_addr[i]));
			}
		}

		return 0;
	}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	if (!strncmp(name, "name", len)) {
		len = read_cb(cb_arg, &bt_dev.name, sizeof(bt_dev.name) - 1);
		if (len < 0) {
			LOG_ERR("Failed to read device name from storage"
			       " (err %zd)", len);
		} else {
			bt_dev.name[len] = '\0';

			LOG_DBG("Name set to %s", bt_dev.name);
		}
		return 0;
	}
#endif

#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	if (!strncmp(name, "appearance", len)) {
		if (len_rd != sizeof(bt_dev.appearance)) {
			LOG_ERR("Ignoring settings entry 'bt/appearance'. Wrong length.");
			return -EINVAL;
		}

		len = read_cb(cb_arg, &bt_dev.appearance, sizeof(bt_dev.appearance));
		if (len < 0) {
			return len;
		}

		return 0;
	}
#endif

#if defined(CONFIG_BT_PRIVACY)
	if (!strncmp(name, "irk", len)) {
		len = read_cb(cb_arg, bt_dev.irk, sizeof(bt_dev.irk));
		if (len < sizeof(bt_dev.irk[0])) {
			if (len < 0) {
				LOG_ERR("Failed to read IRK from storage"
				       " (err %zd)", len);
			} else {
				LOG_ERR("Invalid length IRK in storage");
				(void)memset(bt_dev.irk, 0, sizeof(bt_dev.irk));
			}
		} else {
			int i, count;

			count = len / sizeof(bt_dev.irk[0]);
			for (i = 0; i < count; i++) {
				LOG_DBG("IRK[%d] %s", i, bt_hex(bt_dev.irk[i], 16));
			}
		}

		return 0;
	}
#endif /* CONFIG_BT_PRIVACY */

	return -ENOENT;
}

static int commit_settings(void)
{
	int err;

	LOG_DBG("");

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		/* The Bluetooth settings loader needs to communicate with the Bluetooth
		 * controller to setup identities. This will not work before
		 * bt_enable(). The doc on @ref bt_enable requires the "bt/" settings
		 * tree to be loaded after @ref bt_enable is completed, so this handler
		 * will be called again later.
		 */
		return 0;
	}

#if defined(CONFIG_BT_DEVICE_NAME_DYNAMIC)
	if (bt_dev.name[0] == '\0') {
		bt_set_name(CONFIG_BT_DEVICE_NAME);
	}
#endif
	if (!bt_dev.id_count) {
		err = bt_setup_public_id_addr();
		if (err) {
			LOG_ERR("Unable to setup an identity address");
			return err;
		}
	}

	if (!bt_dev.id_count) {
		err = bt_setup_random_id_addr();
		if (err) {
			LOG_ERR("Unable to setup an identity address");
			return err;
		}
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		bt_finalize_init();
	}

	/* If any part of the Identity Information of the device has been
	 * generated this Identity needs to be saved persistently.
	 */
	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_STORE_ID)) {
		LOG_DBG("Storing Identity Information");
		bt_settings_store_id();
		bt_settings_store_irk();
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt, "bt", NULL, set_setting, commit_settings, NULL);

int bt_settings_init(void)
{
	int err;

	LOG_DBG("");

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("settings_subsys_init failed (err %d)", err);
		return err;
	}

	return 0;
}

__weak void bt_testing_settings_store_hook(const char *key, const void *value, size_t val_len)
{
	ARG_UNUSED(key);
	ARG_UNUSED(value);
	ARG_UNUSED(val_len);
}

__weak void bt_testing_settings_delete_hook(const char *key)
{
	ARG_UNUSED(key);
}

int bt_settings_store(const char *key, uint8_t id, const bt_addr_le_t *addr, const void *value,
		      size_t val_len)
{
	int err;
	char id_str[4];
	char key_str[BT_SETTINGS_KEY_MAX];

	if (addr) {
		if (id) {
			u8_to_dec(id_str, sizeof(id_str), id);
		}

		bt_settings_encode_key(key_str, sizeof(key_str), key, addr, (id ? id_str : NULL));
	} else {
		err = snprintk(key_str, sizeof(key_str), "bt/%s", key);
		if (err < 0) {
			return -EINVAL;
		}
	}

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_testing_settings_store_hook(key_str, value, val_len);
	}

	return settings_save_one(key_str, value, val_len);
}

int bt_settings_delete(const char *key, uint8_t id, const bt_addr_le_t *addr)
{
	int err;
	char id_str[4];
	char key_str[BT_SETTINGS_KEY_MAX];

	if (addr) {
		if (id) {
			u8_to_dec(id_str, sizeof(id_str), id);
		}

		bt_settings_encode_key(key_str, sizeof(key_str), key, addr, (id ? id_str : NULL));
	} else {
		err = snprintk(key_str, sizeof(key_str), "bt/%s", key);
		if (err < 0) {
			return -EINVAL;
		}
	}

	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_testing_settings_delete_hook(key_str);
	}

	return settings_delete(key_str);
}

int bt_settings_store_sc(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len)
{
	return bt_settings_store("sc", id, addr, value, val_len);
}

int bt_settings_delete_sc(uint8_t id, const bt_addr_le_t *addr)
{
	return bt_settings_delete("sc", id, addr);
}

int bt_settings_store_cf(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len)
{
	return bt_settings_store("cf", id, addr, value, val_len);
}

int bt_settings_delete_cf(uint8_t id, const bt_addr_le_t *addr)
{
	return bt_settings_delete("cf", id, addr);
}

int bt_settings_store_ccc(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len)
{
	return bt_settings_store("ccc", id, addr, value, val_len);
}

int bt_settings_delete_ccc(uint8_t id, const bt_addr_le_t *addr)
{
	return bt_settings_delete("ccc", id, addr);
}

int bt_settings_store_hash(const void *value, size_t val_len)
{
	return bt_settings_store("hash", 0, NULL, value, val_len);
}

int bt_settings_delete_hash(void)
{
	return bt_settings_delete("hash", 0, NULL);
}

int bt_settings_store_name(const void *value, size_t val_len)
{
	return bt_settings_store("name", 0, NULL, value, val_len);
}

int bt_settings_delete_name(void)
{
	return bt_settings_delete("name", 0, NULL);
}

int bt_settings_store_appearance(const void *value, size_t val_len)
{
	return bt_settings_store("appearance", 0, NULL, value, val_len);
}

int bt_settings_delete_appearance(void)
{
	return bt_settings_delete("appearance", 0, NULL);
}

static void do_store_id(struct k_work *work)
{
	int err = bt_settings_store("id", 0, NULL, &bt_dev.id_addr, ID_DATA_LEN(bt_dev.id_addr));

	if (err) {
		LOG_ERR("Failed to save ID (err %d)", err);
	}
}

K_WORK_DEFINE(store_id_work, do_store_id);

int bt_settings_store_id(void)
{
	k_work_submit(&store_id_work);

	return 0;
}

int bt_settings_delete_id(void)
{
	return bt_settings_delete("id", 0, NULL);
}

static void do_store_irk(struct k_work *work)
{
#if defined(CONFIG_BT_PRIVACY)
	int err = bt_settings_store("irk", 0, NULL, bt_dev.irk, ID_DATA_LEN(bt_dev.irk));

	if (err) {
		LOG_ERR("Failed to save IRK (err %d)", err);
	}
#endif
}

K_WORK_DEFINE(store_irk_work, do_store_irk);

int bt_settings_store_irk(void)
{
#if defined(CONFIG_BT_PRIVACY)
	k_work_submit(&store_irk_work);
#endif /* defined(CONFIG_BT_PRIVACY) */
	return 0;
}

int bt_settings_delete_irk(void)
{
	return bt_settings_delete("irk", 0, NULL);
}

int bt_settings_store_link_key(const bt_addr_le_t *addr, const void *value, size_t val_len)
{
	return bt_settings_store("link_key", 0, addr, value, val_len);
}

int bt_settings_delete_link_key(const bt_addr_le_t *addr)
{
	return bt_settings_delete("link_key", 0, addr);
}

int bt_settings_store_keys(uint8_t id, const bt_addr_le_t *addr, const void *value, size_t val_len)
{
	return bt_settings_store("keys", id, addr, value, val_len);
}

int bt_settings_delete_keys(uint8_t id, const bt_addr_le_t *addr)
{
	return bt_settings_delete("keys", id, addr);
}
