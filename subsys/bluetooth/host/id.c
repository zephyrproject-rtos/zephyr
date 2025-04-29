/*
 * Copyright (c) 2017-2025 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/toolchain.h>

#include "adv.h"
#include "common/bt_str.h"
#include "common/rpa.h"
#include "conn_internal.h"
#include "hci_core.h"
#include "id.h"
#include "keys.h"
#include "scan.h"
#include "settings.h"
#include "smp.h"

#define LOG_LEVEL CONFIG_BT_HCI_CORE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_id);

struct bt_adv_id_check_data {
	uint8_t id;
	bool adv_enabled;
};

#if defined(CONFIG_BT_OBSERVER) || defined(CONFIG_BT_BROADCASTER)
const bt_addr_le_t *bt_lookup_id_addr(uint8_t id, const bt_addr_le_t *addr)
{
	CHECKIF(id >= CONFIG_BT_ID_MAX || addr == NULL) {
		return NULL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		struct bt_keys *keys;

		keys = bt_keys_find_irk(id, addr);
		if (keys) {
			LOG_DBG("Identity %s matched RPA %s", bt_addr_le_str(&keys->addr),
				bt_addr_le_str(addr));
			return &keys->addr;
		}
	}

	return addr;
}
#endif /* CONFIG_BT_OBSERVER || CONFIG_BT_CONN */

static void adv_id_check_func(struct bt_le_ext_adv *adv, void *data)
{
	struct bt_adv_id_check_data *check_data = data;

	if (IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		/* Only check if the ID is in use, as the advertiser can be
		 * started and stopped without reconfiguring parameters.
		 */
		if (check_data->id == adv->id) {
			check_data->adv_enabled = true;
		}
	} else {
		if (check_data->id == adv->id &&
		    atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
			check_data->adv_enabled = true;
		}
	}
}

static void adv_is_private_enabled(struct bt_le_ext_adv *adv, void *data)
{
	bool *adv_enabled = data;

	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED) &&
	    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
		*adv_enabled = true;
	}
}

#if defined(CONFIG_BT_SMP)
static void adv_is_limited_enabled(struct bt_le_ext_adv *adv, void *data)
{
	bool *adv_enabled = data;

	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED) &&
	    atomic_test_bit(adv->flags, BT_ADV_LIMITED)) {
		*adv_enabled = true;
	}
}

static void adv_pause_enabled(struct bt_le_ext_adv *adv, void *data)
{
	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		atomic_set_bit(adv->flags, BT_ADV_PAUSED);
		bt_le_adv_set_enable(adv, false);
	}
}

static void adv_unpause_enabled(struct bt_le_ext_adv *adv, void *data)
{
	if (atomic_test_and_clear_bit(adv->flags, BT_ADV_PAUSED)) {
		bt_le_adv_set_enable(adv, true);
	}
}
#endif /* defined(CONFIG_BT_SMP) */

static int set_random_address(const bt_addr_t *addr)
{
	struct net_buf *buf;
	int err;

	LOG_DBG("%s", bt_addr_str(addr));

	/* Do nothing if we already have the right address */
	if (bt_addr_eq(addr, &bt_dev.random_addr.a)) {
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, sizeof(*addr));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, addr, sizeof(*addr));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_RANDOM_ADDRESS, buf, NULL);
	if (err) {
		if (err == -EACCES) {
			/* If we are here we probably tried to set a random
			 * address while a legacy advertising, scanning or
			 * initiating is enabled, this is illegal.
			 *
			 * See Core Spec @ Vol 4, Part E 7.8.4
			 */
			LOG_WRN("cmd disallowed");
		}
		return err;
	}

	bt_addr_copy(&bt_dev.random_addr.a, addr);
	bt_dev.random_addr.type = BT_ADDR_LE_RANDOM;
	return 0;
}

int bt_id_set_adv_random_addr(struct bt_le_ext_adv *adv,
			      const bt_addr_t *addr)
{
	struct bt_hci_cp_le_set_adv_set_random_addr *cp;
	struct net_buf *buf;
	int err;

	CHECKIF(adv == NULL || addr == NULL) {
		return -EINVAL;
	}

	if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		return set_random_address(addr);
	}

	LOG_DBG("%s", bt_addr_str(addr));

	if (!atomic_test_bit(adv->flags, BT_ADV_PARAMS_SET)) {
		bt_addr_copy(&adv->random_addr.a, addr);
		adv->random_addr.type = BT_ADDR_LE_RANDOM;
		atomic_set_bit(adv->flags, BT_ADV_RANDOM_ADDR_PENDING);
		return 0;
	}

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR,
				sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	cp->handle = adv->handle;
	bt_addr_copy(&cp->bdaddr, addr);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADV_SET_RANDOM_ADDR, buf,
				   NULL);
	if (err) {
		return err;
	}

	if (&adv->random_addr.a != addr) {
		bt_addr_copy(&adv->random_addr.a, addr);
	}
	adv->random_addr.type = BT_ADDR_LE_RANDOM;
	return 0;
}

/*	If rpa sharing is enabled, then rpa expired cb of adv-sets belonging
 *	to same id is verified to return true. If not, adv-sets will continue
 *	with old rpa through out the rpa rotations.
 */
static void adv_rpa_expired(struct bt_le_ext_adv *adv, void *data)
{
	bool rpa_invalid = true;
#if defined(CONFIG_BT_EXT_ADV) && defined(CONFIG_BT_PRIVACY)
	/* Notify the user about the RPA timeout and set the RPA validity. */
	if (atomic_test_bit(adv->flags, BT_ADV_RPA_VALID) &&
	    adv->cb && adv->cb->rpa_expired) {
		rpa_invalid = adv->cb->rpa_expired(adv);
	}
#endif

	if (IS_ENABLED(CONFIG_BT_RPA_SHARING)) {

		if (adv->id >= bt_dev.id_count) {
			return;
		}
		bool *rpa_invalid_set_ptr = data;

		if (!rpa_invalid) {
			rpa_invalid_set_ptr[adv->id] = false;
		}
	} else {
		if (rpa_invalid) {
			atomic_clear_bit(adv->flags, BT_ADV_RPA_VALID);
		}
	}
}

static void adv_rpa_invalidate(struct bt_le_ext_adv *adv, void *data)
{
	/* RPA of Advertisers limited by timeout or number of packets only expire
	 * when they are stopped.
	 */
	if (!atomic_test_bit(adv->flags, BT_ADV_LIMITED) &&
	    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
		adv_rpa_expired(adv, data);
	}
}

#if defined(CONFIG_BT_RPA_SHARING)
static void adv_rpa_clear_data(struct bt_le_ext_adv *adv, void *data)
{
	if (adv->id >= bt_dev.id_count) {
		return;
	}
	bool *rpa_invalid_set_ptr = data;

	if (rpa_invalid_set_ptr[adv->id]) {
		atomic_clear_bit(adv->flags, BT_ADV_RPA_VALID);
		bt_addr_copy(&bt_dev.rpa[adv->id], BT_ADDR_NONE);
	} else {
		LOG_WRN("Adv sets rpa expired cb with id %d returns false", adv->id);
	}
}
#endif

static void le_rpa_invalidate(void)
{
	/* Invalidate RPA */
	if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED))) {
		atomic_clear_bit(bt_dev.flags, BT_DEV_RPA_VALID);
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {

		if (bt_dev.id_count == 0) {
			return;
		}
		bool rpa_expired_data[bt_dev.id_count];
		for (uint8_t i = 0; i < bt_dev.id_count; i++) {
			rpa_expired_data[i] = true;
		}

		bt_le_ext_adv_foreach(adv_rpa_invalidate, &rpa_expired_data);
#if defined(CONFIG_BT_RPA_SHARING)
		/* rpa_expired data collected. now clear data based on data collected. */
		bt_le_ext_adv_foreach(adv_rpa_clear_data, &rpa_expired_data);
#endif
	}
}

#if defined(CONFIG_BT_PRIVACY)

#if defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC)
static void le_rpa_timeout_update(void)
{
	int err = 0;

	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_RPA_TIMEOUT_CHANGED)) {
		struct net_buf *buf;
		struct bt_hci_cp_le_set_rpa_timeout *cp;

		buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_RPA_TIMEOUT,
					sizeof(*cp));
		if (!buf) {
			LOG_ERR("Failed to create HCI RPA timeout command");
			err = -ENOBUFS;
			goto submit;
		}

		cp = net_buf_add(buf, sizeof(*cp));
		cp->rpa_timeout = sys_cpu_to_le16(bt_dev.rpa_timeout);
		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_RPA_TIMEOUT, buf, NULL);
		if (err) {
			LOG_ERR("Failed to send HCI RPA timeout command");
			goto submit;
		}
	}

submit:
	if (err) {
		atomic_set_bit(bt_dev.flags, BT_DEV_RPA_TIMEOUT_CHANGED);
	}
}
#endif

static void le_rpa_timeout_submit(void)
{
#if defined(CONFIG_BT_RPA_TIMEOUT_DYNAMIC)
	le_rpa_timeout_update();
#endif

	(void)k_work_schedule(&bt_dev.rpa_update, K_SECONDS(bt_dev.rpa_timeout));
}

/* this function sets new RPA only if current one is no longer valid */
int bt_id_set_private_addr(uint8_t id)
{
	bt_addr_t rpa;
	int err;

	CHECKIF(id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	/* check if RPA is valid */
	if (atomic_test_bit(bt_dev.flags, BT_DEV_RPA_VALID)) {
		return 0;
	}

	err = bt_rpa_create(bt_dev.irk[id], &rpa);
	if (!err) {
		err = set_random_address(&rpa);
		if (!err) {
			atomic_set_bit(bt_dev.flags, BT_DEV_RPA_VALID);
		}
	}

	le_rpa_timeout_submit();

	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		LOG_INF("RPA: %s", bt_addr_str(&rpa));
	}

	return 0;
}

#if defined(CONFIG_BT_RPA_SHARING)
static int adv_rpa_get(struct bt_le_ext_adv *adv, bt_addr_t *rpa)
{
	int err;

	if (bt_addr_eq(&bt_dev.rpa[adv->id], BT_ADDR_NONE)) {
		err = bt_rpa_create(bt_dev.irk[adv->id], &bt_dev.rpa[adv->id]);
		if (err) {
			return err;
		}
	}

	bt_addr_copy(rpa, &bt_dev.rpa[adv->id]);

	return 0;
}
#else
static int adv_rpa_get(struct bt_le_ext_adv *adv, bt_addr_t *rpa)
{
	int err;

	err = bt_rpa_create(bt_dev.irk[adv->id], rpa);
	if (err) {
		return err;
	}

	return 0;
}
#endif /* defined(CONFIG_BT_RPA_SHARING) */

int bt_id_set_adv_private_addr(struct bt_le_ext_adv *adv)
{
	bt_addr_t rpa;
	int err;

	CHECKIF(adv == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    (adv->options & BT_LE_ADV_OPT_USE_NRPA)) {
		/* The host doesn't support setting NRPAs when BT_PRIVACY=y.
		 * In that case you probably want to use an RPA anyway.
		 */
		LOG_ERR("NRPA not supported when BT_PRIVACY=y");

		return -ENOSYS;
	}

	if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		return bt_id_set_private_addr(adv->id);
	}

	/* check if RPA is valid */
	if (atomic_test_bit(adv->flags, BT_ADV_RPA_VALID)) {
		/* Schedule the RPA timer if it is not running.
		 * The RPA may be valid without the timer running.
		 */
		if (!atomic_test_bit(adv->flags, BT_ADV_LIMITED)) {
			le_rpa_timeout_submit();
		}

		return 0;
	}

	if (adv == bt_le_adv_lookup_legacy() && adv->id == BT_ID_DEFAULT) {
		/* Make sure that a Legacy advertiser using default ID has same
		 * RPA address as scanner roles.
		 */
		err = bt_id_set_private_addr(BT_ID_DEFAULT);
		if (err) {
			return err;
		}

		err = bt_id_set_adv_random_addr(adv, &bt_dev.random_addr.a);
		if (!err) {
			atomic_set_bit(adv->flags, BT_ADV_RPA_VALID);
		}

		return 0;
	}

	err = adv_rpa_get(adv, &rpa);
	if (!err) {
		err = bt_id_set_adv_random_addr(adv, &rpa);
		if (!err) {
			atomic_set_bit(adv->flags, BT_ADV_RPA_VALID);
		}
	}

	if (!atomic_test_bit(adv->flags, BT_ADV_LIMITED)) {
		le_rpa_timeout_submit();
	}

	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		LOG_INF("RPA: %s", bt_addr_str(&rpa));
	}

	return 0;
}
#else
int bt_id_set_private_addr(uint8_t id)
{
	bt_addr_t nrpa;
	int err;

	CHECKIF(id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	err = bt_rand(nrpa.val, sizeof(nrpa.val));
	if (err) {
		return err;
	}

	BT_ADDR_SET_NRPA(&nrpa);

	err = set_random_address(&nrpa);
	if (err)  {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		LOG_INF("NRPA: %s", bt_addr_str(&nrpa));
	}

	return 0;
}

int bt_id_set_adv_private_addr(struct bt_le_ext_adv *adv)
{
	bt_addr_t nrpa;
	int err;

	CHECKIF(adv == NULL) {
		return -EINVAL;
	}

	err = bt_rand(nrpa.val, sizeof(nrpa.val));
	if (err) {
		return err;
	}

	BT_ADDR_SET_NRPA(&nrpa);

	err = bt_id_set_adv_random_addr(adv, &nrpa);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_LOG_SNIFFER_INFO)) {
		LOG_INF("NRPA: %s", bt_addr_str(&nrpa));
	}

	return 0;
}
#endif /* defined(CONFIG_BT_PRIVACY) */

static void adv_pause_rpa(struct bt_le_ext_adv *adv, void *data)
{
	bool *adv_enabled = data;

	/* Disable advertising sets to prepare them for RPA update. */
	if (atomic_test_bit(adv->flags, BT_ADV_ENABLED) &&
	    !atomic_test_bit(adv->flags, BT_ADV_LIMITED) &&
	    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
		int err;

		err = bt_le_adv_set_enable_ext(adv, false, NULL);
		if (err) {
			LOG_ERR("Failed to disable advertising (err %d)", err);
		}

		atomic_set_bit(adv->flags, BT_ADV_RPA_UPDATE);
		*adv_enabled = true;
	}
}

static bool le_adv_rpa_timeout(void)
{
	bool adv_enabled = false;

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		if (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
		    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
			/* Pause all advertising sets using RPAs */
			bt_le_ext_adv_foreach(adv_pause_rpa, &adv_enabled);
		} else {
			/* Check if advertising set is enabled */
			bt_le_ext_adv_foreach(adv_is_private_enabled, &adv_enabled);
		}
	}

	return adv_enabled;
}

static void adv_enable_rpa(struct bt_le_ext_adv *adv, void *data)
{
	if (atomic_test_and_clear_bit(adv->flags, BT_ADV_RPA_UPDATE)) {
		int err;

		err = bt_id_set_adv_private_addr(adv);
		if (err) {
			LOG_WRN("Failed to update advertiser RPA address (%d)", err);
		}

		err = bt_le_adv_set_enable_ext(adv, true, NULL);
		if (err) {
			LOG_ERR("Failed to enable advertising (err %d)", err);
		}
	}
}

static void le_update_private_addr(void)
{
	struct bt_le_ext_adv *adv = NULL;
	bool adv_enabled = false;
	uint8_t id = BT_ID_DEFAULT;
	int err;

#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled = false;

	if (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
	    !(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED))) {
		bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
		scan_enabled = true;
	}
#endif
	if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING)) {
		/* Canceled initiating procedure will be restarted by
		 * connection complete event.
		 */
		bt_le_create_conn_cancel();
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
	    !(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	      BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		adv = bt_le_adv_lookup_legacy();

		if (adv &&
		    atomic_test_bit(adv->flags, BT_ADV_ENABLED) &&
		    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
			adv_enabled = true;
			id = adv->id;
			bt_le_adv_set_enable_legacy(adv, false);
		}
	}

	/* If both advertiser and scanner is running then the advertiser
	 * ID must be BT_ID_DEFAULT, this will update the RPA address
	 * for both roles.
	 */
	err = bt_id_set_private_addr(id);
	if (err) {
		LOG_WRN("Failed to update RPA address (%d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
	    IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	    BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features)) {
		bt_le_ext_adv_foreach(adv_enable_rpa, NULL);
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
	    adv && adv_enabled) {
		bt_le_adv_set_enable_legacy(adv, true);
	}

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		bt_le_scan_set_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif
}

static void le_force_rpa_timeout(void)
{
#if defined(CONFIG_BT_PRIVACY)
	struct k_work_sync sync;

	k_work_cancel_delayable_sync(&bt_dev.rpa_update, &sync);
#endif
	(void)le_adv_rpa_timeout();
	le_rpa_invalidate();
	le_update_private_addr();
}

#if defined(CONFIG_BT_PRIVACY)
static void rpa_timeout(struct k_work *work)
{
	bool adv_enabled;

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		struct bt_conn *conn =
			bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL,
						BT_CONN_SCAN_BEFORE_INITIATING);

		if (conn) {
			bt_conn_unref(conn);
			bt_le_create_conn_cancel();
		}
	}

	adv_enabled = le_adv_rpa_timeout();
	le_rpa_invalidate();

	/* IF no roles using the RPA is running we can stop the RPA timer */
	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		if (!(adv_enabled || atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING) ||
		      bt_le_scan_active_scanner_running())) {
			return;
		}
	}

	le_update_private_addr();
}
#endif /* CONFIG_BT_PRIVACY */

bool bt_id_scan_random_addr_check(void)
{
	struct bt_le_ext_adv *adv;

	if (!IS_ENABLED(CONFIG_BT_BROADCASTER) ||
	    (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	     BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		/* Advertiser is not enabled or advertiser and scanner are using
		 * a different random address.
		 */
		return true;
	}

	adv = bt_le_adv_lookup_legacy();
	if (!adv) {
		return true;
	}

	/* If the advertiser is not active there is no issue */
	if (!atomic_test_bit(adv->flags, BT_ADV_ENABLED)) {
		return true;
	}

	/* When privacy is enabled the random address will not be set
	 * immediately before starting the role, because the RPA might still be
	 * valid and only updated on RPA timeout.
	 */
	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		/* Cannot start scanner or initiator if the random address is
		 * used by the advertiser for an RPA with a different identity
		 * or for a random static identity address.
		 */
		if ((atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY) &&
		     bt_dev.id_addr[adv->id].type == BT_ADDR_LE_RANDOM) ||
		    adv->id != BT_ID_DEFAULT) {
			return false;
		}
	}

	/* If privacy is not enabled then the random address will be attempted
	 * to be set before enabling the role. If another role is already using
	 * the random address then this command will fail, and should return
	 * the error code to the application.
	 */
	return true;
}

bool bt_id_adv_random_addr_check(const struct bt_le_adv_param *param)
{
	CHECKIF(param == NULL) {
		return false;
	}

	if (!IS_ENABLED(CONFIG_BT_OBSERVER) ||
	    (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	     BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		/* If scanner roles are not enabled or advertiser and scanner
		 * are using a different random address.
		 */
		return true;
	}

	/* If scanner roles are not active there is no issue. */
	if (!(atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING) ||
	      atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING))) {
		return true;
	}

	/* When privacy is enabled the random address will not be set
	 * immediately before starting the role, because the RPA might still be
	 * valid and only updated on RPA timeout.
	 */
	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		/* Cannot start an advertiser with random static identity or
		 * using an RPA generated for a different identity than scanner
		 * roles.
		 */
		if (((param->options & BT_LE_ADV_OPT_USE_IDENTITY) &&
		     bt_dev.id_addr[param->id].type == BT_ADDR_LE_RANDOM) ||
		    param->id != BT_ID_DEFAULT) {
			return false;
		}
	} else if (IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY) &&
		   atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) &&
		   bt_dev.id_addr[BT_ID_DEFAULT].type == BT_ADDR_LE_RANDOM) {
		/* Scanning with random static identity. Stop the advertiser
		 * from overwriting the passive scanner identity address.
		 * In this case the LE Set Random Address command does not
		 * protect us in the case of a passive scanner.
		 * Explicitly stop it here.
		 */

		if (!(param->options & _BT_LE_ADV_OPT_CONNECTABLE) &&
		     (param->options & BT_LE_ADV_OPT_USE_IDENTITY)) {
			/* Attempt to set non-connectable NRPA */
			return false;
		} else if (bt_dev.id_addr[param->id].type ==
			   BT_ADDR_LE_RANDOM &&
			   param->id != BT_ID_DEFAULT) {
			/* Attempt to set connectable, or non-connectable with
			 * identity different than scanner.
			 */
			return false;
		}
	}

	/* If privacy is not enabled then the random address will be attempted
	 * to be set before enabling the role. If another role is already using
	 * the random address then this command will fail, and should return
	 * the error code to the application.
	 */
	return true;
}

void bt_id_adv_limited_stopped(struct bt_le_ext_adv *adv)
{
	adv_rpa_expired(adv, NULL);
}

#if defined(CONFIG_BT_SMP)
static int le_set_privacy_mode(const bt_addr_le_t *addr, uint8_t mode)
{
	struct bt_hci_cp_le_set_privacy_mode cp;
	struct net_buf *buf;
	int err;

	/* Check if set privacy mode command is supported */
	if (!BT_CMD_TEST(bt_dev.supported_commands, 39, 2)) {
		LOG_WRN("Set privacy mode command is not supported");
		return 0;
	}

	LOG_DBG("addr %s mode 0x%02x", bt_addr_le_str(addr), mode);

	bt_addr_le_copy(&cp.id_addr, addr);
	cp.mode = mode;

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_PRIVACY_MODE, sizeof(cp));
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_mem(buf, &cp, sizeof(cp));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_PRIVACY_MODE, buf, NULL);
	if (err) {
		return err;
	}

	return 0;
}

static int addr_res_enable(uint8_t enable)
{
	struct net_buf *buf;

	LOG_DBG("%s", enable ? "enabled" : "disabled");

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE, 1);
	if (!buf) {
		return -ENOBUFS;
	}

	net_buf_add_u8(buf, enable);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE,
				    buf, NULL);
}

static int hci_id_add(uint8_t id, const bt_addr_le_t *addr, uint8_t peer_irk[16])
{
	struct bt_hci_cp_le_add_dev_to_rl *cp;
	struct net_buf *buf;

	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	LOG_DBG("addr %s", bt_addr_le_str(addr));

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_ADD_DEV_TO_RL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_id_addr, addr);
	memcpy(cp->peer_irk, peer_irk, 16);

#if defined(CONFIG_BT_PRIVACY)
	(void)memcpy(cp->local_irk, &bt_dev.irk[id], 16);
#else
	(void)memset(cp->local_irk, 0, 16);
#endif

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_RL, buf, NULL);
}

static void pending_id_update(struct bt_keys *keys, void *data)
{
	if (keys->state & BT_KEYS_ID_PENDING_ADD) {
		keys->state &= ~BT_KEYS_ID_PENDING_ADD;
		bt_id_add(keys);
		return;
	}

	if (keys->state & BT_KEYS_ID_PENDING_DEL) {
		keys->state &= ~BT_KEYS_ID_PENDING_DEL;
		bt_id_del(keys);
		return;
	}
}

void bt_id_pending_keys_update_set(struct bt_keys *keys, uint8_t flag)
{
	atomic_set_bit(bt_dev.flags, BT_DEV_ID_PENDING);
	keys->state |= flag;
}

void bt_id_pending_keys_update(void)
{
	if (atomic_test_and_clear_bit(bt_dev.flags, BT_DEV_ID_PENDING)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    IS_ENABLED(CONFIG_BT_PRIVACY)) {
			bt_keys_foreach_type(BT_KEYS_ALL, pending_id_update, NULL);
		} else {
			bt_keys_foreach_type(BT_KEYS_IRK, pending_id_update, NULL);
		}
	}
}

struct bt_id_conflict {
	struct bt_keys *candidate;
	struct bt_keys *found;
};

/* The Controller Resolve List is constrained by 7.8.38 "LE Add Device To
 * Resolving List command". The Host is designed with the assumption that all
 * local bonds can be put in the resolve list if there is room. Therefore we
 * must refuse bonds that conflict in the resolve list. Notably, this prevents
 * multiple local identities to bond with the same remote identity.
 */
void find_rl_conflict(struct bt_keys *resident, void *user_data)
{
	struct bt_id_conflict *conflict = user_data;
	bool addr_conflict;
	bool irk_conflict;

	__ASSERT_NO_MSG(conflict != NULL);
	__ASSERT_NO_MSG(conflict->candidate != NULL);
	__ASSERT_NO_MSG(resident != NULL);
	/* Only uncommitted bonds can be in conflict with committed bonds. */
	__ASSERT_NO_MSG((conflict->candidate->state & BT_KEYS_ID_ADDED) == 0);

	if (conflict->found) {
		return;
	}

	/* Test against committed bonds only. */
	if ((resident->state & BT_KEYS_ID_ADDED) == 0) {
		return;
	}

	addr_conflict = bt_addr_le_eq(&conflict->candidate->addr, &resident->addr);

	/* All-zero IRK is "no IRK", and does not conflict with other Zero-IRKs. */
	irk_conflict = (!bt_irk_eq(&conflict->candidate->irk, &(struct bt_irk){}) &&
			bt_irk_eq(&conflict->candidate->irk, &resident->irk));

	if (addr_conflict || irk_conflict) {
		LOG_DBG("Resident : addr %s and IRK %s", bt_addr_le_str(&resident->addr),
			bt_hex(resident->irk.val, sizeof(resident->irk.val)));
		LOG_DBG("Candidate: addr %s and IRK %s", bt_addr_le_str(&conflict->candidate->addr),
			bt_hex(conflict->candidate->irk.val, sizeof(conflict->candidate->irk.val)));

		conflict->found = resident;
	}
}

struct bt_keys *bt_id_find_conflict(struct bt_keys *candidate)
{
	struct bt_id_conflict conflict = {
		.candidate = candidate,
	};

	bt_keys_foreach_type(BT_KEYS_IRK, find_rl_conflict, &conflict);

	return conflict.found;
}

void bt_id_add(struct bt_keys *keys)
{
	CHECKIF(keys == NULL) {
		return;
	}

	struct bt_conn *conn;
	int err;

	LOG_DBG("addr %s", bt_addr_le_str(&keys->addr));

	__ASSERT_NO_MSG(keys != NULL);
	/* We assume (and could assert) !bt_id_find_conflict(keys) here. */

	/* Nothing to be done if host-side resolving is used */
	if (!bt_dev.le.rl_size || bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		bt_dev.le.rl_entries++;
		keys->state |= BT_KEYS_ID_ADDED;
		return;
	}

	conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_INITIATING);
	if (conn) {
		bt_id_pending_keys_update_set(keys, BT_KEYS_ID_PENDING_ADD);
		bt_conn_unref(conn);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
	    IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		bool adv_enabled = false;

		bt_le_ext_adv_foreach(adv_is_limited_enabled, &adv_enabled);
		if (adv_enabled) {
			bt_id_pending_keys_update_set(keys,
						   BT_KEYS_ID_PENDING_ADD);
			return;
		}
	}

#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) && scan_enabled &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED)) {
		bt_id_pending_keys_update_set(keys, BT_KEYS_ID_PENDING_ADD);
	}
#endif

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		bt_le_ext_adv_foreach(adv_pause_enabled, NULL);
	}

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	/* If there are any existing entries address resolution will be on */
	if (bt_dev.le.rl_entries) {
		err = addr_res_enable(BT_HCI_ADDR_RES_DISABLE);
		if (err) {
			LOG_WRN("Failed to disable address resolution");
			goto done;
		}
	}

	if (bt_dev.le.rl_entries == bt_dev.le.rl_size) {
		LOG_WRN("Resolving list size exceeded. Switching to host.");

		err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_CLEAR_RL, NULL, NULL);
		if (err) {
			LOG_ERR("Failed to clear resolution list");
			goto done;
		}

		bt_dev.le.rl_entries++;
		keys->state |= BT_KEYS_ID_ADDED;

		goto done;
	}

	err = hci_id_add(keys->id, &keys->addr, keys->irk.val);
	if (err) {
		LOG_ERR("Failed to add IRK to controller");
		goto done;
	}

	bt_dev.le.rl_entries++;
	keys->state |= BT_KEYS_ID_ADDED;

	/*
	 * According to Core Spec. 5.0 Vol 1, Part A 5.4.5 Privacy Feature
	 *
	 * By default, network privacy mode is used when private addresses are
	 * resolved and generated by the Controller, so advertising packets from
	 * peer devices that contain private addresses will only be accepted.
	 * By changing to the device privacy mode device is only concerned about
	 * its privacy and will accept advertising packets from peer devices
	 * that contain their identity address as well as ones that contain
	 * a private address, even if the peer device has distributed its IRK in
	 * the past.
	 */
	err = le_set_privacy_mode(&keys->addr, BT_HCI_LE_PRIVACY_MODE_DEVICE);
	if (err) {
		LOG_ERR("Failed to set privacy mode");
		goto done;
	}

done:
	addr_res_enable(BT_HCI_ADDR_RES_ENABLE);

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		bt_le_scan_set_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		bt_le_ext_adv_foreach(adv_unpause_enabled, NULL);
	}
}

static void keys_add_id(struct bt_keys *keys, void *data)
{
	if (keys->state & BT_KEYS_ID_ADDED) {
		hci_id_add(keys->id, &keys->addr, keys->irk.val);
	}
}

static int hci_id_del(const bt_addr_le_t *addr)
{
	struct bt_hci_cp_le_rem_dev_from_rl *cp;
	struct net_buf *buf;

	LOG_DBG("addr %s", bt_addr_le_str(addr));

	buf = bt_hci_cmd_create(BT_HCI_OP_LE_REM_DEV_FROM_RL, sizeof(*cp));
	if (!buf) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_id_addr, addr);

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_REM_DEV_FROM_RL, buf, NULL);
}

void bt_id_del(struct bt_keys *keys)
{
	struct bt_conn *conn;
	int err;

	CHECKIF(keys == NULL) {
		return;
	}

	LOG_DBG("addr %s", bt_addr_le_str(&keys->addr));

	if (!bt_dev.le.rl_size ||
	    bt_dev.le.rl_entries > bt_dev.le.rl_size + 1) {
		__ASSERT_NO_MSG(bt_dev.le.rl_entries > 0);
		if (bt_dev.le.rl_entries > 0) {
			bt_dev.le.rl_entries--;
		}
		keys->state &= ~BT_KEYS_ID_ADDED;
		return;
	}

	conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL, BT_CONN_INITIATING);
	if (conn) {
		bt_id_pending_keys_update_set(keys, BT_KEYS_ID_PENDING_DEL);
		bt_conn_unref(conn);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER) &&
	    IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		bool adv_enabled = false;

		bt_le_ext_adv_foreach(adv_is_limited_enabled, &adv_enabled);
		if (adv_enabled) {
			bt_id_pending_keys_update_set(keys, BT_KEYS_ID_PENDING_DEL);
			return;
		}
	}

#if defined(CONFIG_BT_OBSERVER)
	bool scan_enabled = atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);

	if (IS_ENABLED(CONFIG_BT_EXT_ADV) && scan_enabled &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_SCAN_LIMITED)) {
		bt_id_pending_keys_update_set(keys, BT_KEYS_ID_PENDING_DEL);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		bt_le_ext_adv_foreach(adv_pause_enabled, NULL);
	}

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	err = addr_res_enable(BT_HCI_ADDR_RES_DISABLE);
	if (err) {
		LOG_ERR("Disabling address resolution failed (err %d)", err);
		goto done;
	}

	/* We checked size + 1 earlier, so here we know we can fit again */
	if (bt_dev.le.rl_entries > bt_dev.le.rl_size) {
		bt_dev.le.rl_entries--;
		keys->state &= ~BT_KEYS_ID_ADDED;
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    IS_ENABLED(CONFIG_BT_PRIVACY)) {
			bt_keys_foreach_type(BT_KEYS_ALL, keys_add_id, NULL);
		} else {
			bt_keys_foreach_type(BT_KEYS_IRK, keys_add_id, NULL);
		}
		goto done;
	}

	err = hci_id_del(&keys->addr);
	if (err) {
		LOG_ERR("Failed to remove IRK from controller");
		goto done;
	}

	bt_dev.le.rl_entries--;
	keys->state &= ~BT_KEYS_ID_ADDED;

done:
	/* Only re-enable if there are entries to do resolving with */
	if (bt_dev.le.rl_entries) {
		addr_res_enable(BT_HCI_ADDR_RES_ENABLE);
	}

#if defined(CONFIG_BT_OBSERVER)
	if (scan_enabled) {
		bt_le_scan_set_enable(BT_HCI_LE_SCAN_ENABLE);
	}
#endif /* CONFIG_BT_OBSERVER */

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		bt_le_ext_adv_foreach(adv_unpause_enabled, NULL);
	}
}
#endif /* defined(CONFIG_BT_SMP) */

void bt_id_get(bt_addr_le_t *addrs, size_t *count)
{
	if (addrs) {
		size_t to_copy = MIN(*count, bt_dev.id_count);

		memcpy(addrs, bt_dev.id_addr, to_copy * sizeof(bt_addr_le_t));
		*count = to_copy;
	} else {
		*count = bt_dev.id_count;
	}
}

static int id_find(const bt_addr_le_t *addr)
{
	uint8_t id;

	for (id = 0U; id < bt_dev.id_count; id++) {
		if (bt_addr_le_eq(addr, &bt_dev.id_addr[id])) {
			return id;
		}
	}

	return -ENOENT;
}

static int id_create(uint8_t id, bt_addr_le_t *addr, uint8_t *irk)
{
	if (addr && !bt_addr_le_eq(addr, BT_ADDR_LE_ANY)) {
		bt_addr_le_copy(&bt_dev.id_addr[id], addr);
	} else {
		bt_addr_le_t new_addr;

		do {
			int err;

			err = bt_addr_le_create_static(&new_addr);
			if (err) {
				return err;
			}
			/* Make sure we didn't generate a duplicate */
		} while (id_find(&new_addr) >= 0);

		bt_addr_le_copy(&bt_dev.id_addr[id], &new_addr);

		if (addr) {
			bt_addr_le_copy(addr, &bt_dev.id_addr[id]);
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	{
		uint8_t zero_irk[16] = { 0 };

		if (irk && memcmp(irk, zero_irk, 16)) {
			memcpy(&bt_dev.irk[id], irk, 16);
		} else {
			int err;

			err = bt_rand(&bt_dev.irk[id], 16);
			if (err) {
				return err;
			}

			if (irk) {
				memcpy(irk, &bt_dev.irk[id], 16);
			}
		}

#if defined(CONFIG_BT_RPA_SHARING)
		bt_addr_copy(&bt_dev.rpa[id], BT_ADDR_NONE);
#endif
	}
#endif
	/* Only store if stack was already initialized. Before initialization
	 * we don't know the flash content, so it's potentially harmful to
	 * try to write anything there.
	 */
	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		(void)bt_settings_store_id();
		(void)bt_settings_store_irk();
	}

	return 0;
}

int bt_id_create(bt_addr_le_t *addr, uint8_t *irk)
{
	int new_id, err;

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && irk) {
		return -EINVAL;
	}

	if (addr && !bt_addr_le_eq(addr, BT_ADDR_LE_ANY)) {
		if (id_find(addr) >= 0) {
			return -EALREADY;
		}

		if (addr->type == BT_ADDR_LE_PUBLIC && IS_ENABLED(CONFIG_BT_HCI_SET_PUBLIC_ADDR)) {
			/* set the single public address */
			if (bt_dev.id_count != 0) {
				return -EALREADY;
			}
			bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], addr);
			bt_dev.id_count++;
			return BT_ID_DEFAULT;
		} else if (addr->type != BT_ADDR_LE_RANDOM || !BT_ADDR_IS_STATIC(&addr->a)) {
			LOG_ERR("Only random static identity address supported");
			return -EINVAL;
		}
	}

	if (bt_dev.id_count == ARRAY_SIZE(bt_dev.id_addr)) {
		return -ENOMEM;
	}

	/* bt_rand is not available before Bluetooth enable has been called */
	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		uint8_t zero_irk[16] = { 0 };

		if (!(addr && !bt_addr_le_eq(addr, BT_ADDR_LE_ANY))) {
			return -EINVAL;
		}

		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    !(irk && memcmp(irk, zero_irk, 16))) {
			return -EINVAL;
		}
	}

	new_id = bt_dev.id_count++;
	err = id_create(new_id, addr, irk);
	if (err) {
		bt_dev.id_count--;
		return err;
	}

	return new_id;
}

int bt_id_reset(uint8_t id, bt_addr_le_t *addr, uint8_t *irk)
{
	int err;

	if (addr && !bt_addr_le_eq(addr, BT_ADDR_LE_ANY)) {
		if (addr->type != BT_ADDR_LE_RANDOM ||
		    !BT_ADDR_IS_STATIC(&addr->a)) {
			LOG_ERR("Only static random identity address supported");
			return -EINVAL;
		}

		if (id_find(addr) >= 0) {
			return -EALREADY;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_PRIVACY) && irk) {
		return -EINVAL;
	}

	if (id == BT_ID_DEFAULT || id >= bt_dev.id_count) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		struct bt_adv_id_check_data check_data = {
			.id = id,
			.adv_enabled = false,
		};

		bt_le_ext_adv_foreach(adv_id_check_func, &check_data);
		if (check_data.adv_enabled) {
			return -EBUSY;
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP) &&
	    !bt_addr_le_eq(&bt_dev.id_addr[id], BT_ADDR_LE_ANY)) {
		err = bt_unpair(id, NULL);
		if (err) {
			return err;
		}
	}

	err = id_create(id, addr, irk);
	if (err) {
		return err;
	}

	return id;
}

int bt_id_delete(uint8_t id)
{
	if (id == BT_ID_DEFAULT || id >= bt_dev.id_count) {
		return -EINVAL;
	}

	if (bt_addr_le_eq(&bt_dev.id_addr[id], BT_ADDR_LE_ANY)) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		struct bt_adv_id_check_data check_data = {
			.id = id,
			.adv_enabled = false,
		};

		bt_le_ext_adv_foreach(adv_id_check_func, &check_data);
		if (check_data.adv_enabled) {
			return -EBUSY;
		}
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		int err;

		err = bt_unpair(id, NULL);
		if (err) {
			return err;
		}
	}

#if defined(CONFIG_BT_PRIVACY)
	(void)memset(bt_dev.irk[id], 0, 16);
#endif
	bt_addr_le_copy(&bt_dev.id_addr[id], BT_ADDR_LE_ANY);

	if (id == bt_dev.id_count - 1) {
		bt_dev.id_count--;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS) &&
	    atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		(void)bt_settings_store_id();
		(void)bt_settings_store_irk();
	}

	return 0;
}

#if defined(CONFIG_BT_PRIVACY)
static void bt_read_identity_root(uint8_t *ir)
{
	/* Invalid IR */
	memset(ir, 0, 16);

#if defined(CONFIG_BT_HCI_VS)
	struct bt_hci_rp_vs_read_key_hierarchy_roots *rp;
	struct net_buf *rsp;
	int err;

	if (!BT_VS_CMD_READ_KEY_ROOTS(bt_dev.vs_commands)) {
		return;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_KEY_HIERARCHY_ROOTS, NULL,
				   &rsp);
	if (err) {
		LOG_WRN("Failed to read identity root");
		return;
	}

	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
	    rsp->len != sizeof(struct bt_hci_rp_vs_read_key_hierarchy_roots)) {
		LOG_WRN("Invalid Vendor HCI extensions");
		net_buf_unref(rsp);
		return;
	}

	rp = (void *)rsp->data;
	memcpy(ir, rp->ir, 16);

	net_buf_unref(rsp);
#endif /* defined(CONFIG_BT_HCI_VS) */
}
#endif /* defined(CONFIG_BT_PRIVACY) */

uint8_t bt_id_read_public_addr(bt_addr_le_t *addr)
{
	struct bt_hci_rp_read_bd_addr *rp;
	struct net_buf *rsp;
	int err;

	CHECKIF(addr == NULL) {
		LOG_WRN("Invalid input parameters");
		return 0U;
	}

	/* Read Bluetooth Address */
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL, &rsp);
	if (err) {
		LOG_WRN("Failed to read public address");
		return 0U;
	}

	rp = (void *)rsp->data;

	if (bt_addr_eq(&rp->bdaddr, BT_ADDR_ANY) ||
	    bt_addr_eq(&rp->bdaddr, BT_ADDR_NONE)) {
		LOG_DBG("Controller has no public address");
		net_buf_unref(rsp);
		return 0U;
	}

	bt_addr_copy(&addr->a, &rp->bdaddr);
	addr->type = BT_ADDR_LE_PUBLIC;

	net_buf_unref(rsp);
	return 1U;
}

int bt_setup_public_id_addr(void)
{
	bt_addr_le_t addr;
	uint8_t *irk = NULL;

	bt_dev.id_count = bt_id_read_public_addr(&addr);

	if (!bt_dev.id_count) {
		return 0;
	}

#if defined(CONFIG_BT_PRIVACY)
	uint8_t ir_irk[16];
	uint8_t ir[16];

	bt_read_identity_root(ir);

	if (!IS_ENABLED(CONFIG_BT_PRIVACY_RANDOMIZE_IR)) {
		if (!bt_smp_irk_get(ir, ir_irk)) {
			irk = ir_irk;
		}
	}
#endif /* defined(CONFIG_BT_PRIVACY) */

	/* If true, `id_create` will randomize the IRK. */
	if (!irk && IS_ENABLED(CONFIG_BT_PRIVACY)) {
		/* `id_create` will not store the id when called before BT_DEV_READY.
		 * But since part of the id will be randomized, it needs to be stored.
		 */
		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			atomic_set_bit(bt_dev.flags, BT_DEV_STORE_ID);
		}
	}

	return id_create(BT_ID_DEFAULT, &addr, irk);
}

static uint8_t vs_read_static_addr(struct bt_hci_vs_static_addr addrs[], uint8_t size)
{
#if defined(CONFIG_BT_HCI_VS)
	struct bt_hci_rp_vs_read_static_addrs *rp;
	struct net_buf *rsp;
	int err, i;
	uint8_t cnt;

	if (!BT_VS_CMD_READ_STATIC_ADDRS(bt_dev.vs_commands)) {
		LOG_WRN("Read Static Addresses command not available");
		return 0;
	}

	err = bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS, NULL, &rsp);
	if (err) {
		LOG_WRN("Failed to read static addresses");
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
	    rsp->len < sizeof(struct bt_hci_rp_vs_read_static_addrs)) {
		LOG_WRN("Invalid Vendor HCI extensions");
		net_buf_unref(rsp);
		return 0;
	}

	rp = (void *)rsp->data;
	cnt = MIN(rp->num_addrs, size);

	if (IS_ENABLED(CONFIG_BT_HCI_VS_EXT_DETECT) &&
	    rsp->len != (sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			 rp->num_addrs *
			 sizeof(struct bt_hci_vs_static_addr))) {
		LOG_WRN("Invalid Vendor HCI extensions");
		net_buf_unref(rsp);
		return 0;
	}

	for (i = 0; i < cnt; i++) {
		memcpy(&addrs[i], &rp->a[i], sizeof(struct bt_hci_vs_static_addr));
	}

	net_buf_unref(rsp);
	if (!cnt) {
		LOG_WRN("No static addresses stored in controller");
	}

	return cnt;
#else
	return 0;
#endif
}

int bt_setup_random_id_addr(void)
{
	/* Only read the addresses if the user has not already configured one or
	 * more identities (!bt_dev.id_count).
	 */
	if (IS_ENABLED(CONFIG_BT_HCI_VS) && !bt_dev.id_count) {
		struct bt_hci_vs_static_addr addrs[CONFIG_BT_ID_MAX];

		bt_dev.id_count = vs_read_static_addr(addrs, CONFIG_BT_ID_MAX);

		for (uint8_t i = 0; i < bt_dev.id_count; i++) {
			int err;
			bt_addr_le_t addr;
			uint8_t *irk = NULL;
			uint8_t ir_irk[16];

			if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
			    !IS_ENABLED(CONFIG_BT_PRIVACY_RANDOMIZE_IR)) {
				if (!bt_smp_irk_get(addrs[i].ir, ir_irk)) {
					irk = ir_irk;
				}
			}

			/* If true, `id_create` will randomize the IRK. */
			if (!irk && IS_ENABLED(CONFIG_BT_PRIVACY)) {
				/* `id_create` will not store the id when called before
				 * BT_DEV_READY. But since part of the id will be
				 * randomized, it needs to be stored.
				 */
				if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
					atomic_set_bit(bt_dev.flags, BT_DEV_STORE_ID);
				}
			}

			bt_addr_copy(&addr.a, &addrs[i].bdaddr);
			addr.type = BT_ADDR_LE_RANDOM;

			err = id_create(i, &addr, irk);
			if (err) {
				return err;
			}
		}

		if (bt_dev.id_count > 0) {
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY) && IS_ENABLED(CONFIG_BT_SETTINGS)) {
		atomic_set_bit(bt_dev.flags, BT_DEV_STORE_ID);
	}

	return bt_id_create(NULL, NULL);
}

#if defined(CONFIG_BT_CENTRAL)
static inline bool rpa_timeout_valid_check(void)
{
#if defined(CONFIG_BT_PRIVACY)
	uint32_t remaining_ms = k_ticks_to_ms_floor32(
		k_work_delayable_remaining_get(&bt_dev.rpa_update));
	/* Check if create conn timeout will happen before RPA timeout. */
	return remaining_ms > (10 * bt_dev.create_param.timeout);
#else
	return true;
#endif
}

int bt_id_set_create_conn_own_addr(bool use_filter, uint8_t *own_addr_type)
{
	int err;

	CHECKIF(own_addr_type == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		if (use_filter || rpa_timeout_valid_check()) {
			err = bt_id_set_private_addr(BT_ID_DEFAULT);
			if (err) {
				return err;
			}
		} else {
			/* Force new RPA timeout so that RPA timeout is not
			 * triggered while direct initiator is active.
			 */
			le_force_rpa_timeout();
		}

		if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			*own_addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
		} else {
			*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
		}
	} else {
		const bt_addr_le_t *addr = &bt_dev.id_addr[BT_ID_DEFAULT];

		/* If Static Random address is used as Identity address we
		 * need to restore it before creating connection. Otherwise
		 * NRPA used for active scan could be used for connection.
		 */
		if (addr->type == BT_ADDR_LE_RANDOM) {
			err = set_random_address(&addr->a);
			if (err) {
				return err;
			}

			*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
		} else {
			/* If address type is not random, it's public. If it's public then we assume
			 * it's the Controller's public address.
			 */
			*own_addr_type = BT_HCI_OWN_ADDR_PUBLIC;
		}
	}

	return 0;
}
#endif /* defined(CONFIG_BT_CENTRAL) */

#if defined(CONFIG_BT_OBSERVER)
static bool is_adv_using_rand_addr(void)
{
	struct bt_le_ext_adv *adv;

	if (!IS_ENABLED(CONFIG_BT_BROADCASTER) ||
	    (IS_ENABLED(CONFIG_BT_EXT_ADV) &&
	     BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
		/* When advertising is not enabled or is using extended
		 * advertising HCI commands then only the scanner uses the set
		 * random address command.
		 */
		return false;
	}

	adv = bt_le_adv_lookup_legacy();

	return adv && atomic_test_bit(adv->flags, BT_ADV_ENABLED);
}

int bt_id_set_scan_own_addr(bool active_scan, uint8_t *own_addr_type)
{
	int err;

	CHECKIF(own_addr_type == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {

		if (BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			*own_addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
		} else {
			*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
		}

		err = bt_id_set_private_addr(BT_ID_DEFAULT);
		if (err == -EACCES && (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) ||
				       atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING))) {
			LOG_WRN("Set random addr failure ignored in scan/init state");

			return 0;
		} else if (err) {
			return err;
		}
	} else {
		/* Use NRPA unless identity has been explicitly requested
		 * (through Kconfig).
		 * Use same RPA as legacy advertiser if advertising.
		 */
		if (!IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY) &&
		    !is_adv_using_rand_addr()) {
			err = bt_id_set_private_addr(BT_ID_DEFAULT);
			if (err) {
				if (active_scan || !is_adv_using_rand_addr()) {
					return err;
				}

				LOG_WRN("Ignoring failure to set address for passive scan (%d)",
					err);
			}

			*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
		} else if (IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY)) {
			if (bt_dev.id_addr[BT_ID_DEFAULT].type == BT_ADDR_LE_RANDOM) {
				/* If scanning with Identity Address we must set the
				 * random identity address for both active and passive
				 * scanner in order to receive adv reports that are
				 * directed towards this identity.
				 */
				err = set_random_address(&bt_dev.id_addr[BT_ID_DEFAULT].a);
				if (err) {
					return err;
				}

				*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
			} else if (bt_dev.id_addr[BT_ID_DEFAULT].type == BT_ADDR_LE_PUBLIC) {
				*own_addr_type = BT_HCI_OWN_ADDR_PUBLIC;
			}
		}
	}

	return 0;
}
#endif /* defined(CONFIG_BT_OBSERVER) */

int bt_id_set_adv_own_addr(struct bt_le_ext_adv *adv, uint32_t options,
			   bool dir_adv, uint8_t *own_addr_type)
{
	const bt_addr_le_t *id_addr;
	int err = 0;

	CHECKIF(adv == NULL || own_addr_type == NULL) {
		return -EINVAL;
	}

	/* Set which local identity address we're advertising with */
	id_addr = &bt_dev.id_addr[adv->id];

	/* Short-circuit to force NRPA usage */
	if (options & BT_LE_ADV_OPT_USE_NRPA) {
		if (options & BT_LE_ADV_OPT_USE_IDENTITY) {
			LOG_ERR("Can't set both IDENTITY & NRPA");

			return -EINVAL;
		}

		err = bt_id_set_adv_private_addr(adv);
		if (err) {
			return err;
		}
		*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;

		return 0;
	}

	if (options & _BT_LE_ADV_OPT_CONNECTABLE) {
		if (dir_adv && (options & BT_LE_ADV_OPT_DIR_ADDR_RPA) &&
		    !BT_FEAT_LE_PRIVACY(bt_dev.le.features)) {
			return -ENOTSUP;
		}

		if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
		    !(options & BT_LE_ADV_OPT_USE_IDENTITY)) {
			err = bt_id_set_adv_private_addr(adv);
			if (err) {
				return err;
			}

			if (dir_adv && (options & BT_LE_ADV_OPT_DIR_ADDR_RPA)) {
				*own_addr_type = BT_HCI_OWN_ADDR_RPA_OR_RANDOM;
			} else {
				*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
			}
		} else {
			/*
			 * If Static Random address is used as Identity
			 * address we need to restore it before advertising
			 * is enabled. Otherwise NRPA used for active scan
			 * could be used for advertising.
			 */
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				err = bt_id_set_adv_random_addr(adv, &id_addr->a);
				if (err) {
					return err;
				}

				*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
			} else if (id_addr->type == BT_ADDR_LE_PUBLIC) {
				*own_addr_type = BT_HCI_OWN_ADDR_PUBLIC;
			}

			if (dir_adv && (options & BT_LE_ADV_OPT_DIR_ADDR_RPA)) {
				*own_addr_type |= BT_HCI_OWN_ADDR_RPA_MASK;
			}
		}
	} else {
		if (options & BT_LE_ADV_OPT_USE_IDENTITY) {
			if (id_addr->type == BT_ADDR_LE_RANDOM) {
				err = bt_id_set_adv_random_addr(adv, &id_addr->a);
				if (err) {
					return err;
				}

				*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
			} else if (id_addr->type == BT_ADDR_LE_PUBLIC) {
				*own_addr_type = BT_HCI_OWN_ADDR_PUBLIC;
			}

			if (options & BT_LE_ADV_OPT_DIR_ADDR_RPA) {
				*own_addr_type |= BT_HCI_OWN_ADDR_RPA_MASK;
			}
		} else if (!(IS_ENABLED(CONFIG_BT_EXT_ADV) &&
			     BT_DEV_FEAT_LE_EXT_ADV(bt_dev.le.features))) {
			/* In case advertising set random address is not
			 * available we must handle the shared random address
			 * problem.
			 */
#if defined(CONFIG_BT_OBSERVER)
			bool scan_enabled = false;

			/* If active scan with NRPA is ongoing refresh NRPA */
			if (!IS_ENABLED(CONFIG_BT_PRIVACY) &&
			    !IS_ENABLED(CONFIG_BT_SCAN_WITH_IDENTITY) &&
			    atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING)) {
				scan_enabled = true;
				bt_le_scan_set_enable(BT_HCI_LE_SCAN_DISABLE);
			}
#endif /* defined(CONFIG_BT_OBSERVER) */
			err = bt_id_set_adv_private_addr(adv);
			*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;

#if defined(CONFIG_BT_OBSERVER)
			if (scan_enabled) {
				bt_le_scan_set_enable(BT_HCI_LE_SCAN_ENABLE);
			}
#endif /* defined(CONFIG_BT_OBSERVER) */
		} else {
			err = bt_id_set_adv_private_addr(adv);
			*own_addr_type = BT_HCI_OWN_ADDR_RANDOM;
		}

		if (err) {
			return err;
		}
	}

	return 0;
}

#if defined(CONFIG_BT_CLASSIC)
int bt_br_oob_get_local(struct bt_br_oob *oob)
{
	CHECKIF(oob == NULL) {
		return -EINVAL;
	}

	bt_addr_copy(&oob->addr, &bt_dev.id_addr[0].a);

	return 0;
}
#endif /* CONFIG_BT_CLASSIC */

int bt_le_oob_get_local(uint8_t id, struct bt_le_oob *oob)
{
	struct bt_le_ext_adv *adv = NULL;
	int err;

	CHECKIF(oob == NULL) {
		return -EINVAL;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (id >= CONFIG_BT_ID_MAX) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		adv = bt_le_adv_lookup_legacy();
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !(adv && adv->id == id &&
	      atomic_test_bit(adv->flags, BT_ADV_ENABLED) &&
	      atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY) &&
	      bt_dev.id_addr[id].type == BT_ADDR_LE_RANDOM)) {
		if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
		    atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING)) {
			struct bt_conn *conn;

			conn = bt_conn_lookup_state_le(BT_ID_DEFAULT, NULL,
						       BT_CONN_SCAN_BEFORE_INITIATING);
			if (conn) {
				/* Cannot set new RPA while creating
				 * connections.
				 */
				bt_conn_unref(conn);
				return -EINVAL;
			}
		}

		if (adv &&
		    atomic_test_bit(adv->flags, BT_ADV_ENABLED) &&
		    atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY) &&
		    (bt_dev.id_addr[id].type == BT_ADDR_LE_RANDOM)) {
			/* Cannot set a new RPA address while advertising with
			 * random static identity address for a different
			 * identity.
			 */
			return -EINVAL;
		}

		if (IS_ENABLED(CONFIG_BT_OBSERVER) &&
		    CONFIG_BT_ID_MAX > 1 &&
		    id != BT_ID_DEFAULT &&
		    (atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING) ||
		     atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING))) {
			/* Cannot switch identity of scanner or initiator */
			return -EINVAL;
		}

		le_force_rpa_timeout();

		bt_addr_le_copy(&oob->addr, &bt_dev.random_addr);
	} else {
		bt_addr_le_copy(&oob->addr, &bt_dev.id_addr[id]);
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		err = bt_smp_le_oob_generate_sc_data(&oob->le_sc_data);
		if (err && err != -ENOTSUP) {
			return err;
		}
	}

	return 0;
}

#if defined(CONFIG_BT_EXT_ADV)
int bt_le_ext_adv_oob_get_local(struct bt_le_ext_adv *adv,
				struct bt_le_oob *oob)
{
	int err;

	CHECKIF(adv == NULL || oob == NULL) {
		return -EINVAL;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY) &&
	    !atomic_test_bit(adv->flags, BT_ADV_USE_IDENTITY)) {
		/* Don't refresh RPA addresses if the RPA is new.
		 * This allows back to back calls to this function or
		 * bt_le_oob_get_local to not invalidate the previously set
		 * RPAs.
		 */
		if (!atomic_test_bit(adv->flags, BT_ADV_LIMITED) &&
		    !bt_id_rpa_is_new()) {
			if (IS_ENABLED(CONFIG_BT_CENTRAL) &&
			    atomic_test_bit(bt_dev.flags, BT_DEV_INITIATING)) {
				struct bt_conn *conn;

				conn = bt_conn_lookup_state_le(
					BT_ID_DEFAULT, NULL,
					BT_CONN_SCAN_BEFORE_INITIATING);

				if (conn) {
					/* Cannot set new RPA while creating
					 * connections.
					 */
					bt_conn_unref(conn);
					return -EINVAL;
				}
			}

			le_force_rpa_timeout();
		}

		bt_addr_le_copy(&oob->addr, &adv->random_addr);
	} else {
		bt_addr_le_copy(&oob->addr, &bt_dev.id_addr[adv->id]);
	}

	if (IS_ENABLED(CONFIG_BT_SMP)) {
		err = bt_smp_le_oob_generate_sc_data(&oob->le_sc_data);
		if (err && err != -ENOTSUP) {
			return err;
		}
	}

	return 0;
}
#endif /* defined(CONFIG_BT_EXT_ADV) */

#if defined(CONFIG_BT_SMP)
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
int bt_le_oob_set_legacy_tk(struct bt_conn *conn, const uint8_t *tk)
{
	if (!bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		LOG_DBG("Invalid connection type: %u for %p", conn->type, conn);
		return -EINVAL;
	}

	CHECKIF(tk == NULL) {
		return -EINVAL;
	}

	return bt_smp_le_oob_set_tk(conn, tk);
}
#endif /* !defined(CONFIG_BT_SMP_SC_PAIR_ONLY) */

#if !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)
int bt_le_oob_set_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data *oobd_local,
			  const struct bt_le_oob_sc_data *oobd_remote)
{
	if (!bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		LOG_DBG("Invalid connection type: %u for %p", conn->type, conn);
		return -EINVAL;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	return bt_smp_le_oob_set_sc_data(conn, oobd_local, oobd_remote);
}

int bt_le_oob_get_sc_data(struct bt_conn *conn,
			  const struct bt_le_oob_sc_data **oobd_local,
			  const struct bt_le_oob_sc_data **oobd_remote)
{
	if (!bt_conn_is_type(conn, BT_CONN_TYPE_LE)) {
		LOG_ERR("Invalid connection: %p", conn);
		LOG_ERR("Invalid connection type: %u for %p", conn->handle, conn);
		return -EINVAL;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	return bt_smp_le_oob_get_sc_data(conn, oobd_local, oobd_remote);
}
#endif /* !defined(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY) */
#endif /* defined(CONFIG_BT_SMP) */

int bt_id_init(void)
{
	int err;

#if defined(CONFIG_BT_PRIVACY)
	k_work_init_delayable(&bt_dev.rpa_update, rpa_timeout);
#if defined(CONFIG_BT_RPA_SHARING)
	for (uint8_t id = 0U; id < ARRAY_SIZE(bt_dev.rpa); id++) {
		bt_addr_copy(&bt_dev.rpa[id], BT_ADDR_NONE);
	}
#endif
#endif

	if (!IS_ENABLED(CONFIG_BT_SETTINGS) && !bt_dev.id_count) {
		LOG_DBG("No user identity. Trying to set public.");

		err = bt_setup_public_id_addr();
		if (err) {
			LOG_ERR("Unable to set identity address");
			return err;
		}
	}

	if (!IS_ENABLED(CONFIG_BT_SETTINGS) && !bt_dev.id_count) {
		LOG_DBG("No public address. Trying to set static random.");

		err = bt_setup_random_id_addr();
		if (err) {
			LOG_ERR("Unable to set identity address");
			return err;
		}

		/* The passive scanner just sends a dummy address type in the
		 * command. If the first activity does this, and the dummy type
		 * is a random address, it needs a valid value, even though it's
		 * not actually used.
		 */
		err = set_random_address(&bt_dev.id_addr[0].a);
		if (err) {
			LOG_ERR("Unable to set random address");
			return err;
		}
	}

	return 0;
}
