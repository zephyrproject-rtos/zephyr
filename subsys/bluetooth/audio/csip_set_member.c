/*  Bluetooth CSIP - Coordinated Set Identification Profile */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include "../host/conn_internal.h"
#include "../host/keys.h"
#include "../host/settings.h"

#include "common/bt_str.h"
#include "audio_internal.h"
#include "csip_internal.h"
#include "csip_crypto.h"

#define CSIP_SET_LOCK_TIMER_VALUE       K_SECONDS(60)

#define CSIS_CHAR_ATTR_COUNT	  3 /* declaration + value + cccd */
#define CSIS_RANK_CHAR_ATTR_COUNT 2 /* declaration + value */

LOG_MODULE_REGISTER(bt_csip_set_member, CONFIG_BT_CSIP_SET_MEMBER_LOG_LEVEL);

enum csip_flag {
	FLAG_ACTIVE,
	FLAG_NOTIFY_LOCK,
	FLAG_NOTIFY_SIRK,
	FLAG_NOTIFY_SIZE,
	FLAG_NUM,
};

struct csip_client {
	bt_addr_le_t addr;

	/* Pending notification flags */
	ATOMIC_DEFINE(flags, FLAG_NUM);
};

struct bt_csip_set_member_svc_inst {
	struct bt_csip_sirk sirk;
	uint8_t set_size;
	uint8_t set_lock;
	uint8_t rank;
	bool lockable;
	struct bt_csip_set_member_cb *cb;
	struct k_work_delayable set_lock_timer;
	bt_addr_le_t lock_client_addr;
	struct bt_gatt_service *service_p;
	struct csip_client clients[CONFIG_BT_MAX_PAIRED];
	/* Must be last: exclude from memset during unregister */
	struct k_mutex mutex;
};

static struct bt_csip_set_member_svc_inst svc_insts[CONFIG_BT_CSIP_SET_MEMBER_MAX_INSTANCE_COUNT];

static void deferred_nfy_work_handler(struct k_work *work);
static void add_bonded_addr_to_client_list(const struct bt_bond_info *info, void *data);

#if defined(CONFIG_BT_SETTINGS)
static int csip_settings_commit(void)
{
	bt_foreach_bond(BT_ID_DEFAULT, add_bonded_addr_to_client_list, NULL);

	LOG_DBG("Restored CSIP client list from bonded devices");

	return 0;
}

/* Register CSIP settings handler with commit priority, BT_SETTINGS_CPRIO_2,
 * to ensure csip_settings_commit() runs after BT keys settings are loaded.
 * Priority is reduced to ensure existing bonds are loaded first.
 */
SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(bt_csip_set_member, "bt/csip", NULL, NULL,
					csip_settings_commit, NULL, BT_SETTINGS_CPRIO_2);
#endif /* CONFIG_BT_SETTINGS */

static K_WORK_DELAYABLE_DEFINE(deferred_nfy_work, deferred_nfy_work_handler);

static bool is_last_client_to_write(const struct bt_csip_set_member_svc_inst *svc_inst,
				    const struct bt_conn *conn)
{
	if (conn != NULL) {
		return bt_addr_le_eq(bt_conn_get_dst(conn),
				     &svc_inst->lock_client_addr);
	} else {
		return bt_addr_le_eq(BT_ADDR_LE_NONE, &svc_inst->lock_client_addr);
	}
}

static void notify_work_reschedule(k_timeout_t delay)
{
	int err;

	/* If it is already scheduled, don't reschedule */
	if (k_work_delayable_remaining_get(&deferred_nfy_work) > 0) {
		return;
	}

	err = k_work_reschedule(&deferred_nfy_work, delay);
	if (err < 0) {
		LOG_ERR("Failed to reschedule notification work err %d", err);
	}
}

static void notify_clients(struct bt_csip_set_member_svc_inst *svc_inst,
			   struct bt_conn *excluded_client, enum csip_flag flag)
{
	bool submit_work = false;

	/* Mark all bonded devices (except the excluded one) as pending notifications */
	for (size_t i = 0U; i < ARRAY_SIZE(svc_inst->clients); i++) {
		struct csip_client *client;

		client = &svc_inst->clients[i];

		if (atomic_test_bit(client->flags, FLAG_ACTIVE)) {
			if (excluded_client != NULL &&
			    bt_addr_le_eq(bt_conn_get_dst(excluded_client), &client->addr)) {
				continue;
			}

			atomic_set_bit(client->flags, flag);
			submit_work = true;
		}
	}

	/* Reschedule work for notifying */
	if (submit_work) {
		notify_work_reschedule(K_NO_WAIT);
	}
}

static int sirk_encrypt(struct bt_conn *conn, const struct bt_csip_sirk *sirk,
			struct bt_csip_sirk *enc_sirk)
{
	int err;
	const uint8_t *k;

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_TEST_SAMPLE_DATA)) {
		/* test_k is from the sample data from A.2 in the CSIS spec */
		static const uint8_t test_k[] = {
			/* Sample data is in big-endian, we need it in little-endian. */
			REVERSE_ARGS(0x67, 0x6e, 0x1b, 0x9b,
				     0xd4, 0x48, 0x69, 0x6f,
				     0x06, 0x1e, 0xc6, 0x22,
				     0x3c, 0xe5, 0xce, 0xd9) };
		LOG_DBG("Encrypting test SIRK");
		k = test_k;
	} else {
		if (conn == NULL) {
			return -EINVAL;
		}

		k = conn->le.keys->ltk.val;
	}

	err = bt_csip_sef(k, sirk->value, enc_sirk->value);

	if (err != 0) {
		return err;
	}

	enc_sirk->type = BT_CSIP_SIRK_TYPE_ENCRYPTED;

	return 0;
}

static int generate_prand(uint8_t dest[BT_CSIP_CRYPTO_PRAND_SIZE])
{
	bool valid = false;

	do {
		int res;
		uint32_t prand;

		*dest = 0;
		res = bt_rand(dest, BT_CSIP_CRYPTO_PRAND_SIZE);
		if (res != 0) {
			return res;
		}

		/* Validate Prand: Must contain both a 1 and a 0 */
		prand = sys_get_le24(dest);
		if (prand != 0 && prand != 0x3FFFFF) {
			valid = true;
		}
	} while (!valid);

	dest[BT_CSIP_CRYPTO_PRAND_SIZE - 1] &= 0x3F;
	dest[BT_CSIP_CRYPTO_PRAND_SIZE - 1] |= BIT(6);

	return 0;
}

int bt_csip_set_member_generate_rsi(const struct bt_csip_set_member_svc_inst *svc_inst,
				    uint8_t rsi[BT_CSIP_RSI_SIZE])
{
	int res = 0;
	uint8_t prand[BT_CSIP_CRYPTO_PRAND_SIZE];
	uint8_t hash[BT_CSIP_CRYPTO_HASH_SIZE];

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_TEST_SAMPLE_DATA)) {
		/* prand is from the sample data from A.2 in the CSIS spec */
		sys_put_le24(0x69f563, prand);
	} else {
		res = generate_prand(prand);

		if (res != 0) {
			LOG_WRN("Could not generate new prand");
			return res;
		}
	}

	res = bt_csip_sih(svc_inst->sirk.value, prand, hash);
	if (res != 0) {
		LOG_WRN("Could not generate new RSI");
		return res;
	}

	(void)memcpy(rsi, hash, BT_CSIP_CRYPTO_HASH_SIZE);
	(void)memcpy(rsi + BT_CSIP_CRYPTO_HASH_SIZE, prand, BT_CSIP_CRYPTO_PRAND_SIZE);

	return res;
}

static ssize_t read_sirk(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	struct bt_csip_sirk enc_sirk;
	struct bt_csip_sirk *sirk;
	struct bt_csip_set_member_svc_inst *svc_inst = BT_AUDIO_CHRC_USER_DATA(attr);

	if (svc_inst->cb != NULL && svc_inst->cb->sirk_read_req != NULL) {
		ssize_t gatt_err = BT_GATT_ERR(BT_ATT_ERR_SUCCESS);
		uint8_t cb_rsp;

		/* Ask higher layer for what SIRK to return, if any */
		cb_rsp = svc_inst->cb->sirk_read_req(conn, &svc_insts[0]);

		if (cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT) {
			sirk = &svc_inst->sirk;
		} else if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_ENC_SIRK_SUPPORT) &&
			   cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC) {
			int err;

			err = sirk_encrypt(conn, &svc_inst->sirk, &enc_sirk);
			if (err != 0) {
				LOG_ERR("Could not encrypt SIRK: %d",
					err);
				gatt_err = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			} else {
				sirk = &enc_sirk;
				LOG_HEXDUMP_DBG(enc_sirk.value, sizeof(enc_sirk.value),
						"Encrypted SIRK");
			}
		} else if (cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_REJECT) {
			gatt_err = BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
		} else if (cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_OOB_ONLY) {
			gatt_err = BT_GATT_ERR(BT_CSIP_ERROR_SIRK_OOB_ONLY);
		} else {
			LOG_ERR("Invalid callback response: %u", cb_rsp);
			gatt_err = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		if (gatt_err != BT_GATT_ERR(BT_ATT_ERR_SUCCESS)) {
			return gatt_err;
		}
	} else {
		sirk = &svc_inst->sirk;
	}

	LOG_DBG("SIRK %sencrypted", sirk->type == BT_CSIP_SIRK_TYPE_PLAIN ? "not " : "");
	LOG_HEXDUMP_DBG(svc_inst->sirk.value, sizeof(svc_inst->sirk.value), "SIRK");
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 sirk, sizeof(*sirk));
}

#if defined(CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE)
static void sirk_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE */

static ssize_t read_set_size(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip_set_member_svc_inst *svc_inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%u", svc_inst->set_size);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &svc_inst->set_size,
				 sizeof(svc_inst->set_size));
}

#if defined(CONFIG_BT_CSIP_SET_MEMBER_SIZE_NOTIFIABLE)
static void set_size_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_CSIP_SET_MEMBER_SIZE_NOTIFIABLE */

static ssize_t read_set_lock(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip_set_member_svc_inst *svc_inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%u", svc_inst->set_lock);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &svc_inst->set_lock,
				 sizeof(svc_inst->set_lock));
}

/**
 * @brief Set the lock value of a CSIP instance.
 *
 * @param conn      The connection locking the instance.
 *                  Will be NULL if the server locally sets the lock.
 * @param svc_inst  The CSIP instance to change the lock value of
 * @param val       The lock value (BT_CSIP_LOCK_VALUE or BT_CSIP_RELEASE_VALUE)
 *
 * @return BT_CSIP_ERROR_* on failure or 0 if success
 */
static uint8_t set_lock(struct bt_conn *conn,
			struct bt_csip_set_member_svc_inst *svc_inst,
			uint8_t val)
{
	bool notify;

	if (val != BT_CSIP_RELEASE_VALUE && val != BT_CSIP_LOCK_VALUE) {
		return BT_CSIP_ERROR_LOCK_INVAL_VALUE;
	}

	if (svc_inst->set_lock == BT_CSIP_LOCK_VALUE) {
		if (val == BT_CSIP_LOCK_VALUE) {
			if (is_last_client_to_write(svc_inst, conn)) {
				return BT_CSIP_ERROR_LOCK_ALREADY_GRANTED;
			} else {
				return BT_CSIP_ERROR_LOCK_DENIED;
			}
		} else if (!is_last_client_to_write(svc_inst, conn)) {
			return BT_CSIP_ERROR_LOCK_RELEASE_DENIED;
		}
	}

	notify = svc_inst->set_lock != val;

	svc_inst->set_lock = val;
	if (svc_inst->set_lock == BT_CSIP_LOCK_VALUE) {
		if (conn != NULL) {
			bt_addr_le_copy(&svc_inst->lock_client_addr,
					bt_conn_get_dst(conn));
		}
		(void)k_work_reschedule(&svc_inst->set_lock_timer,
					CSIP_SET_LOCK_TIMER_VALUE);
	} else {
		bt_addr_le_copy(&svc_inst->lock_client_addr, BT_ADDR_LE_NONE);
		(void)k_work_cancel_delayable(&svc_inst->set_lock_timer);
	}

	LOG_DBG("%u", svc_inst->set_lock);

	if (notify) {
		/*
		 * The Spec states that all clients, except for the
		 * client writing the value, shall be notified
		 * (if subscribed)
		 */
		notify_clients(svc_inst, conn, FLAG_NOTIFY_LOCK);

		if (svc_inst->cb != NULL && svc_inst->cb->lock_changed != NULL) {
			bool locked = svc_inst->set_lock == BT_CSIP_LOCK_VALUE;

			svc_inst->cb->lock_changed(conn, svc_inst, locked);
		}
	}

	return 0;
}

static ssize_t write_set_lock(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len,
			      uint16_t offset, uint8_t flags)
{
	ssize_t res;
	uint8_t val;
	struct bt_csip_set_member_svc_inst *svc_inst = BT_AUDIO_CHRC_USER_DATA(attr);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len != sizeof(val)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	(void)memcpy(&val, buf, len);

	res = set_lock(conn, svc_inst, val);
	if (res != BT_ATT_ERR_SUCCESS) {
		return BT_GATT_ERR(res);
	}

	return len;
}

static void set_lock_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_rank(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip_set_member_svc_inst *svc_inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%u", svc_inst->rank);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &svc_inst->rank,
				 sizeof(svc_inst->rank));

}

static void set_lock_timer_handler(struct k_work *work)
{
	struct k_work_delayable *delayable;
	struct bt_csip_set_member_svc_inst *svc_inst;

	delayable = k_work_delayable_from_work(work);
	svc_inst = CONTAINER_OF(delayable, struct bt_csip_set_member_svc_inst,
				set_lock_timer);

	LOG_DBG("Lock timeout, releasing");
	svc_inst->set_lock = BT_CSIP_RELEASE_VALUE;
	notify_clients(svc_inst, NULL, FLAG_NOTIFY_LOCK);

	if (svc_inst->cb != NULL && svc_inst->cb->lock_changed != NULL) {
		bool locked = svc_inst->set_lock == BT_CSIP_LOCK_VALUE;

		svc_inst->cb->lock_changed(NULL, svc_inst, locked);
	}
}

static void csip_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	const bt_addr_le_t *peer_addr;

	if (err != 0 || conn->encrypt == 0) {
		return;
	}

	peer_addr = bt_conn_get_dst(conn);

	if (!bt_le_bond_exists(conn->id, &conn->le.dst)) {
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
		struct bt_csip_set_member_svc_inst *svc_inst = &svc_insts[i];
		struct csip_client *client;
		bool found = false;

		/* Check if client is already in the active list */
		for (size_t j = 0U; j < ARRAY_SIZE(svc_inst->clients); j++) {
			client = &svc_inst->clients[j];

			if (atomic_test_bit(client->flags, FLAG_ACTIVE) &&
			    bt_addr_le_eq(peer_addr, &client->addr)) {
				found = true;
				break;
			}
		}

		/* If not found, add the bonded address to the client list */
		if (!found) {
			const struct bt_bond_info bond_info = {
				.addr = *peer_addr
			};

			add_bonded_addr_to_client_list(&bond_info, NULL);
			return;
		}

		/* Check if client is set with FLAG_NOTIFY_LOCK */
		if (atomic_test_bit(client->flags, FLAG_NOTIFY_LOCK)) {
			notify_work_reschedule(K_NO_WAIT);
			break;
		}
	}
}

static void handle_csip_disconnect(struct bt_csip_set_member_svc_inst *svc_inst,
				   struct bt_conn *conn)
{
	LOG_DBG("Non-bonded device");
	if (is_last_client_to_write(svc_inst, conn)) {
		bt_addr_le_copy(&svc_inst->lock_client_addr, BT_ADDR_LE_NONE);
		svc_inst->set_lock = BT_CSIP_RELEASE_VALUE;
		notify_clients(svc_inst, NULL, FLAG_NOTIFY_LOCK);

		if (svc_inst->cb != NULL && svc_inst->cb->lock_changed != NULL) {
			bool locked = svc_inst->set_lock == BT_CSIP_LOCK_VALUE;

			svc_inst->cb->lock_changed(conn, svc_inst, locked);
		}
	}

	/* Check if the disconnected device once was bonded and stored
	 * here as a bonded device
	 */
	for (size_t i = 0U; i < ARRAY_SIZE(svc_inst->clients); i++) {
		struct csip_client *client;

		client = &svc_inst->clients[i];

		if (bt_addr_le_eq(bt_conn_get_dst(conn), &client->addr)) {
			(void)memset(client, 0, sizeof(*client));
			break;
		}
	}
}

static void csip_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("Disconnected: %s (reason %u)", bt_addr_le_str(bt_conn_get_dst(conn)), reason);

	if (!bt_le_bond_exists(conn->id, &conn->le.dst)) {
		for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
			handle_csip_disconnect(&svc_insts[i], conn);
		}
	}
}

static void handle_csip_auth_complete(struct bt_csip_set_member_svc_inst *svc_inst,
				      struct bt_conn *conn)
{
	/* Check if already in list, and do nothing if it is */
	for (size_t i = 0U; i < ARRAY_SIZE(svc_inst->clients); i++) {
		struct csip_client *client;

		client = &svc_inst->clients[i];

		if (atomic_test_bit(client->flags, FLAG_ACTIVE) &&
		    bt_addr_le_eq(bt_conn_get_dst(conn), &client->addr)) {
			return;
		}
	}

	/* Else add the device */
	for (size_t i = 0U; i < ARRAY_SIZE(svc_inst->clients); i++) {
		struct csip_client *client;

		client = &svc_inst->clients[i];

		if (!atomic_test_bit(client->flags, FLAG_ACTIVE)) {
			atomic_set_bit(client->flags, FLAG_ACTIVE);
			memcpy(&client->addr, bt_conn_get_dst(conn), sizeof(bt_addr_le_t));

			/* Send out all pending notifications */
			notify_work_reschedule(K_NO_WAIT);
			return;
		}
	}

	LOG_WRN("Could not add device to pending notification list");
}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	/**
	 * If a pairing is complete for a bonded device, then we
	 * 1) Store the connection pointer to later validate SIRK encryption
	 * 2) Check if the device is already in the `clients`, and if it is
	 * not, then we
	 * 3) Check if there's room for another device in the `clients`
	 *    array. If there are no more room for a new device, then
	 * 4) Either we ignore this new device (bad luck), or we overwrite
	 *    the oldest entry, following the behavior of the key storage.
	 */

	LOG_DBG("%s paired (%sbonded)", bt_addr_le_str(bt_conn_get_dst(conn)),
		bonded ? "" : "not ");

	if (!bonded) {
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
		handle_csip_auth_complete(&svc_insts[i], conn);
	}
}

static void csip_bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
		struct bt_csip_set_member_svc_inst *svc_inst = &svc_insts[i];

		for (size_t j = 0U; j < ARRAY_SIZE(svc_inst->clients); j++) {

			/* Check if match, and if active, if so, reset */
			if (atomic_test_bit(svc_inst->clients[i].flags, FLAG_ACTIVE) &&
			    bt_addr_le_eq(peer, &svc_inst->clients[i].addr)) {
				atomic_clear(svc_inst->clients[i].flags);
				(void)memset(&svc_inst->clients[i].addr, 0, sizeof(bt_addr_le_t));
				break;
			}
		}
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = csip_disconnected,
	.security_changed = csip_security_changed,
};

static struct bt_conn_auth_info_cb auth_callbacks = {
	.pairing_complete = auth_pairing_complete,
	.bond_deleted = csip_bond_deleted
};

#if defined(CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE)
#define BT_CSIS_CHR_SIRK(_csip)                                                                    \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SIRK, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,                  \
		      BT_GATT_PERM_READ_ENCRYPT, read_sirk, NULL, &_csip),                         \
		BT_AUDIO_CCC(sirk_cfg_changed)
#else
#define BT_CSIS_CHR_SIRK(_csip)                                                                    \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SIRK, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT, read_sirk,  \
		      NULL, &_csip)
#endif /* CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE */

#if defined(CONFIG_BT_CSIP_SET_MEMBER_SIZE_NOTIFIABLE)
#define BT_CSIS_CHR_SIZE(_csip)                                                                    \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SET_SIZE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,              \
		      BT_GATT_PERM_READ_ENCRYPT, read_set_size, NULL, &_csip),                     \
		BT_AUDIO_CCC(set_size_cfg_changed)
#else
#define BT_CSIS_CHR_SIZE(_csip)                                                                    \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SET_SIZE, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,         \
		      read_set_size, NULL, &_csip)
#endif /* CONFIG_BT_CSIP_SET_MEMBER_SIZE_NOTIFIABLE */

#define BT_CSIP_SERVICE_DEFINITION(_csip) {\
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSIS), \
	BT_CSIS_CHR_SIRK(_csip), \
	BT_CSIS_CHR_SIZE(_csip), \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SET_LOCK, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_set_lock, write_set_lock, &_csip), \
	BT_AUDIO_CCC(set_lock_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_CSIS_RANK, \
		      BT_GATT_CHRC_READ, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_rank, NULL, &_csip) \
	}

BT_GATT_SERVICE_INSTANCE_DEFINE(csip_set_member_service_list, svc_insts,
				CONFIG_BT_CSIP_SET_MEMBER_MAX_INSTANCE_COUNT,
				BT_CSIP_SERVICE_DEFINITION);

/****************************** Public API ******************************/
void *bt_csip_set_member_svc_decl_get(const struct bt_csip_set_member_svc_inst *svc_inst)
{
	if (svc_inst == NULL || svc_inst->service_p == NULL) {
		return NULL;
	}

	return svc_inst->service_p->attrs;
}

static bool valid_register_param(const struct bt_csip_set_member_register_param *param)
{
	if (param->lockable && param->rank == 0) {
		LOG_DBG("Rank cannot be 0 if service is lockable");
		return false;
	}

	if (param->rank > 0 && param->set_size > 0 && param->rank > param->set_size) {
		LOG_DBG("Invalid rank: %u (shall be less than or equal to set_size: %u)",
			param->rank, param->set_size);
		return false;
	}

#if CONFIG_BT_CSIP_SET_MEMBER_MAX_INSTANCE_COUNT > 1
	if (param->parent == NULL) {
		LOG_DBG("Parent service not provided");
		return false;
	}
#endif /* CONFIG_BT_CSIP_SET_MEMBER_MAX_INSTANCE_COUNT > 1 */

	return true;
}

static void remove_csis_char(const struct bt_uuid *uuid, struct bt_gatt_service *svc)
{
	size_t attrs_to_rem;

	/* Rank does not have any CCCD */
	if (bt_uuid_cmp(uuid, BT_UUID_CSIS_RANK) == 0) {
		attrs_to_rem = CSIS_RANK_CHAR_ATTR_COUNT;
	} else {
		attrs_to_rem = CSIS_CHAR_ATTR_COUNT;
	}

	/* Start at index 4 as the first 4 attributes are mandatory */
	for (size_t i = 4U; i < svc->attr_count; i++) {
		if (bt_uuid_cmp(svc->attrs[i].uuid, uuid) == 0) {
			/* Remove the characteristic declaration, the characteristic value and
			 * potentially the CCCD. The value declaration will be a i - 1, the
			 * characteristic value at i and the CCCD is potentially at i + 1
			 */

			/* We use attrs_to_rem to determine whether there is a CCCD after the
			 * characteristic value or not, which then determines if this is the last
			 * characteristic or not
			 */
			if (i == (svc->attr_count - (attrs_to_rem - 1))) {
				/* This is the last characteristic in the service: just decrement
				 * the attr_count by number of attributes to remove
				 * (CSIS_CHAR_ATTR_COUNT)
				 */
			} else {
				/* Move all following attributes attrs_to_rem locations "up" */
				for (size_t j = i - 1U; j < svc->attr_count - attrs_to_rem; j++) {
					svc->attrs[j] = svc->attrs[j + attrs_to_rem];
				}
			}

			svc->attr_count -= attrs_to_rem;

			return;
		}
	}

	__ASSERT(false, "Failed to remove CSIS char %s", bt_uuid_str(uuid));
}

static void notify(struct bt_csip_set_member_svc_inst *svc_inst, struct bt_conn *conn,
		   const struct bt_uuid *uuid, const void *data, uint16_t len)
{
	int err;
	const struct bt_gatt_attr *attr;

	attr = bt_gatt_find_by_uuid(
		svc_inst->service_p->attrs,
		svc_inst->service_p->attr_count,
		uuid);

	if (attr == NULL) {
		LOG_WRN("Attribute for UUID %p not found", uuid);
		return;
	}

	if (!bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY)) {
		LOG_DBG("Connection %p not subscribed to UUID %p", conn, uuid);
		return;
	}

	err = bt_gatt_notify(conn, attr, data, len);
	if (err) {
		if (err == -ENOTCONN) {
			LOG_DBG("Notification error: ENOTCONN (%d)", err);
		} else {
			LOG_ERR("Notification error: %d", err);
		}
	}
}

static void notify_cb(struct bt_conn *conn, void *data)
{
	struct bt_conn_info info;
	int err = 0;

	err = bt_conn_get_info(conn, &info);
	if (err != 0) {
		return;
	}

	if (info.state != BT_CONN_STATE_CONNECTED) {
		/* Not connected */
		LOG_DBG("Not connected: %u", info.state);
		return;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
		struct bt_csip_set_member_svc_inst *svc_inst = &svc_insts[i];
		struct csip_client *client;
		bool client_found = false;

		err = k_mutex_lock(&svc_inst->mutex, K_NO_WAIT);
		if (err != 0) {
			LOG_DBG("Mutex lock failed (%d) for svc_inst[%zu], rescheduling", err, i);
			notify_work_reschedule(K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
			continue;
		}

		if (svc_inst->service_p == NULL || svc_inst->service_p->attrs == NULL) {
			goto unlock_and_return;
		}

		/* find the client object for the connection */
		for (size_t j = 0U; j < ARRAY_SIZE(svc_inst->clients); j++) {

			client = &svc_inst->clients[j];

			if (atomic_test_bit(client->flags, FLAG_ACTIVE) &&
			    bt_addr_le_eq(bt_conn_get_dst(conn), &client->addr)) {
				client_found = true;
				break;
			}
		}

		if (client_found == false) {
			goto unlock_and_return;
		}

		if (atomic_test_and_clear_bit(client->flags, FLAG_NOTIFY_LOCK)) {
			notify(svc_inst, conn, BT_UUID_CSIS_SET_LOCK, &svc_inst->set_lock,
			       sizeof(svc_inst->set_lock));
		}

		if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE) &&
		    atomic_test_and_clear_bit(client->flags, FLAG_NOTIFY_SIRK)) {
			notify(svc_inst, conn, BT_UUID_CSIS_SIRK, &svc_inst->sirk,
			       sizeof(svc_inst->sirk));
		}

		if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_SIZE_NOTIFIABLE) &&
		    atomic_test_and_clear_bit(client->flags, FLAG_NOTIFY_SIZE)) {
			notify(svc_inst, conn, BT_UUID_CSIS_SET_SIZE, &svc_inst->set_size,
			       sizeof(svc_inst->set_size));
		}

unlock_and_return:
		err = k_mutex_unlock(&svc_inst->mutex);
		__ASSERT(err == 0, "Failed to unlock mutex: %d", err);
	}
}

static void deferred_nfy_work_handler(struct k_work *work)
{
	bt_conn_foreach(BT_CONN_TYPE_LE, notify_cb, NULL);
}

static void add_bonded_addr_to_client_list(const struct bt_bond_info *info, void *data)
{

	for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
		struct bt_csip_set_member_svc_inst *svc_inst = &svc_insts[i];

		for (size_t j = 0U; j < ARRAY_SIZE(svc_inst->clients); j++) {
			/* Check if device is registered, it not, add it */
			if (!atomic_test_bit(svc_inst->clients[j].flags, FLAG_ACTIVE)) {
				char addr_str[BT_ADDR_LE_STR_LEN];

				atomic_set_bit(svc_inst->clients[j].flags, FLAG_ACTIVE);
				memcpy(&svc_inst->clients[j].addr, &info->addr,
				       sizeof(bt_addr_le_t));
				bt_addr_le_to_str(&svc_inst->clients[j].addr, addr_str,
						  sizeof(addr_str));
				LOG_DBG("Added %s to bonded list\n", addr_str);
				return;
			}
		}
	}
}

int bt_csip_set_member_register(const struct bt_csip_set_member_register_param *param,
				struct bt_csip_set_member_svc_inst **svc_inst)
{
	static bool first_register;
	struct bt_csip_set_member_svc_inst *inst;
	int err;

	CHECKIF(param == NULL) {
		LOG_DBG("NULL param");
		return -EINVAL;
	}

	CHECKIF(!valid_register_param(param)) {
		LOG_DBG("Invalid parameters");
		return -EINVAL;
	}

	if (!first_register) {
		for (size_t i = 0U; i < ARRAY_SIZE(svc_insts); i++) {
			k_mutex_init(&svc_insts[i].mutex);
		}
		bt_conn_auth_info_cb_register(&auth_callbacks);
		first_register = true;
	}

	inst = NULL;
	ARRAY_FOR_EACH(svc_insts, i) {
		if (svc_insts[i].service_p == NULL) {
			inst = &svc_insts[i];

			err = k_mutex_lock(&inst->mutex, K_NO_WAIT);
			if (err != 0) {
				/* Try the next */
				continue;
			}

			inst->service_p = &csip_set_member_service_list[i];
			break;
		}
	}

	if (inst == NULL) {
		LOG_DBG("Too many set member registrations");
		return -ENOMEM;
	}

	/* The removal of the optional characteristics should be done in reverse order of the order
	 * in BT_CSIP_SERVICE_DEFINITION, as that improves the performance of remove_csis_char,
	 * since it's easier to remove the last characteristic
	 */
	if (param->rank == 0U) {
		remove_csis_char(BT_UUID_CSIS_RANK, inst->service_p);
	}

	if (param->set_size == 0U) {
		remove_csis_char(BT_UUID_CSIS_SET_SIZE, inst->service_p);
	}

	if (!param->lockable) {
		remove_csis_char(BT_UUID_CSIS_SET_LOCK, inst->service_p);
	}

	err = bt_gatt_service_register(inst->service_p);
	if (err != 0) {
		int mutex_err;

		LOG_DBG("CSIS service register failed: %d", err);
		mutex_err = k_mutex_unlock(&inst->mutex);
		__ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);
		return err;
	}

	k_work_init_delayable(&inst->set_lock_timer,
			      set_lock_timer_handler);
	inst->rank = param->rank;
	inst->set_size = param->set_size;
	inst->set_lock = BT_CSIP_RELEASE_VALUE;
	inst->sirk.type = BT_CSIP_SIRK_TYPE_PLAIN;
	inst->cb = param->cb;
	inst->lockable = param->lockable;
	bt_addr_le_copy(&inst->lock_client_addr, BT_ADDR_LE_NONE);

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_TEST_SAMPLE_DATA)) {
		uint8_t test_sirk[] = {
			0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45,
		};

		(void)memcpy(inst->sirk.value, test_sirk, sizeof(test_sirk));
		LOG_DBG("CSIP SIRK was overwritten by sample data SIRK");
	} else {
		(void)memcpy(inst->sirk.value, param->sirk, sizeof(inst->sirk.value));
	}

	*svc_inst = inst;

	err = k_mutex_unlock(&inst->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return 0;
}

int bt_csip_set_member_unregister(struct bt_csip_set_member_svc_inst *svc_inst)
{
	const struct bt_gatt_attr csis_definition[] = BT_CSIP_SERVICE_DEFINITION(svc_inst);
	int err;

	CHECKIF(svc_inst == NULL) {
		LOG_DBG("NULL svc_inst");
		return -EINVAL;
	}

	err = k_mutex_lock(&svc_inst->mutex, K_NO_WAIT);
	if (err != 0) {
		LOG_DBG("Failed to lock mutex");
		return -EBUSY;
	}

	err = bt_gatt_service_unregister(svc_inst->service_p);
	if (err != 0) {
		int mutex_err;

		LOG_DBG("CSIS service unregister failed: %d", err);
		mutex_err = k_mutex_unlock(&svc_inst->mutex);
		__ASSERT(mutex_err == 0, "Failed to unlock mutex: %d", mutex_err);

		return err;
	}

	/* Restore original declaration */
	(void)memcpy(svc_inst->service_p->attrs, csis_definition, sizeof(csis_definition));
	svc_inst->service_p->attr_count = ARRAY_SIZE(csis_definition);

	(void)k_work_cancel_delayable(&svc_inst->set_lock_timer);

	memset(svc_inst, 0, offsetof(struct bt_csip_set_member_svc_inst, mutex));

	err = k_mutex_unlock(&svc_inst->mutex);
	__ASSERT(err == 0, "Failed to unlock mutex: %d", err);

	return 0;
}

int bt_csip_set_member_sirk(struct bt_csip_set_member_svc_inst *svc_inst,
			    const uint8_t sirk[BT_CSIP_SIRK_SIZE])
{
	CHECKIF(svc_inst == NULL) {
		LOG_DBG("NULL svc_inst");
		return -EINVAL;
	}

	CHECKIF(sirk == NULL) {
		LOG_DBG("NULL SIRK");
		return -EINVAL;
	}

	if (memcmp(sirk, svc_inst->sirk.value, BT_CSIP_SIRK_SIZE) != 0) {
		memcpy(svc_inst->sirk.value, sirk, BT_CSIP_SIRK_SIZE);

		if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE)) {
			notify_clients(svc_inst, NULL, FLAG_NOTIFY_SIRK);
		}
	}

	return 0;
}

int bt_csip_set_member_set_size_and_rank(struct bt_csip_set_member_svc_inst *svc_inst, uint8_t size,
					 uint8_t rank)
{
	if (svc_inst == NULL) {
		LOG_DBG("svc_inst is NULL");
		return -EINVAL;
	}

	if (size < 1U) {
		LOG_DBG("Invalid set size %u", size);
		return -EINVAL;
	}

	if (!svc_inst->lockable && rank != 0U) {
		LOG_DBG("Invalid rank %u for non-lockable service", rank);
		return -EINVAL;
	}

	if (svc_inst->lockable && !IN_RANGE(rank, 1U, size)) {
		LOG_DBG("Invalid rank: %u for size %u", rank, size);
		return -EINVAL;
	}

	if (svc_inst->set_size == size && svc_inst->rank == rank) {
		LOG_DBG("Set size %u and rank %u is already set", size, rank);
		return -EALREADY;
	}

	svc_inst->set_size = size;
	svc_inst->rank = svc_inst->lockable ? rank : 0U;

	if (IS_ENABLED(CONFIG_BT_CSIP_SET_MEMBER_SIZE_NOTIFIABLE)) {
		notify_clients(svc_inst, NULL, FLAG_NOTIFY_SIZE);
	}

	return 0;
}

int bt_csip_set_member_get_info(const struct bt_csip_set_member_svc_inst *svc_inst,
				struct bt_csip_set_member_set_info *info)
{
	if (svc_inst == NULL) {
		LOG_DBG("svc_inst is NULL");
		return -EINVAL;
	}

	if (info == NULL) {
		LOG_DBG("info is NULL");
		return -EINVAL;
	}

	info->lockable = svc_inst->lockable;
	info->locked = svc_inst->set_lock == BT_CSIP_LOCK_VALUE;
	info->rank = svc_inst->rank;
	info->set_size = svc_inst->set_size;
	memcpy(info->sirk, svc_inst->sirk.value, BT_CSIP_SIRK_SIZE);
	bt_addr_le_copy(&info->lock_client_addr, &svc_inst->lock_client_addr);

	return 0;
}

int bt_csip_set_member_lock(struct bt_csip_set_member_svc_inst *svc_inst,
			    bool lock, bool force)
{
	uint8_t lock_val;
	int err = 0;

	if (lock) {
		lock_val = BT_CSIP_LOCK_VALUE;
	} else {
		lock_val = BT_CSIP_RELEASE_VALUE;
	}

	if (!lock && force) {
		svc_inst->set_lock = BT_CSIP_RELEASE_VALUE;
		notify_clients(svc_inst, NULL, FLAG_NOTIFY_LOCK);

		if (svc_inst->cb != NULL && svc_inst->cb->lock_changed != NULL) {
			svc_inst->cb->lock_changed(NULL, &svc_insts[0], false);
		}
	} else {
		err = set_lock(NULL, svc_inst, lock_val);
	}

	if (err < 0) {
		return BT_GATT_ERR(err);
	} else {
		return 0;
	}
}
