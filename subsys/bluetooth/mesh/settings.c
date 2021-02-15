/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/util.h>

#include <settings/settings.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_SETTINGS)
#define LOG_MODULE_NAME bt_mesh_settings
#include "common/log.h"

#include "mesh.h"
#include "subnet.h"
#include "app_keys.h"
#include "net.h"
#include "cdb.h"
#include "crypto.h"
#include "rpl.h"
#include "transport.h"
#include "heartbeat.h"
#include "access.h"
#include "proxy.h"
#include "settings.h"
#include "cfg.h"

static struct k_delayed_work pending_store;
static ATOMIC_DEFINE(pending_flags, BT_MESH_SETTINGS_FLAG_COUNT);

int bt_mesh_settings_set(settings_read_cb read_cb, void *cb_arg,
			 void *out, size_t read_len)
{
	ssize_t len;

	len = read_cb(cb_arg, out, read_len);
	if (len < 0) {
		BT_ERR("Failed to read value (err %zd)", len);
		return len;
	}

	BT_HEXDUMP_DBG(out, len, "val");

	if (len != read_len) {
		BT_ERR("Unexpected value length (%zd != %zu)", len, read_len);
		return -EINVAL;
	}

	return 0;
}

static int mesh_commit(void)
{
	if (!bt_mesh_subnet_next(NULL)) {
		/* Nothing to do since we're not yet provisioned */
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		bt_mesh_proxy_prov_disable(true);
	}

	bt_mesh_net_settings_commit();
	bt_mesh_model_settings_commit();

	atomic_set_bit(bt_mesh.flags, BT_MESH_VALID);

	bt_mesh_start();

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(bt_mesh, "bt/mesh", NULL, NULL, mesh_commit,
			       NULL);

/* Pending flags that use K_NO_WAIT as the storage timeout */
#define NO_WAIT_PENDING_BITS (BIT(BT_MESH_SETTINGS_NET_PENDING) |           \
			      BIT(BT_MESH_SETTINGS_IV_PENDING)  |           \
			      BIT(BT_MESH_SETTINGS_SEQ_PENDING) |           \
			      BIT(BT_MESH_SETTINGS_CDB_PENDING))

/* Pending flags that use CONFIG_BT_MESH_STORE_TIMEOUT */
#define GENERIC_PENDING_BITS (BIT(BT_MESH_SETTINGS_NET_KEYS_PENDING) |      \
			      BIT(BT_MESH_SETTINGS_APP_KEYS_PENDING) |      \
			      BIT(BT_MESH_SETTINGS_HB_PUB_PENDING)   |      \
			      BIT(BT_MESH_SETTINGS_CFG_PENDING)      |      \
			      BIT(BT_MESH_SETTINGS_MOD_PENDING))

void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag)
{
	int32_t timeout_ms, remaining;

	atomic_set_bit(pending_flags, flag);

	if (atomic_get(pending_flags) & NO_WAIT_PENDING_BITS) {
		timeout_ms = 0;
	} else if (atomic_test_bit(pending_flags,
				   BT_MESH_SETTINGS_RPL_PENDING) &&
		   (!(atomic_get(bt_mesh.flags) & GENERIC_PENDING_BITS) ||
		    (CONFIG_BT_MESH_RPL_STORE_TIMEOUT <
		     CONFIG_BT_MESH_STORE_TIMEOUT))) {
		timeout_ms = CONFIG_BT_MESH_RPL_STORE_TIMEOUT * MSEC_PER_SEC;
	} else {
		timeout_ms = CONFIG_BT_MESH_STORE_TIMEOUT * MSEC_PER_SEC;
	}

	remaining = k_delayed_work_remaining_get(&pending_store);
	if ((remaining > 0) && remaining < timeout_ms) {
		BT_DBG("Not rescheduling due to existing earlier deadline");
		return;
	}

	BT_DBG("Waiting %d seconds", timeout_ms / MSEC_PER_SEC);

	k_delayed_work_submit(&pending_store, K_MSEC(timeout_ms));
}

static void store_pending(struct k_work *work)
{
	BT_DBG("");

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_RPL_PENDING)) {
		bt_mesh_rpl_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_NET_KEYS_PENDING)) {
		bt_mesh_subnet_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_APP_KEYS_PENDING)) {
		bt_mesh_app_key_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_NET_PENDING)) {
		bt_mesh_net_pending_net_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_IV_PENDING)) {
		bt_mesh_net_pending_iv_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_SEQ_PENDING)) {
		bt_mesh_net_pending_seq_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_HB_PUB_PENDING)) {
		bt_mesh_hb_pub_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_CFG_PENDING)) {
		bt_mesh_cfg_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_MOD_PENDING)) {
		bt_mesh_model_pending_store();
	}

	if (atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_VA_PENDING)) {
		bt_mesh_va_pending_store();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB) &&
	    atomic_test_and_clear_bit(pending_flags,
				      BT_MESH_SETTINGS_CDB_PENDING)) {
		bt_mesh_cdb_pending_store();
	}
}

void bt_mesh_settings_init(void)
{
	k_delayed_work_init(&pending_store, store_pending);
}
