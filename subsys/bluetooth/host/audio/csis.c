/*  Bluetooth csis - Telephone Bearer Service */

/*
 * Copyright (c) 2019 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include <device.h>
#include <init.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/buf.h>
#include <sys/byteorder.h>
#include "csis.h"
#include "csip.h"
#include "csis_crypto.h"
#include "../conn_internal.h"
#include "../hci_core.h"
#include "../keys.h"

#define BT_CSIS_SIH_PRAND_SIZE          3
#define BT_CSIS_SIH_HASH_SIZE           3
#define CSIS_SET_LOCK_TIMER_VALUE       K_SECONDS(60)
#if defined(CONFIG_BT_PRIVACY)
/* The ADV time (in tens of milliseconds). Shall be less than the RPA.
 * Make it relatively smaller (90%) to handle all ranges. Maxmimum value is
 * 2^16 - 1 (UINT16_MAX).
 */
#define CSIS_ADV_TIME  (MIN((CONFIG_BT_RPA_TIMEOUT * 100 * 0.9), UINT16_MAX))
#else
/* Without privacy, connectable adv won't update the address when restarting,
 * so we might as well continue advertising non-stop.
 */
#define CSIS_ADV_TIME  0
#endif /* CONFIG_BT_PRIVACY */

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CSIS)
#define LOG_MODULE_NAME bt_csis
#include "common/log.h"

#if defined(CONFIG_BT_RPA) && !defined(CONFIG_BT_BONDABLE)
#define SIRK_READ_PERM	(BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_READ_ENCRYPT)
#else
#define SIRK_READ_PERM	(BT_GATT_PERM_READ_ENCRYPT)
#endif

/* 16-byte key used to generate SIRKs.
 * This key has to be the same for all devices in the set.
 */
static uint8_t set_sirk_key_gen_key[16] = {
	0x92, 0x5f, 0xcb, 0xcb, 0x8a, 0xa8, 0x96, 0xe9,
	0x3e, 0x62, 0x01, 0x54, 0xf9, 0xad, 0xef, 0x54
};

static struct bt_csis_cb_t *csis_cbs;
struct csis_pending_notifications_t {
	bt_addr_le_t addr;
	bool pending;
	bool active;

/* Since there's a 1-to-1 connection between bonded devices, and devices in
 * the array containing this struct, if the security manager overwrites
 * the oldest keys, we also overwrite the oldest entry
 */
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	uint32_t age;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
};

struct csis_instance_t {
	struct bt_csip_set_sirk_t set_sirk;
	uint8_t psri[BT_CSIS_PSRI_SIZE];
	uint8_t set_size;
	uint8_t set_lock;
	uint8_t rank;
	bool adv_enabled;
	struct k_work_delayable set_lock_timer;
	bt_addr_le_t lock_client_addr;
	const struct bt_gatt_service_static *service_p;
	struct csis_pending_notifications_t pend_notify[CONFIG_BT_MAX_PAIRED];
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	uint32_t age_counter;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
#if defined(CONFIG_BT_EXT_ADV)
	uint8_t conn_cnt;
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_cb adv_cb;
	struct k_work work;
#endif /* CONFIG_BT_EXT_ADV */
};

static struct csis_instance_t csis_inst;
extern const struct bt_gatt_service_static csis_svc;
static bt_addr_le_t server_dummy_addr; /* 0'ed address */

struct csis_cond_check_t {
	const bt_addr_le_t *addr;
	bool bonded;
};

static void check_bond(const struct bt_bond_info *info, void *data)
{
	struct csis_cond_check_t *bond_check = (struct csis_cond_check_t *)data;

	if (!bond_check->bonded) {
		bond_check->bonded = !bt_addr_le_cmp(bond_check->addr,
						     &info->addr);
	}
}

static bool is_bonded(struct bt_conn *conn)
{
	struct csis_cond_check_t bond_check = {
		.addr = bt_conn_get_dst(conn),
		.bonded = false
	};

	bt_foreach_bond(BT_ID_DEFAULT, check_bond, &bond_check);
	return bond_check.bonded;
}

static bool is_last_client_to_write(struct bt_conn *conn)
{
	if (conn) {
		return !bt_addr_le_cmp(bt_conn_get_dst(conn),
				       &csis_inst.lock_client_addr);
	} else {
		return !bt_addr_le_cmp(&server_dummy_addr,
				       &csis_inst.lock_client_addr);
	}
}

static void notify_lock_value(struct bt_conn *conn)
{
	bt_gatt_notify_uuid(conn, BT_UUID_CSIS_SET_LOCK, csis_svc.attrs,
			    &csis_inst.set_lock, sizeof(csis_inst.set_lock));
}

static void notify_client(struct bt_conn *conn, void *data)
{
	bt_addr_le_t *addr;
	struct bt_conn *excluded_conn = (struct bt_conn *)data;

	if (excluded_conn && conn == excluded_conn) {
		return;
	}

	notify_lock_value(conn);

	for (int i = 0; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		addr = &csis_inst.pend_notify[i].addr;
		if (csis_inst.pend_notify[i].pending &&
		    !bt_addr_le_cmp(bt_conn_get_dst(conn), addr)) {
			csis_inst.pend_notify[i].pending = false;
			break;
		}
	}
}

static void notify_clients(struct bt_conn *excluded_client)
{
	bt_addr_le_t *addr;

	/* Mark all bonded devices as pending notifications, and clear those
	 * that are notified in `notify_client`
	 */
	for (int i = 0; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		if (csis_inst.pend_notify[i].active) {
			addr = &csis_inst.pend_notify[i].addr;
			if (excluded_client &&
			    !bt_addr_le_cmp(bt_conn_get_dst(excluded_client),
					    addr)) {
				continue;
			}

			csis_inst.pend_notify[i].pending = true;
		}
	}
	bt_conn_foreach(BT_CONN_TYPE_ALL, notify_client, excluded_client);
}

static int sirk_encrypt(struct bt_conn *conn,
			const struct bt_csip_set_sirk_t *sirk,
			struct bt_csip_set_sirk_t *enc_sirk)
{
	int err;
	uint8_t *k;

	if (IS_ENABLED(CONFIG_BT_CSIS_TEST_SAMPLE_DATA)) {
		/* test_k is from the sample data from A.2 in the CSIS spec */
		static uint8_t test_k[] = {0x67, 0x6e, 0x1b, 0x9b,
					   0xd4, 0x48, 0x69, 0x6f,
					   0x06, 0x1e, 0xc6, 0x22,
					   0x3c, 0xe5, 0xce, 0xd9};
		static bool swapped;

		if (!swapped && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
			/* Swap test_k to little endian */
			sys_mem_swap(test_k, 16);
			swapped = true;
		}
		BT_DBG("Encrypting test SIRK");
		k = test_k;
	} else {
		k = conn->le.keys->ltk.val;
	}

	err = bt_csis_sef(k, sirk->value, enc_sirk->value);

	if (err) {
		return err;
	}

	enc_sirk->type = BT_CSIP_SIRK_TYPE_ENCRYPTED;

	return 0;
}

static int generate_sirk(uint32_t seed, uint8_t sirk_dest[16])
{
	int err;

	/* r' = padding || r */
	memcpy(sirk_dest, &seed, sizeof(seed));
	memset(sirk_dest + sizeof(seed), 0, 16 - sizeof(seed));

	err = bt_encrypt_le(set_sirk_key_gen_key, sirk_dest, sirk_dest);
	if (err) {
		return err;
	}

	return 0;
}

static int generate_prand(uint32_t *dest)
{
	int res;
	bool valid = false;

	do {
		*dest = 0;
		res = bt_rand(dest, BT_CSIS_SIH_PRAND_SIZE);
		if (res) {
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

static int csis_update_psri(void)
{
	int res = 0;
	uint32_t prand;
	uint32_t hash;

	if (IS_ENABLED(CONFIG_BT_CSIS_TEST_SAMPLE_DATA)) {
		prand = 0x69f563;
	} else {
		res = generate_prand(&prand);

		if (res) {
			BT_WARN("Could not generate new prand");
			return res;
		}
	}

	res = bt_csis_sih(csis_inst.set_sirk.value, prand, &hash);
	if (res) {
		BT_WARN("Could not generate new PSRI");
		return res;
	}

	memcpy(csis_inst.psri, &hash, 3);
	memcpy(csis_inst.psri + 3, &prand, 3);
	return res;
}

int csis_adv_resume(void)
{
	int err;
	struct bt_data ad[2] = {
		BT_DATA_BYTES(BT_DATA_FLAGS,
				BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)
	};

	BT_DBG("Restarting CSIS advertising");

	if (csis_update_psri() != 0) {
		return -EAGAIN;
	}

	ad[1].type = BT_DATA_CSIS_RSI;
	ad[1].data_len = sizeof(csis_inst.psri);
	ad[1].data = csis_inst.psri;

#if defined(CONFIG_BT_EXT_ADV)
	struct bt_le_ext_adv_start_param param;
	if (!csis_inst.adv) {
		struct bt_le_adv_param param;

		memset(&param, 0, sizeof(param));
		param.options |= BT_LE_ADV_OPT_CONNECTABLE;
		param.options |= BT_LE_ADV_OPT_SCANNABLE;
		param.options |= BT_LE_ADV_OPT_USE_NAME;

		param.id = BT_ID_DEFAULT;
		param.sid = 0;
		param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
		param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

		err = bt_le_ext_adv_create(&param, &csis_inst.adv_cb,
						&csis_inst.adv);
		if (err) {
			BT_DBG("Could not create adv set: %d", err);
			return err;
		}
	}

	err = bt_le_ext_adv_set_data(csis_inst.adv, ad, ARRAY_SIZE(ad),
					NULL, 0);

	if (err) {
		BT_DBG("Could not set adv data: %d", err);
		return err;
	}

	memset(&param, 0, sizeof(param));
	param.timeout = CSIS_ADV_TIME;
	err = bt_le_ext_adv_start(csis_inst.adv, &param);
#else
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME,
				ad, ARRAY_SIZE(ad), NULL, 0);
#endif /* CONFIG_BT_EXT_ADV */

	if (err) {
		BT_DBG("Could not start adv: %d", err);
		return err;
	}

	return err;
}

static ssize_t read_set_sirk(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_csip_set_sirk_t enc_sirk;
	struct bt_csip_set_sirk_t *sirk;

	if (csis_cbs && csis_cbs->sirk_read_req) {
		uint8_t cb_rsp;

		/* Ask higher layer for what SIRK to return, if any */
		cb_rsp = csis_cbs->sirk_read_req(conn);

		if (cb_rsp == BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT) {
			sirk = &csis_inst.set_sirk;
		} else if (IS_ENABLED(CONFIG_BT_CSIS_ENC_SIRK_SUPPORT) &&
			   cb_rsp == BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT_ENC) {
			int err;

			err = sirk_encrypt(conn, &csis_inst.set_sirk,
					   &enc_sirk);
			if (err) {
				BT_ERR("Could not encrypt SIRK: %d",
					err);
				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}
			sirk = &enc_sirk;
			BT_HEXDUMP_DBG(enc_sirk.value, sizeof(enc_sirk.value),
				       "Encrypted Set SIRK");
		} else if (cb_rsp == BT_CSIS_READ_SIRK_REQ_RSP_REJECT) {
			return BT_GATT_ERR(BT_CSIP_ERROR_SIRK_ACCESS_REJECTED);
		} else if (cb_rsp == BT_CSIS_READ_SIRK_REQ_RSP_OOB_ONLY) {
			return BT_GATT_ERR(BT_CSIP_ERROR_SIRK_OOB_ONLY);
		} else {
			BT_ERR("Invalid callback response: %u", cb_rsp);
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	} else {
		sirk = &csis_inst.set_sirk;
	}

	BT_DBG("Set sirk %sencrypted",
	       sirk->type ==  BT_CSIP_SIRK_TYPE_PLAIN ? "not " : "");
	BT_HEXDUMP_DBG(csis_inst.set_sirk.value,
		       sizeof(csis_inst.set_sirk.value), "Set SIRK");
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
	BT_DBG("%u", csis_inst.set_size);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csis_inst.set_size,
				 sizeof(csis_inst.set_size));
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
	BT_DBG("%u", csis_inst.set_lock);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csis_inst.set_lock,
				 sizeof(csis_inst.set_lock));
}

static ssize_t write_set_lock(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len,
			      uint16_t offset, uint8_t flags)
{
	uint8_t val;
	bool notify;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	} else if (len != sizeof(val)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&val, buf, len);

	if (val != BT_CSIP_RELEASE_VALUE && val != BT_CSIP_LOCK_VALUE) {
		return BT_GATT_ERR(BT_CSIP_ERROR_LOCK_INVAL_VALUE);
	}

	if (csis_inst.set_lock == BT_CSIP_LOCK_VALUE) {
		if (val == BT_CSIP_LOCK_VALUE) {
			if (is_last_client_to_write(conn)) {
				return BT_GATT_ERR(
					BT_CSIP_ERROR_LOCK_ALREADY_GRANTED);
			} else {
				return BT_GATT_ERR(BT_CSIP_ERROR_LOCK_DENIED);
			}
		} else if (!is_last_client_to_write(conn)) {
			return BT_GATT_ERR(BT_CSIP_ERROR_LOCK_RELEASE_DENIED);
		}
	}

	notify = csis_inst.set_lock != val;

	csis_inst.set_lock = val;
	if (csis_inst.set_lock == BT_CSIP_LOCK_VALUE) {
		if (conn) {
			bt_addr_le_copy(&csis_inst.lock_client_addr,
					bt_conn_get_dst(conn));
		}
		(void)k_work_reschedule(&csis_inst.set_lock_timer,
					CSIS_SET_LOCK_TIMER_VALUE);
	} else {
		memset(&csis_inst.lock_client_addr, 0,
		       sizeof(csis_inst.lock_client_addr));
		(void)k_work_cancel_delayable(&csis_inst.set_lock_timer);
	}

	BT_DBG("%u", csis_inst.set_lock);

	if (notify) {
		/*
		 * The Spec states that all clients, except for the
		 * client writing the value, shall be notified
		 * (if subscribed)
		 */
		notify_clients(conn);

		if (csis_cbs && csis_cbs->lock_changed) {
			bool locked = csis_inst.set_lock == BT_CSIP_LOCK_VALUE;

			csis_cbs->lock_changed(conn, locked);
		}
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
	BT_DBG("%u", csis_inst.rank);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &csis_inst.rank, sizeof(csis_inst.rank));

}

static void set_lock_timer_handler(struct k_work *work)
{
	BT_DBG("Lock timeout, releasing");
	csis_inst.set_lock = BT_CSIP_RELEASE_VALUE;
	notify_clients(NULL);

	if (csis_cbs && csis_cbs->lock_changed) {
		bool locked = csis_inst.set_lock == BT_CSIP_LOCK_VALUE;

		csis_cbs->lock_changed(NULL, locked);
	}
}

static void csis_security_changed(struct bt_conn *conn, bt_security_t level,
				  enum bt_security_err err)
{
	bt_addr_le_t *addr;

	if (!is_bonded(conn)) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		addr = &csis_inst.pend_notify[i].addr;
		if (csis_inst.pend_notify[i].pending &&
		    !bt_addr_le_cmp(bt_conn_get_dst(conn), addr)) {
			notify_lock_value(conn);
			csis_inst.pend_notify[i].pending = false;
			break;
		}
	}
}

#if defined(CONFIG_BT_EXT_ADV)
static void csis_connected(struct bt_conn *conn, uint8_t reason)
{
	csis_inst.conn_cnt++;

	__ASSERT(csis_inst.conn_cnt <= CONFIG_BT_MAX_CONN,
		 "Invalid csis_inst.conn_cnt value");
}

static void disconnect_adv(struct k_work *work)
{
	int err = csis_adv_resume();

	if (err) {
		BT_ERR("Disconnect: Could not restart advertising: %d",
			err);
		csis_inst.adv_enabled = false;
	}
}
#endif /* CONFIG_BT_EXT_ADV */

static void csis_disconnected(struct bt_conn *conn, uint8_t reason)
{
	bt_addr_le_t *addr;

#if defined(CONFIG_BT_EXT_ADV)
	__ASSERT(csis_inst.conn_cnt, "Invalid csis_inst.conn_cnt value");

	if (csis_inst.conn_cnt == CONFIG_BT_MAX_CONN && csis_inst.adv_enabled) {
		/* A connection spot opened up */
		k_work_submit(&csis_inst.work);
	}
	csis_inst.conn_cnt--;
#endif /* CONFIG_BT_EXT_ADV */

	BT_DBG("Disconnected: %s (reason %u)",
	       bt_addr_le_str(bt_conn_get_dst(conn)), reason);

	/*
	 * If lock was taken by non-bonded device, set lock to released value,
	 * and notify other connections.
	 */
	if (is_bonded(conn)) {
		return;
	}

	BT_DBG("Non-bonded device");
	if (is_last_client_to_write(conn)) {
		memset(&csis_inst.lock_client_addr, 0,
		sizeof(csis_inst.lock_client_addr));
		csis_inst.set_lock = BT_CSIP_RELEASE_VALUE;
		notify_clients(NULL);

		if (csis_cbs && csis_cbs->lock_changed) {
			bool locked = csis_inst.set_lock == BT_CSIP_LOCK_VALUE;

			csis_cbs->lock_changed(conn, locked);
		}
	}

	/* Check if the disconnected device once was bonded and stored
	 * here as a bonded device
	 */
	for (int i = 0; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		addr = &csis_inst.pend_notify[i].addr;
		if (!bt_addr_le_cmp(bt_conn_get_dst(conn), addr)) {
			memset(&csis_inst.pend_notify[i], 0,
			       sizeof(csis_inst.pend_notify[i]));
			break;
		}
	}
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
	bt_addr_le_t *addr;

	BT_DBG("%s paired (%sbonded)",
	       bt_addr_le_str(bt_conn_get_dst(conn)), bonded ? "" : "not ");

	if (!bonded) {
		return;
	}

	/* Check if already in list, and do nothing if it is */
	for (int i = 0; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		addr = &csis_inst.pend_notify[i].addr;
		if (csis_inst.pend_notify[i].active &&
			!bt_addr_le_cmp(bt_conn_get_dst(conn), addr)) {
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
			csis_inst.pend_notify[i].age = csis_inst.age_counter++;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
			return;
		}
	}

	/* Copy addr to list over devices to save notifications for */
	for (int i = 0; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		addr = &csis_inst.pend_notify[i].addr;
		if (!csis_inst.pend_notify[i].active) {
			bt_addr_le_copy(addr, bt_conn_get_dst(conn));
			csis_inst.pend_notify[i].active = true;
#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
			csis_inst.pend_notify[i].age = csis_inst.age_counter++;
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
			return;
		}
	}

#if IS_ENABLED(CONFIG_BT_KEYS_OVERWRITE_OLDEST)
	struct csis_pending_notifications_t *oldest = &csis_inst.pend_notify[0];

	for (int i = 1; i < ARRAY_SIZE(csis_inst.pend_notify); i++) {
		if (csis_inst.pend_notify[i].age < oldest->age) {
			oldest = &csis_inst.pend_notify[i];
		}
	}
	memset(oldest, 0, sizeof(*oldest));
	bt_addr_le_copy(&oldest->addr, &conn->le.dst);
	oldest->active = true;
	oldest->age = csis_inst.age_counter++;
#else
	BT_WARN("Could not add device to pending notification list");
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
}

static struct bt_conn_cb conn_callbacks = {

#if defined(CONFIG_BT_EXT_ADV)
	.connected = csis_connected,
#endif /* CONFIG_BT_EXT_ADV */
	.disconnected = csis_disconnected,
	.security_changed = csis_security_changed,
};

static const struct bt_conn_auth_cb auth_callbacks = {
	.pairing_complete = auth_pairing_complete
};

#if defined(CONFIG_BT_EXT_ADV)
/* TODO: Temp fix due to bug in adv callbacks */
static bool conn_based_timeout;
static void adv_timeout(struct bt_le_ext_adv *adv,
			struct bt_le_ext_adv_sent_info *info)
{
	int err;

	__ASSERT(adv == csis_inst.adv, "Wrong adv set");

	if (conn_based_timeout) {
		return;
	}
	conn_based_timeout = false;

	/* Restart to update RSI value with new private address */
	if (csis_inst.adv_enabled) {
		err = csis_adv_resume();

		if (err) {
			BT_ERR("Timeout: Could not restart advertising: %d",
			       err);
			csis_inst.adv_enabled = false;
		}
	}
}

static void adv_connected(struct bt_le_ext_adv *adv,
			  struct bt_le_ext_adv_connected_info *info)
{
	__ASSERT(adv == csis_inst.adv, "Wrong adv set");

	if (csis_inst.conn_cnt < CONFIG_BT_MAX_CONN && csis_inst.adv_enabled) {
		int err = csis_adv_resume();

		if (err) {
			BT_ERR("Connected: Could not restart advertising: %d",
			       err);
			csis_inst.adv_enabled = false;
		}
	}

	conn_based_timeout = true;
}
#endif /* CONFIG_BT_EXT_ADV */

#define BT_CSIS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_CSIS), \
	BT_GATT_CHARACTERISTIC(BT_UUID_CSIS_SET_SIRK, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			SIRK_READ_PERM, \
			read_set_sirk, NULL, NULL), \
	BT_GATT_CCC(set_sirk_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_CSIS_SET_SIZE, \
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_set_size, NULL, NULL), \
	BT_GATT_CCC(set_size_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_CSIS_SET_LOCK, \
			BT_GATT_CHRC_READ | \
				BT_GATT_CHRC_NOTIFY | \
				BT_GATT_CHRC_WRITE, \
			BT_GATT_PERM_READ_ENCRYPT | \
				BT_GATT_PERM_WRITE_ENCRYPT, \
			read_set_lock, write_set_lock, NULL), \
	BT_GATT_CCC(set_lock_cfg_changed, \
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_CSIS_RANK, \
			BT_GATT_CHRC_READ, \
			BT_GATT_PERM_READ_ENCRYPT, \
			read_rank, NULL, NULL) \


BT_GATT_SERVICE_DEFINE(csis_svc, BT_CSIS_SERVICE_DEFINITION);

static int bt_csis_init(const struct device *unused)
{
	int res = 0;

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_callbacks);

	k_work_init_delayable(&csis_inst.set_lock_timer,
			      set_lock_timer_handler);
	csis_inst.service_p = &csis_svc;
	csis_inst.rank = CONFIG_BT_CSIS_SET_RANK;
	csis_inst.set_size = CONFIG_BT_CSIS_SET_SIZE;
	csis_inst.set_lock = BT_CSIP_RELEASE_VALUE;
	csis_inst.set_sirk.type = BT_CSIP_SIRK_TYPE_PLAIN;

	if (IS_ENABLED(CONFIG_BT_CSIS_TEST_SAMPLE_DATA)) {
		uint8_t test_sirk[] = {
			0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45,
		};

		memcpy(csis_inst.set_sirk.value, test_sirk, sizeof(test_sirk));
	} else {
		res = generate_sirk(CONFIG_BT_CSIS_SET_SIRK_SEED,
				csis_inst.set_sirk.value);
		if (res) {
			BT_DBG("Sirk generation failed for instance");
		}
	}

#if defined(CONFIG_BT_EXT_ADV)
	csis_inst.adv_cb.sent = adv_timeout;
	csis_inst.adv_cb.connected = adv_connected;
	k_work_init(&csis_inst.work, disconnect_adv);
#endif /* CONFIG_BT_EXT_ADV */

	return res;
}

DEVICE_DEFINE(bt_csis, "bt_csis", &bt_csis_init, NULL, NULL, NULL,
	      APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

/****************************** Public API ******************************/
void bt_csis_register_cb(struct bt_csis_cb_t *cb)
{
	csis_cbs = cb;
}

int bt_csis_advertise(bool enable)
{
	int err;

	if (enable) {
		if (csis_inst.adv_enabled) {
			return -EALREADY;
		}

		err = csis_adv_resume();

		if (err) {
			BT_DBG("Could not start adv: %d", err);
			return err;
		}
		csis_inst.adv_enabled = true;
	} else {
		if (!csis_inst.adv_enabled) {
			return -EALREADY;
		}
#if defined(CONFIG_BT_EXT_ADV)
		err = bt_le_ext_adv_stop(csis_inst.adv);
#else
		err = bt_le_adv_stop();
#endif /* CONFIG_BT_EXT_ADV */

		if (err) {
			BT_DBG("Could not stop start adv: %d", err);
			return err;
		}
		csis_inst.adv_enabled = false;
	}

	return err;
}

int bt_csis_lock(bool lock, bool force)
{
	uint8_t lock_val;
	int err = 0;

	if (lock) {
		lock_val = BT_CSIP_LOCK_VALUE;
	} else {
		lock_val = BT_CSIP_RELEASE_VALUE;
	}

	if (!lock && force) {
		csis_inst.set_lock = BT_CSIP_RELEASE_VALUE;
		notify_clients(NULL);

		if (csis_cbs && csis_cbs->lock_changed) {
			csis_cbs->lock_changed(NULL, false);
		}
	} else {
		err = write_set_lock(NULL, NULL, &lock_val,
				     sizeof(lock_val), 0, 0);
	}

	if (err < 0) {
		return err;
	} else {
		return 0;
	}
}

void bt_csis_print_sirk(void)
{
	BT_HEXDUMP_DBG(&csis_inst.set_sirk, sizeof(csis_inst.set_sirk),
			"Set SIRK");
}

#if defined(CONFIG_BT_TESTING)
int bt_csis_test_set_rank(uint8_t rank)
{
	if (rank > CONFIG_BT_CSIS_SET_SIZE) {
		return -EINVAL;
	}

	csis_inst.rank = rank;

	return 0;
}
#endif /* CONFIG_BT_TESTING */
