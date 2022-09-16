/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/hci.h>

#include "ecc.h"
#include "hci_core.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#define LOG_MODULE_NAME bt_ecc
#include "common/log.h"

static uint8_t pub_key[BT_PUB_KEY_LEN];
static sys_slist_t pub_key_cb_slist;
static bt_dh_key_cb_t dh_key_cb;

static const uint8_t debug_public_key[BT_PUB_KEY_LEN] = {
	/* X */
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
	0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
	0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20,
	/* Y */
	0x8b, 0xd2, 0x89, 0x15, 0xd0, 0x8e, 0x1c, 0x74,
	0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63,
	0x6d, 0xeb, 0x2a, 0x65, 0x49, 0x9c, 0x80, 0xdc
};

bool bt_pub_key_is_debug(uint8_t *pub_key)
{
	return memcmp(pub_key, debug_public_key, BT_PUB_KEY_LEN) == 0;
}

int bt_pub_key_gen(struct bt_pub_key_cb *new_cb)
{
	struct bt_pub_key_cb *cb;
	int err;

	/*
	 * We check for both "LE Read Local P-256 Public Key" and
	 * "LE Generate DH Key" support here since both commands are needed for
	 * ECC support. If "LE Generate DH Key" is not supported then there
	 * is no point in reading local public key.
	 */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 34, 1) ||
	    !BT_CMD_TEST(bt_dev.supported_commands, 34, 2)) {
		BT_WARN("ECC HCI commands not available");
		return -ENOTSUP;
	}

	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS)) {
		if (!BT_CMD_TEST(bt_dev.supported_commands, 41, 2)) {
			BT_WARN("ECC Debug keys HCI command not available");
		} else {
			atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
			new_cb->func(debug_public_key);
			return 0;
		}
	}

	if (!new_cb) {
		return -EINVAL;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&pub_key_cb_slist, cb, node) {
		if (cb == new_cb) {
			BT_WARN("Callback already registered");
			return -EALREADY;
		}
	}

	sys_slist_prepend(&pub_key_cb_slist, &new_cb->node);

	if (atomic_test_and_set_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return 0;
	}

	atomic_clear_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_P256_PUBLIC_KEY, NULL, NULL);
	if (err) {

		BT_ERR("Sending LE P256 Public Key command failed");
		atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

		SYS_SLIST_FOR_EACH_CONTAINER(&pub_key_cb_slist, cb, node) {
			if (cb->func) {
				cb->func(NULL);
			}
		}

		sys_slist_init(&pub_key_cb_slist);
		return err;
	}

	return 0;
}

const uint8_t *bt_pub_key_get(void)
{
	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS) &&
	    BT_CMD_TEST(bt_dev.supported_commands, 41, 2)) {
		return debug_public_key;
	}

	if (atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return pub_key;
	}

	return NULL;
}

static int hci_generate_dhkey_v1(const uint8_t *remote_pk)
{
	struct bt_hci_cp_le_generate_dhkey *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_GENERATE_DHKEY, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(cp->key, remote_pk, sizeof(cp->key));

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_GENERATE_DHKEY, buf, NULL);
}

static int hci_generate_dhkey_v2(const uint8_t *remote_pk, uint8_t key_type)
{
	struct bt_hci_cp_le_generate_dhkey_v2 *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_GENERATE_DHKEY_V2, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	memcpy(cp->key, remote_pk, sizeof(cp->key));
	cp->key_type = key_type;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_GENERATE_DHKEY_V2, buf, NULL);
}

int bt_dh_key_gen(const uint8_t remote_pk[BT_PUB_KEY_LEN], bt_dh_key_cb_t cb)
{
	int err;

	if (dh_key_cb == cb) {
		return -EALREADY;
	}

	if (dh_key_cb || atomic_test_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY)) {
		return -EBUSY;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY)) {
		return -EADDRNOTAVAIL;
	}

	dh_key_cb = cb;

	if (IS_ENABLED(CONFIG_BT_USE_DEBUG_KEYS) &&
	    BT_CMD_TEST(bt_dev.supported_commands, 41, 2)) {
		err = hci_generate_dhkey_v2(remote_pk,
					    BT_HCI_LE_KEY_TYPE_DEBUG);
	} else {
		err = hci_generate_dhkey_v1(remote_pk);
	}

	if (err) {
		dh_key_cb = NULL;
		BT_WARN("Failed to generate DHKey (err %d)", err);
		return err;
	}

	return 0;
}

void bt_hci_evt_le_pkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_p256_public_key_complete *evt = (void *)buf->data;
	struct bt_pub_key_cb *cb;

	BT_DBG("status: 0x%02x", evt->status);

	atomic_clear_bit(bt_dev.flags, BT_DEV_PUB_KEY_BUSY);

	if (!evt->status) {
		memcpy(pub_key, evt->key, BT_PUB_KEY_LEN);
		atomic_set_bit(bt_dev.flags, BT_DEV_HAS_PUB_KEY);
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&pub_key_cb_slist, cb, node) {
		if (cb->func) {
			cb->func(evt->status ? NULL : pub_key);
		}
	}

	sys_slist_init(&pub_key_cb_slist);
}

void bt_hci_evt_le_dhkey_complete(struct net_buf *buf)
{
	struct bt_hci_evt_le_generate_dhkey_complete *evt = (void *)buf->data;

	BT_DBG("status: 0x%02x", evt->status);

	if (dh_key_cb) {
		bt_dh_key_cb_t cb = dh_key_cb;

		dh_key_cb = NULL;
		cb(evt->status ? NULL : evt->dhkey);
	}
}
