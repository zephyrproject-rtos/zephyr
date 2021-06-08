/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdbool.h>
#include <errno.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG)
#define LOG_MODULE_NAME bt_mesh_main
#include "common/log.h"

#include "test.h"
#include "adv.h"
#include "prov.h"
#include "provisioner.h"
#include "net.h"
#include "subnet.h"
#include "app_keys.h"
#include "rpl.h"
#include "cfg.h"
#include "beacon.h"
#include "lpn.h"
#include "friend.h"
#include "transport.h"
#include "heartbeat.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "settings.h"
#include "mesh.h"

int bt_mesh_provision(const uint8_t net_key[16], uint16_t net_idx,
		      uint8_t flags, uint32_t iv_index, uint16_t addr,
		      const uint8_t dev_key[16])
{
	bool pb_gatt_enabled;
	int err;

	BT_INFO("Primary Element: 0x%04x", addr);
	BT_DBG("net_idx 0x%04x flags 0x%02x iv_index 0x%04x",
	       net_idx, flags, iv_index);

	if (atomic_test_and_set_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		if (bt_mesh_proxy_prov_disable(false) == 0) {
			pb_gatt_enabled = true;
		} else {
			pb_gatt_enabled = false;
		}
	} else {
		pb_gatt_enabled = false;
	}

	/*
	 * FIXME:
	 * Should net_key and iv_index be over-ridden?
	 */
	if (IS_ENABLED(CONFIG_BT_MESH_CDB) &&
	    atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID)) {
		const struct bt_mesh_comp *comp;
		const struct bt_mesh_prov *prov;
		struct bt_mesh_cdb_node *node;

		comp = bt_mesh_comp_get();
		if (comp == NULL) {
			BT_ERR("Failed to get node composition");
			atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
			return -EINVAL;
		}

		if (!bt_mesh_cdb_subnet_get(net_idx)) {
			BT_ERR("No subnet with idx %d", net_idx);
			atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
			return -ENOENT;
		}

		prov = bt_mesh_prov_get();
		node = bt_mesh_cdb_node_alloc(prov->uuid, addr,
					      comp->elem_count, net_idx);
		if (node == NULL) {
			BT_ERR("Failed to allocate database node");
			atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
			return -ENOMEM;
		}

		addr = node->addr;
		iv_index = bt_mesh_cdb.iv_index;
		memcpy(node->dev_key, dev_key, 16);

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_cdb_node_store(node);
		}
	}

	err = bt_mesh_net_create(net_idx, flags, net_key, iv_index);
	if (err) {
		atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);

		if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) && pb_gatt_enabled) {
			(void)bt_mesh_proxy_prov_enable();
		}

		return err;
	}

	bt_mesh.seq = 0U;

	bt_mesh_comp_provision(addr);

	memcpy(bt_mesh.dev_key, dev_key, 16);

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    IS_ENABLED(CONFIG_BT_MESH_LPN_SUB_ALL_NODES_ADDR)) {
		bt_mesh_lpn_group_add(BT_MESH_ADDR_ALL_NODES);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_pending_net_store();
	}

	bt_mesh_start();

	return 0;
}

int bt_mesh_provision_adv(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			  uint8_t attention_duration)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EINVAL;
	}

	if (bt_mesh_subnet_get(net_idx) == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROVISIONER) &&
	    IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		return bt_mesh_pb_adv_open(uuid, net_idx, addr,
					   attention_duration);
	}

	return -ENOTSUP;
}

void bt_mesh_reset(void)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return;
	}

	bt_mesh.iv_index = 0U;
	bt_mesh.seq = 0U;

	memset(bt_mesh.flags, 0, sizeof(bt_mesh.flags));

	/* If this fails, the work handler will return early on the next
	 * execution, as the device is not provisioned. If the device is
	 * reprovisioned, the timer is always restarted.
	 */
	(void)k_work_cancel_delayable(&bt_mesh.ivu_timer);

	bt_mesh_model_reset();
	bt_mesh_cfg_default_set();
	bt_mesh_trans_reset();
	bt_mesh_app_keys_reset();
	bt_mesh_net_keys_reset();

	bt_mesh_net_loopback_clear(BT_MESH_KEY_ANY);

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		if (IS_ENABLED(CONFIG_BT_MESH_LPN_SUB_ALL_NODES_ADDR)) {
			uint16_t group = BT_MESH_ADDR_ALL_NODES;

			bt_mesh_lpn_group_del(&group, 1);
		}

		bt_mesh_lpn_disable(true);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		bt_mesh_friends_clear();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		bt_mesh_proxy_gatt_disable();
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_clear();
	}

	(void)memset(bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));

	bt_mesh_scan_disable();
	bt_mesh_beacon_disable();

	bt_mesh_comp_unprovision();

	if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
		bt_mesh_prov_reset();
	}
}

bool bt_mesh_is_provisioned(void)
{
	return atomic_test_bit(bt_mesh.flags, BT_MESH_VALID);
}

static void model_suspend(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
			  bool vnd, bool primary, void *user_data)
{
	if (mod->pub && mod->pub->update) {
		mod->pub->count = 0U;
		/* If this fails, the work handler will check the suspend call
		 * and exit without transmitting.
		 */
		(void)k_work_cancel_delayable(&mod->pub->timer);
	}
}

int bt_mesh_suspend(void)
{
	int err;

	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EINVAL;
	}

	if (atomic_test_and_set_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		return -EALREADY;
	}

	err = bt_mesh_scan_disable();
	if (err) {
		atomic_clear_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
		BT_WARN("Disabling scanning failed (err %d)", err);
		return err;
	}

	bt_mesh_hb_suspend();

	if (bt_mesh_beacon_enabled()) {
		bt_mesh_beacon_disable();
	}

	bt_mesh_model_foreach(model_suspend, NULL);

	return 0;
}

static void model_resume(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
			  bool vnd, bool primary, void *user_data)
{
	if (mod->pub && mod->pub->update) {
		int32_t period_ms = bt_mesh_model_pub_period_get(mod);

		if (period_ms) {
			k_work_reschedule(&mod->pub->timer,
					  K_MSEC(period_ms));
		}
	}
}

int bt_mesh_resume(void)
{
	int err;

	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		return -EALREADY;
	}

	err = bt_mesh_scan_enable();
	if (err) {
		BT_WARN("Re-enabling scanning failed (err %d)", err);
		atomic_set_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
		return err;
	}

	bt_mesh_hb_resume();

	if (bt_mesh_beacon_enabled()) {
		bt_mesh_beacon_enable();
	}

	bt_mesh_model_foreach(model_resume, NULL);

	return err;
}

int bt_mesh_init(const struct bt_mesh_prov *prov,
		 const struct bt_mesh_comp *comp)
{
	int err;

	err = bt_mesh_test();
	if (err) {
		return err;
	}

	err = bt_mesh_comp_register(comp);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT)) {
		bt_mesh_proxy_init();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
		err = bt_mesh_prov_init(prov);
		if (err) {
			return err;
		}
	}

	bt_mesh_cfg_default_set();
	bt_mesh_net_init();
	bt_mesh_trans_init();
	bt_mesh_hb_init();
	bt_mesh_beacon_init();
	bt_mesh_adv_init();

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_init();
	}

	return 0;
}

static void model_start(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
			bool vnd, bool primary, void *user_data)
{
	if (mod->cb && mod->cb->start) {
		mod->cb->start(mod);
	}
}

int bt_mesh_start(void)
{
	int err;

	err = bt_mesh_adv_enable();
	if (err) {
		BT_ERR("Failed enabling advertiser");
		return err;
	}

	if (bt_mesh_beacon_enabled()) {
		bt_mesh_beacon_enable();
	} else {
		bt_mesh_beacon_disable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) &&
	    bt_mesh_gatt_proxy_get() != BT_MESH_GATT_PROXY_NOT_SUPPORTED) {
		bt_mesh_proxy_gatt_enable();
		bt_mesh_adv_update();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_init();
	} else {
		bt_mesh_scan_enable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		bt_mesh_friend_init();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
		struct bt_mesh_subnet *sub = bt_mesh_subnet_next(NULL);
		uint16_t addr = bt_mesh_primary_addr();

		bt_mesh_prov_complete(sub->net_idx, addr);
	}

	bt_mesh_hb_start();

	bt_mesh_model_foreach(model_start, NULL);

	return 0;
}
