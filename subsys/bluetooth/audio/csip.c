/*  Bluetooth CSIP - Coordinated Set Identification Profile */

/*
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdlib.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include "audio_internal.h"
#include "csip_internal.h"
#include "csip_crypto.h"
#include "../host/conn_internal.h"
#include "../host/hci_core.h"
#include "../host/keys.h"

#define BT_CSIP_SIH_PRAND_SIZE          3
#define BT_CSIP_SIH_HASH_SIZE           3
#define CSIP_SET_LOCK_TIMER_VALUE       K_SECONDS(60)

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CSIP)
#define LOG_MODULE_NAME bt_csip
#include "common/log.h"
#include "common/bt_str.h"

static struct bt_csip csip_insts[CONFIG_BT_CSIP_MAX_INSTANCE_COUNT];
static bt_addr_le_t server_dummy_addr; /* 0'ed address */

struct csip_notify_foreach {
	struct bt_conn *excluded_client;
	struct bt_csip *csip;
};

static bool is_last_client_to_write(const struct bt_csip *csip,
				    const struct bt_conn *conn)
{
	if (conn != NULL) {
		return bt_addr_le_eq(bt_conn_get_dst(conn),
				     &csip->srv.lock_client_addr);
	} else {
		return bt_addr_le_eq(&server_dummy_addr,
				     &csip->srv.lock_client_addr);
	}
}

static void notify_lock_value(const struct bt_csip *csip, struct bt_conn *conn)
{
	bt_gatt_notify_uuid(conn, BT_UUID_CSIS_SET_LOCK,
			    csip->srv.service_p->attrs,
			    &csip->srv.set_lock,
			    sizeof(csip->srv.set_lock));
}

static void notify_client(struct bt_conn *conn, void *data)
{
	struct csip_notify_foreach *csip_data = (struct csip_notify_foreach *)data;
	struct bt_csip *csip = csip_data->csip;
	struct bt_conn *excluded_conn = csip_data->excluded_client;

	if (excluded_conn != NULL && conn == excluded_conn) {
		return;
	}

	notify_lock_value(csip, conn);

	for (int i = 0; i < ARRAY_SIZE(csip->srv.pend_notify); i++) {
		struct csip_pending_notifications *pend_notify;

		pend_notify = &csip->srv.pend_notify[i];

		if (pend_notify->pending &&
		    bt_addr_le_eq(bt_conn_get_dst(conn), &pend_notify->addr)) {
			pend_notify->pending = false;
			break;
		}
	}
}

static void notify_clients(struct bt_csip *csip,
			   struct bt_conn *excluded_client)
{
	struct csip_notify_foreach data = {
		.excluded_client = excluded_client,
		.csip = csip,
	};

	/* Mark all bonded devices as pending notifications, and clear those
	 * that are notified in `notify_client`
	 */
	for (int i = 0; i < ARRAY_SIZE(csip->srv.pend_notify); i++) {
		struct csip_pending_notifications *pend_notify;

		pend_notify = &csip->srv.pend_notify[i];

		if (pend_notify->active) {
			if (excluded_client != NULL &&
			    bt_addr_le_eq(bt_conn_get_dst(excluded_client), &pend_notify->addr)) {
				continue;
			}

			pend_notify->pending = true;
		}
	}

	bt_conn_foreach(BT_CONN_TYPE_LE, notify_client, &data);
}

static int sirk_encrypt(struct bt_conn *conn,
			const struct bt_csip_set_sirk *sirk,
			struct bt_csip_set_sirk *enc_sirk)
{
	int err;
	uint8_t *k;

	if (IS_ENABLED(CONFIG_BT_CSIP_TEST_SAMPLE_DATA)) {
		/* test_k is from the sample data from A.2 in the CSIS spec */
		static uint8_t test_k[] = {0x67, 0x6e, 0x1b, 0x9b,
					   0xd4, 0x48, 0x69, 0x6f,
					   0x06, 0x1e, 0xc6, 0x22,
					   0x3c, 0xe5, 0xce, 0xd9};
		static bool swapped;

		if (!swapped && IS_ENABLED(CONFIG_LITTLE_ENDIAN)) {
			/* Swap test_k to little endian */
			sys_mem_swap(test_k, 16);
			swapped = true;
		}
		BT_DBG("Encrypting test SIRK");
		k = test_k;
	} else {
		k = conn->le.keys->ltk.val;
	}

	err = bt_csip_sef(k, sirk->value, enc_sirk->value);

	if (err != 0) {
		return err;
	}

	enc_sirk->type = BT_CSIP_SIRK_TYPE_ENCRYPTED;

	return 0;
}

static int generate_prand(uint32_t *dest)
{
	bool valid = false;

	do {
		int res;

		*dest = 0;
		res = bt_rand(dest, BT_CSIP_SIH_PRAND_SIZE);
		if (res != 0) {
			return res;
		}

		/* Validate Prand: Must contain both a 1 and a 0 */
		if (*dest != 0 && *dest != 0x3FFFFF) {
			valid = true;
		}
	} while (!valid);

	*dest &= 0x3FFFFF;
	*dest |= BIT(22); /* bit 23 shall be 0, and bit 22 shall be 1 */

	return 0;
}

int bt_csip_generate_rsi(const struct bt_csip *csip, uint8_t rsi[BT_CSIP_RSI_SIZE])
{
	int res = 0;
	uint32_t prand;
	uint32_t hash;

	if (IS_ENABLED(CONFIG_BT_CSIP_TEST_SAMPLE_DATA)) {
		/* prand is from the sample data from A.2 in the CSIS spec */
		prand = 0x69f563;
	} else {
		res = generate_prand(&prand);

		if (res != 0) {
			BT_WARN("Could not generate new prand");
			return res;
		}
	}

	res = bt_csip_sih(csip->srv.set_sirk.value, prand, &hash);
	if (res != 0) {
		BT_WARN("Could not generate new RSI");
		return res;
	}

	(void)memcpy(rsi, &hash, BT_CSIP_SIH_HASH_SIZE);
	(void)memcpy(rsi + BT_CSIP_SIH_HASH_SIZE, &prand, BT_CSIP_SIH_PRAND_SIZE);

	return res;
}

static ssize_t read_set_sirk(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip_set_sirk enc_sirk;
	struct bt_csip_set_sirk *sirk;
	struct bt_csip *csip = BT_AUDIO_CHRC_USER_DATA(attr);

	if (csip->srv.cb != NULL && csip->srv.cb->sirk_read_req != NULL) {
		uint8_t cb_rsp;

		/* Ask higher layer for what SIRK to return, if any */
		cb_rsp = csip->srv.cb->sirk_read_req(conn, &csip_insts[0]);

		if (cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT) {
			sirk = &csip->srv.set_sirk;
		} else if (IS_ENABLED(CONFIG_BT_CSIP_ENC_SIRK_SUPPORT) &&
			   cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC) {
			int err;

			err = sirk_encrypt(conn, &csip->srv.set_sirk,
					   &enc_sirk);
			if (err != 0) {
				BT_ERR("Could not encrypt SIRK: %d",
					err);
				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}

			sirk = &enc_sirk;
			LOG_HEXDUMP_DBG(enc_sirk.value, sizeof(enc_sirk.value),
					"Encrypted Set SIRK");
		} else if (cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_REJECT) {
			return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
		} else if (cb_rsp == BT_CSIP_READ_SIRK_REQ_RSP_OOB_ONLY) {
			return BT_GATT_ERR(BT_CSIP_ERROR_SIRK_OOB_ONLY);
		} else {
			BT_ERR("Invalid callback response: %u", cb_rsp);
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	} else {
		sirk = &csip->srv.set_sirk;
	}


	BT_DBG("Set sirk %sencrypted",
	       sirk->type ==  BT_CSIP_SIRK_TYPE_PLAIN ? "not " : "");
	LOG_HEXDUMP_DBG(csip->srv.set_sirk.value,
			sizeof(csip->srv.set_sirk.value), "Set SIRK");
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 sirk, sizeof(*sirk));
}

static void set_sirk_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_set_size(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip *csip = BT_AUDIO_CHRC_USER_DATA(attr);

	BT_DBG("%u", csip->srv.set_size);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csip->srv.set_size,
				 sizeof(csip->srv.set_size));
}

static void set_size_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_set_lock(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip *csip = BT_AUDIO_CHRC_USER_DATA(attr);

	BT_DBG("%u", csip->srv.set_lock);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csip->srv.set_lock,
				 sizeof(csip->srv.set_lock));
}

/**
 * @brief Set the lock value of a CSIP instance.
 *
 * @param conn  The connection locking the instance.
 *              Will be NULL if the server locally sets the lock.
 * @param csip  The CSIP instance to change the lock value of
 * @param val   The lock value (BT_CSIP_LOCK_VALUE or BT_CSIP_RELEASE_VALUE)
 *
 * @return BT_CSIP_ERROR_* on failure or 0 if success
 */
static uint8_t set_lock(struct bt_conn *conn, struct bt_csip *csip, uint8_t val)
{
	bool notify;

	if (val != BT_CSIP_RELEASE_VALUE && val != BT_CSIP_LOCK_VALUE) {
		return BT_CSIP_ERROR_LOCK_INVAL_VALUE;
	}

	if (csip->srv.set_lock == BT_CSIP_LOCK_VALUE) {
		if (val == BT_CSIP_LOCK_VALUE) {
			if (is_last_client_to_write(csip, conn)) {
				return BT_CSIP_ERROR_LOCK_ALREADY_GRANTED;
			} else {
				return BT_CSIP_ERROR_LOCK_DENIED;
			}
		} else if (!is_last_client_to_write(csip, conn)) {
			return BT_CSIP_ERROR_LOCK_RELEASE_DENIED;
		}
	}

	notify = csip->srv.set_lock != val;

	csip->srv.set_lock = val;
	if (csip->srv.set_lock == BT_CSIP_LOCK_VALUE) {
		if (conn != NULL) {
			bt_addr_le_copy(&csip->srv.lock_client_addr,
					bt_conn_get_dst(conn));
		}
		(void)k_work_reschedule(&csip->srv.set_lock_timer,
					CSIP_SET_LOCK_TIMER_VALUE);
	} else {
		(void)memset(&csip->srv.lock_client_addr, 0,
			     sizeof(csip->srv.lock_client_addr));
		(void)k_work_cancel_delayable(&csip->srv.set_lock_timer);
	}

	BT_DBG("%u", csip->srv.set_lock);

	if (notify) {
		/*
		 * The Spec states that all clients, except for the
		 * client writing the value, shall be notified
		 * (if subscribed)
		 */
		notify_clients(csip, conn);

		if (csip->srv.cb != NULL && csip->srv.cb->lock_changed != NULL) {
			bool locked = csip->srv.set_lock == BT_CSIP_LOCK_VALUE;

			csip->srv.cb->lock_changed(conn, csip, locked);
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
	struct bt_csip *csip = BT_AUDIO_CHRC_USER_DATA(attr);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len != sizeof(val)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	(void)memcpy(&val, buf, len);

	res = set_lock(conn, csip, val);
	if (res != BT_ATT_ERR_SUCCESS) {
		return BT_GATT_ERR(res);
	}

	return len;
}

static void set_lock_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_rank(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip *csip = BT_AUDIO_CHRC_USER_DATA(attr);

	BT_DBG("%u", csip->srv.rank);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csip->srv.rank,
				 sizeof(csip->srv.rank));

}

static void set_lock_timer_handler(struct k_work *work)
{
	struct k_work_delayable *delayable;
	struct bt_csip_server *server;
	struct bt_csip *csip;

	delayable = CONTAINER_OF(work, struct k_work_delayable, work);
	server = CONTAINER_OF(delayable, struct bt_csip_server, set_lock_timer);
	csip = CONTAINER_OF(server, struct bt_csip, srv);

	BT_DBG("Lock timeout, releasing");
	csip->srv.set_lock = BT_CSIP_RELEASE_VALUE;
	notify_clients(csip, NULL);

	if (csip->srv.cb != NULL && csip->srv.cb->lock_changed != NULL) {
		bool locked = csip->srv.set_lock == BT_CSIP_LOCK_VALUE;

		csip->srv.cb->lock_changed(NULL, csip, locked);
	}
}

static void csip_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	if (err != 0 || conn->encrypt == 0) {
		return;
	}

	if (!bt_addr_le_is_bonded(conn->id, &conn->le.dst)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(csip_insts); i++) {
		struct bt_csip *csip = &csip_insts[i];

		for (int j = 0; j < ARRAY_SIZE(csip->srv.pend_notify); j++) {
			struct csip_pending_notifications *pend_notify;

			pend_notify = &csip->srv.pend_notify[j];

			if (pend_notify->pending &&
			    bt_addr_le_eq(bt_conn_get_dst(conn), &pend_notify->addr)) {
				notify_lock_value(csip, conn);
				pend_notify->pending = false;
				break;
			}
		}
	}
}

static void handle_csip_disconnect(struct bt_csip *csip, struct bt_conn *conn)
{
	BT_DBG("Non-bonded device");
	if (is_last_client_to_write(csip, conn)) {
		(void)memset(&csip->srv.lock_client_addr, 0,
			     sizeof(csip->srv.lock_client_addr));
		csip->srv.set_lock = BT_CSIP_RELEASE_VALUE;
		notify_clients(csip, NULL);

		if (csip->srv.cb != NULL && csip->srv.cb->lock_changed != NULL) {
			bool locked = csip->srv.set_lock == BT_CSIP_LOCK_VALUE;

			csip->srv.cb->lock_changed(conn, csip, locked);
		}
	}

	/* Check if the disconnected device once was bonded and stored
	 * here as a bonded device
	 */
	for (int i = 0; i < ARRAY_SIZE(csip->srv.pend_notify); i++) {
		struct csip_pending_notifications *pend_notify;

		pend_notify = &csip->srv.pend_notify[i];

		if (bt_addr_le_eq(bt_conn_get_dst(conn), &pend_notify->addr)) {
			(void)memset(pend_notify, 0, sizeof(*pend_notify));
			break;
		}
	}
}

static void csip_disconnected(struct bt_conn *conn, uint8_t reason)
{
	BT_DBG("Disconnected: %s (reason %u)",
	       bt_addr_le_str(bt_conn_get_dst(conn)), reason);

	for (int i = 0; i < ARRAY_SIZE(csip_insts); i++) {
		handle_csip_disconnect(&csip_insts[i], conn);
	}
}

static void handle_csip_auth_complete(struct bt_csip *csip,
				      struct bt_conn *conn)
{
	/* Check if already in list, and do nothing if it is */
	for (int i = 0; i < ARRAY_SIZE(csip->srv.pend_notify); i++) {
		struct csip_pending_notifications *pend_notify;

		pend_notify = &csip->srv.pend_notify[i];

		if (pend_notify->active &&
		    bt_addr_le_eq(bt_conn_get_dst(conn), &pend_notify->addr)) {
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
			pend_notify->age = csip->srv.age_counter++;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
			return;
		}
	}

	/* Copy addr to list over devices to save notifications for */
	for (int i = 0; i < ARRAY_SIZE(csip->srv.pend_notify); i++) {
		struct csip_pending_notifications *pend_notify;

		pend_notify = &csip->srv.pend_notify[i];

		if (!pend_notify->active) {
			bt_addr_le_copy(&pend_notify->addr,
					bt_conn_get_dst(conn));
			pend_notify->active = true;
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
			pend_notify->age = csip->srv.age_counter++;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
			return;
		}
	}

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	struct csip_pending_notifications *oldest;

	oldest = &csip->srv.pend_notify[0];

	for (int i = 1; i < ARRAY_SIZE(csip->srv.pend_notify); i++) {
		struct csip_pending_notifications *pend_notify;

		pend_notify = &csip->srv.pend_notify[i];

		if (pend_notify->age < oldest->age) {
			oldest = pend_notify;
		}
	}
	(void)memset(oldest, 0, sizeof(*oldest));
	bt_addr_le_copy(&oldest->addr, &conn->le.dst);
	oldest->active = true;
	oldest->age = csip->srv.age_counter++;
#else
	BT_WARN("Could not add device to pending notification list");
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */

}

static void auth_pairing_complete(struct bt_conn *conn, bool bonded)
{
	/**
	 * If a pairing is complete for a bonded device, then we
	 * 1) Store the connection pointer to later validate SIRK encryption
	 * 2) Check if the device is already in the `pend_notify`, and if it is
	 * not, then we
	 * 3) Check if there's room for another device in the `pend_notify`
	 *    array. If there are no more room for a new device, then
	 * 4) Either we ignore this new device (bad luck), or we overwrite
	 *    the oldest entry, following the behavior of the key storage.
	 */

	BT_DBG("%s paired (%sbonded)",
	       bt_addr_le_str(bt_conn_get_dst(conn)), bonded ? "" : "not ");

	if (!bonded) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(csip_insts); i++) {
		handle_csip_auth_complete(&csip_insts[i], conn);
	}
}

static void csip_bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	for (int i = 0; i < ARRAY_SIZE(csip_insts); i++) {
		struct bt_csip *csip = &csip_insts[i];

		for (int j = 0; j < ARRAY_SIZE(csip->srv.pend_notify); j++) {
			struct csip_pending_notifications *pend_notify;

			pend_notify = &csip->srv.pend_notify[j];

			if (pend_notify->active &&
			    bt_addr_le_eq(peer, &pend_notify->addr)) {
				(void)memset(pend_notify, 0,
					     sizeof(*pend_notify));
				break;
			}
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
	.disconnected = csip_disconnected,
	.security_changed = csip_security_changed,
};

static struct bt_conn_auth_info_cb auth_callbacks = {
	.pairing_complete = auth_pairing_complete,
	.bond_deleted = csip_bond_deleted
};

#define BT_CSIP_SERVICE_DEFINITION(_csip) {\
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSIS), \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SET_SIRK, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_set_sirk, NULL, &_csip), \
	BT_AUDIO_CCC(set_sirk_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_CSIS_SET_SIZE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_set_size, NULL, &_csip), \
	BT_AUDIO_CCC(set_size_cfg_changed), \
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

BT_GATT_SERVICE_INSTANCE_DEFINE(csip_service_list, csip_insts,
				CONFIG_BT_CSIP_MAX_INSTANCE_COUNT,
				BT_CSIP_SERVICE_DEFINITION);

/****************************** Public API ******************************/
void *bt_csip_svc_decl_get(const struct bt_csip *csip)
{
	return csip->srv.service_p->attrs;
}

static bool valid_register_param(const struct bt_csip_register_param *param)
{
	if (param->lockable && param->rank == 0) {
		BT_DBG("Rank cannot be 0 if service is lockable");
		return false;
	}

	if (param->rank > 0 && param->rank > param->set_size) {
		BT_DBG("Invalid rank: %u (shall be less than set_size: %u)",
		       param->set_size, param->set_size);
		return false;
	}

#if CONFIG_BT_CSIP_MAX_INSTANCE_COUNT > 1
	if (param->parent == NULL) {
		BT_DBG("Parent service not provided");
		return false;
	}
#endif /* CONFIG_BT_CSIP_MAX_INSTANCE_COUNT > 1 */

	return true;
}

int bt_csip_register(const struct bt_csip_register_param *param,
		     struct bt_csip **csip)
{
	static uint8_t instance_cnt;
	struct bt_csip *inst;
	int err;

	if (instance_cnt == ARRAY_SIZE(csip_insts)) {
		return -ENOMEM;
	}

	CHECKIF(param == NULL) {
		BT_DBG("NULL param");
		return -EINVAL;
	}

	CHECKIF(!valid_register_param(param)) {
		BT_DBG("Invalid parameters");
		return -EINVAL;
	}

	inst = &csip_insts[instance_cnt];
	inst->srv.service_p = &csip_service_list[instance_cnt];
	instance_cnt++;

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_info_cb_register(&auth_callbacks);

	err = bt_gatt_service_register(inst->srv.service_p);
	if (err != 0) {
		BT_DBG("CSIS service register failed: %d", err);
		return err;
	}

	k_work_init_delayable(&inst->srv.set_lock_timer,
			      set_lock_timer_handler);
	inst->srv.rank = param->rank;
	inst->srv.set_size = param->set_size;
	inst->srv.set_lock = BT_CSIP_RELEASE_VALUE;
	inst->srv.set_sirk.type = BT_CSIP_SIRK_TYPE_PLAIN;
	inst->srv.cb = param->cb;

	if (IS_ENABLED(CONFIG_BT_CSIP_TEST_SAMPLE_DATA)) {
		uint8_t test_sirk[] = {
			0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45,
		};

		(void)memcpy(inst->srv.set_sirk.value, test_sirk,
			     sizeof(test_sirk));
		BT_DBG("CSIP SIRK was overwritten by sample data SIRK");
	} else {
		(void)memcpy(inst->srv.set_sirk.value, param->set_sirk,
			     sizeof(inst->srv.set_sirk.value));
	}

	*csip = inst;
	return 0;
}

int bt_csip_lock(struct bt_csip *csip, bool lock, bool force)
{
	uint8_t lock_val;
	int err = 0;

	if (lock) {
		lock_val = BT_CSIP_LOCK_VALUE;
	} else {
		lock_val = BT_CSIP_RELEASE_VALUE;
	}

	if (!lock && force) {
		csip->srv.set_lock = BT_CSIP_RELEASE_VALUE;
		notify_clients(csip, NULL);

		if (csip->srv.cb != NULL && csip->srv.cb->lock_changed != NULL) {
			csip->srv.cb->lock_changed(NULL, &csip_insts[0], false);
		}
	} else {
		err = set_lock(NULL, csip, lock_val);
	}

	if (err < 0) {
		return BT_GATT_ERR(err);
	} else {
		return 0;
	}
}

void bt_csip_print_sirk(const struct bt_csip *csip)
{
	LOG_HEXDUMP_DBG(&csip->srv.set_sirk, sizeof(csip->srv.set_sirk),
			"Set SIRK");
}
