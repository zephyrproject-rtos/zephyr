/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <sys/types.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>

#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/mesh.h>

#include "host/hci_core.h"
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
#include "pb_gatt_srv.h"
#include "settings.h"
#include "cfg.h"
#include "solicitation.h"
#include "va.h"

#define LOG_LEVEL CONFIG_BT_MESH_SETTINGS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_settings);

#ifdef CONFIG_BT_MESH_RPL_STORE_TIMEOUT
#define RPL_STORE_TIMEOUT CONFIG_BT_MESH_RPL_STORE_TIMEOUT
#else
#define RPL_STORE_TIMEOUT (-1)
#endif

#ifdef CONFIG_BT_MESH_SETTINGS_WORKQ_PRIO
#define SETTINGS_WORKQ_PRIO CONFIG_BT_MESH_SETTINGS_WORKQ_PRIO
#else
#define SETTINGS_WORKQ_PRIO 1
#endif

#ifdef CONFIG_BT_MESH_SETTINGS_WORKQ_STACK_SIZE
#define SETTINGS_WORKQ_STACK_SIZE CONFIG_BT_MESH_SETTINGS_WORKQ_STACK_SIZE
#else
#define SETTINGS_WORKQ_STACK_SIZE 0
#endif

static struct k_work_q settings_work_q;
static K_THREAD_STACK_DEFINE(settings_work_stack, SETTINGS_WORKQ_STACK_SIZE);

static struct k_work_delayable pending_store;
static ATOMIC_DEFINE(pending_flags, BT_MESH_SETTINGS_FLAG_COUNT);

int bt_mesh_settings_set(settings_read_cb read_cb, void *cb_arg,
			 void *out, size_t read_len)
{
	ssize_t len;

	len = read_cb(cb_arg, out, read_len);
	if (len < 0) {
		LOG_ERR("Failed to read value (err %zd)", len);
		return len;
	}

	LOG_HEXDUMP_DBG(out, len, "val");

	if (len != read_len) {
		LOG_ERR("Unexpected value length (%zd != %zu)", len, read_len);
		return -EINVAL;
	}

	return 0;
}

static int mesh_commit(void)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_INIT)) {
		return 0;
	}

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_ENABLE)) {
		/* The Bluetooth mesh settings loader calls bt_mesh_start() immediately
		 * after loading the settings. This is not intended to work before
		 * bt_enable(). The doc on @ref bt_enable requires the "bt/" settings
		 * tree to be loaded after @ref bt_enable is completed, so this handler
		 * will be called again later.
		 */
		return 0;
	}

	if (!bt_mesh_subnet_next(NULL)) {
		/* Nothing to do since we're not yet provisioned */
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		(void)bt_mesh_pb_gatt_srv_disable();
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
			      BIT(BT_MESH_SETTINGS_MOD_PENDING)      |      \
			      BIT(BT_MESH_SETTINGS_VA_PENDING)       |      \
			      BIT(BT_MESH_SETTINGS_SSEQ_PENDING)     |      \
			      BIT(BT_MESH_SETTINGS_COMP_PENDING)     |      \
			      BIT(BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING))

void bt_mesh_settings_store_schedule(enum bt_mesh_settings_flag flag)
{
	uint32_t timeout_ms, remaining_ms;

	atomic_set_bit(pending_flags, flag);

	if (atomic_get(pending_flags) & NO_WAIT_PENDING_BITS) {
		timeout_ms = 0;
	} else if (IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS) && RPL_STORE_TIMEOUT >= 0 &&
		   (atomic_test_bit(pending_flags, BT_MESH_SETTINGS_RPL_PENDING) ||
		     atomic_test_bit(pending_flags, BT_MESH_SETTINGS_SRPL_PENDING)) &&
		   !(atomic_get(pending_flags) & GENERIC_PENDING_BITS)) {
		timeout_ms = RPL_STORE_TIMEOUT * MSEC_PER_SEC;
	} else {
		timeout_ms = CONFIG_BT_MESH_STORE_TIMEOUT * MSEC_PER_SEC;
	}

	remaining_ms = k_ticks_to_ms_floor32(k_work_delayable_remaining_get(&pending_store));
	LOG_DBG("Waiting %u ms vs rem %u ms", timeout_ms, remaining_ms);

	/* If the new deadline is sooner, override any existing
	 * deadline; otherwise schedule without changing any existing
	 * deadline.
	 */
	if (timeout_ms < remaining_ms) {
		if (IS_ENABLED(CONFIG_BT_MESH_SETTINGS_WORKQ)) {
			k_work_reschedule_for_queue(&settings_work_q, &pending_store,
						    K_MSEC(timeout_ms));
		} else {
			k_work_reschedule(&pending_store, K_MSEC(timeout_ms));
		}
	} else {
		if (IS_ENABLED(CONFIG_BT_MESH_SETTINGS_WORKQ)) {
			k_work_schedule_for_queue(&settings_work_q, &pending_store,
						  K_MSEC(timeout_ms));
		} else {
			k_work_schedule(&pending_store, K_MSEC(timeout_ms));
		}
	}
}

void bt_mesh_settings_store_cancel(enum bt_mesh_settings_flag flag)
{
	atomic_clear_bit(pending_flags, flag);
}

static void store_pending(struct k_work *work)
{
	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_MESH_RPL_STORAGE_MODE_SETTINGS) &&
	    atomic_test_and_clear_bit(pending_flags, BT_MESH_SETTINGS_RPL_PENDING)) {
		bt_mesh_rpl_pending_store(BT_MESH_ADDR_ALL_NODES);
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
				      BT_MESH_SETTINGS_DEV_KEY_CAND_PENDING)) {
		bt_mesh_net_pending_dev_key_cand_store();
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
				      BT_MESH_SETTINGS_COMP_PENDING)) {
		bt_mesh_comp_data_pending_clear();
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

	if (IS_ENABLED(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV) &&
		atomic_test_and_clear_bit(pending_flags,
					  BT_MESH_SETTINGS_SRPL_PENDING)) {
		bt_mesh_srpl_pending_store();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION) &&
		atomic_test_and_clear_bit(pending_flags,
					  BT_MESH_SETTINGS_SSEQ_PENDING)) {
		bt_mesh_sseq_pending_store();
	}
}

void bt_mesh_settings_init(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_SETTINGS_WORKQ)) {
		k_work_queue_start(&settings_work_q, settings_work_stack,
				   K_THREAD_STACK_SIZEOF(settings_work_stack),
				   K_PRIO_COOP(SETTINGS_WORKQ_PRIO), NULL);
		k_thread_name_set(&settings_work_q.thread, "BT Mesh settings workq");
	}

	k_work_init_delayable(&pending_store, store_pending);
}

void bt_mesh_settings_store_pending(void)
{
	(void)k_work_cancel_delayable(&pending_store);

	store_pending(&pending_store.work);
}
