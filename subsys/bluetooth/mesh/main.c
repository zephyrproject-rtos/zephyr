/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdbool.h>
#include <errno.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/mesh.h>

#include <zephyr/logging/log.h>
#include <common/bt_str.h>

#include "test.h"
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
#include "pb_gatt_srv.h"
#include "settings.h"
#include "mesh.h"
#include "solicitation.h"
#include "gatt_cli.h"
#include "crypto.h"

LOG_MODULE_REGISTER(bt_mesh_main, CONFIG_BT_MESH_LOG_LEVEL);

int bt_mesh_provision(const uint8_t net_key[16], uint16_t net_idx,
		      uint8_t flags, uint32_t iv_index, uint16_t addr,
		      const uint8_t dev_key[16])
{
	struct bt_mesh_key mesh_dev_key;
	struct bt_mesh_key mesh_net_key;
	bool is_net_key_valid = false;
	bool is_dev_key_valid = false;
	int err = 0;

	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_INIT)) {
		return -ENODEV;
	}

	struct bt_mesh_cdb_subnet *subnet = NULL;
	struct bt_mesh_cdb_node *node = NULL;

	LOG_INF("Primary Element: 0x%04x", addr);
	LOG_DBG("net_idx 0x%04x flags 0x%02x iv_index 0x%04x", net_idx, flags, iv_index);

	if (atomic_test_and_set_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB) &&
	    atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID)) {
		const struct bt_mesh_comp *comp;
		const struct bt_mesh_prov *prov;

		comp = bt_mesh_comp_get();
		if (comp == NULL) {
			LOG_ERR("Failed to get node composition");
			atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
			return -EINVAL;
		}

		subnet = bt_mesh_cdb_subnet_get(net_idx);
		if (!subnet) {
			LOG_ERR("No subnet with idx %d", net_idx);
			atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
			return -ENOENT;
		}

		prov = bt_mesh_prov_get();
		node = bt_mesh_cdb_node_alloc(prov->uuid, addr,
					      comp->elem_count, net_idx);
		if (node == NULL) {
			LOG_ERR("Failed to allocate database node");
			atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
			return -ENOMEM;
		}

		if (BT_MESH_KEY_REFRESH(flags)) {
			subnet->kr_phase = BT_MESH_KR_PHASE_2;
		} else {
			subnet->kr_phase = BT_MESH_KR_NORMAL;
		}

		/* The primary network key has been imported during cdb creation.
		 * Importing here leaves it 'as is' if the key is the same.
		 * Otherwise, cdb replaces the old one with the new one.
		 */
		err = bt_mesh_cdb_subnet_key_import(subnet, BT_MESH_KEY_REFRESH(flags) ? 1 : 0,
						    net_key);
		if (err) {
			LOG_ERR("Failed to import cdb network key");
			goto end;
		}
		bt_mesh_cdb_subnet_store(subnet);

		addr = node->addr;
		bt_mesh_cdb_iv_update(iv_index, BT_MESH_IV_UPDATE(flags));

		err = bt_mesh_cdb_node_key_import(node, dev_key);
		if (err) {
			LOG_ERR("Failed to import cdb device key");
			goto end;
		}

		if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
			bt_mesh_cdb_node_store(node);
		}
	}

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_DEV, dev_key, &mesh_dev_key);
	if (err) {
		LOG_ERR("Failed to import device key");
		goto end;
	}
	is_dev_key_valid = true;

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_NET, net_key, &mesh_net_key);
	if (err) {
		LOG_ERR("Failed to import network key");
		goto end;
	}
	is_net_key_valid = true;

	err = bt_mesh_net_create(net_idx, flags, &mesh_net_key, iv_index);
	if (err) {
		atomic_clear_bit(bt_mesh.flags, BT_MESH_VALID);
		goto end;
	}

	bt_mesh_net_settings_commit();

	bt_mesh.seq = 0U;

	bt_mesh_comp_provision(addr);

	memcpy(&bt_mesh.dev_key, &mesh_dev_key, sizeof(struct bt_mesh_key));

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) &&
	    IS_ENABLED(CONFIG_BT_MESH_LPN_SUB_ALL_NODES_ADDR)) {
		bt_mesh_lpn_group_add(BT_MESH_ADDR_ALL_NODES);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_store();
	}

	bt_mesh_start();

end:
	if (err && node != NULL && IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		bt_mesh_cdb_node_del(node, true);
	}

	if (err && is_dev_key_valid) {
		bt_mesh_key_destroy(&mesh_dev_key);
	}

	if (err && is_net_key_valid) {
		bt_mesh_key_destroy(&mesh_net_key);
	}

	return err;
}

#if defined(CONFIG_BT_MESH_RPR_SRV)
void bt_mesh_reprovision(uint16_t addr)
{
	LOG_DBG("0x%04x devkey: %s", addr,
		bt_hex(&bt_mesh.dev_key_cand, sizeof(struct bt_mesh_key)));

	if (addr != bt_mesh_primary_addr()) {
		bt_mesh.seq = 0U;

		bt_mesh_comp_provision(addr);
		bt_mesh_trans_reset();

		if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
			bt_mesh_friends_clear();
		}

		if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
			bt_mesh_lpn_friendship_end();
		}
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		LOG_DBG("Storing network information persistently");
		bt_mesh_net_store();
		bt_mesh_net_seq_store(true);
		bt_mesh_comp_data_clear();
	}
}

void bt_mesh_dev_key_cand(const uint8_t *key)
{
	int err;

	LOG_DBG("%s", bt_hex(key, 16));

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_DEV, key, &bt_mesh.dev_key_cand);
	if (err) {
		LOG_ERR("Failed to import device key candidate");
		return;
	}

	atomic_set_bit(bt_mesh.flags, BT_MESH_DEVKEY_CAND);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_dev_key_cand_store();
	}
}

void bt_mesh_dev_key_cand_remove(void)
{
	if (!atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_DEVKEY_CAND)) {
		return;
	}

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_dev_key_cand_store();
	}
}

void bt_mesh_dev_key_cand_activate(void)
{
	if (!atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_DEVKEY_CAND)) {
		return;
	}

	bt_mesh_key_destroy(&bt_mesh.dev_key);
	memcpy(&bt_mesh.dev_key, &bt_mesh.dev_key_cand, sizeof(struct bt_mesh_key));
	memset(&bt_mesh.dev_key_cand, 0, sizeof(struct bt_mesh_key));

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_pending_net_store();
		bt_mesh_net_dev_key_cand_store();
	}
}
#endif

int bt_mesh_provision_adv(const uint8_t uuid[16], uint16_t net_idx,
			  uint16_t addr, uint8_t attention_duration)
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

int bt_mesh_provision_gatt(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
			   uint8_t attention_duration)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EINVAL;
	}

	if (bt_mesh_subnet_get(net_idx) == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT_CLIENT)) {
		return bt_mesh_pb_gatt_open(uuid, net_idx, addr,
					    attention_duration);
	}

	return -ENOTSUP;
}

int bt_mesh_provision_remote(struct bt_mesh_rpr_cli *cli,
			     const struct bt_mesh_rpr_node *srv,
			     const uint8_t uuid[16], uint16_t net_idx,
			     uint16_t addr)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EINVAL;
	}

	if (bt_mesh_subnet_get(net_idx) == NULL) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROVISIONER) &&
	    IS_ENABLED(CONFIG_BT_MESH_RPR_CLI)) {
		return bt_mesh_pb_remote_open(cli, srv, uuid, net_idx, addr);
	}

	return -ENOTSUP;
}

int bt_mesh_reprovision_remote(struct bt_mesh_rpr_cli *cli,
			       struct bt_mesh_rpr_node *srv,
			       uint16_t addr, bool comp_change)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID)) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROVISIONER) &&
	    IS_ENABLED(CONFIG_BT_MESH_RPR_CLI)) {
		return bt_mesh_pb_remote_open_node(cli, srv, addr, comp_change);
	}

	return -ENOTSUP;
}

void bt_mesh_reset(void)
{
	if (!atomic_test_bit(bt_mesh.flags, BT_MESH_VALID) ||
	    !atomic_test_bit(bt_mesh.flags, BT_MESH_INIT)) {
		return;
	}

	bt_mesh.iv_index = 0U;
	bt_mesh.ivu_duration = 0;
	bt_mesh.seq = 0U;

	memset(bt_mesh.flags, 0, sizeof(bt_mesh.flags));
	atomic_set_bit(bt_mesh.flags, BT_MESH_INIT);

	bt_mesh_scan_disable();

	/* If this fails, the work handler will return early on the next
	 * execution, as the device is not provisioned. If the device is
	 * reprovisioned, the timer is always restarted.
	 */
	(void)k_work_cancel_delayable(&bt_mesh.ivu_timer);

	bt_mesh_access_reset();
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
		(void)bt_mesh_proxy_gatt_disable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_CLIENT)) {
		bt_mesh_gatt_client_deinit();
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_net_clear();
	}

	bt_mesh_key_destroy(&bt_mesh.dev_key);
	memset(&bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));

	bt_mesh_beacon_disable();

	bt_mesh_comp_unprovision();

	if (IS_ENABLED(CONFIG_BT_MESH_PROXY_SOLICITATION)) {
		bt_mesh_sol_reset();
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_store_pending();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
		bt_mesh_prov_reset();
	}
}

bool bt_mesh_is_provisioned(void)
{
	return atomic_test_bit(bt_mesh.flags, BT_MESH_VALID);
}

static void model_suspend(const struct bt_mesh_model *mod, const struct bt_mesh_elem *elem,
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
		LOG_WRN("Disabling scanning failed (err %d)", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_CLIENT)) {
		bt_mesh_proxy_disconnect(BT_MESH_KEY_ANY);
	}

	bt_mesh_hb_suspend();

	bt_mesh_beacon_disable();

	bt_mesh_model_foreach(model_suspend, NULL);

	bt_mesh_access_suspend();

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		err = bt_mesh_pb_gatt_srv_disable();
		if (err && err != -EALREADY) {
			LOG_WRN("Disabling PB-GATT failed (err %d)", err);
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		err = bt_mesh_proxy_gatt_disable();
		if (err && err != -EALREADY) {
			LOG_WRN("Disabling GATT proxy failed (err %d)", err);
			return err;
		}
	}

	err = bt_mesh_adv_disable();
	if (err) {
		atomic_clear_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
		LOG_WRN("Disabling advertisers failed (err %d)", err);
		return err;
	}

	return 0;
}

static void model_resume(const struct bt_mesh_model *mod, const struct bt_mesh_elem *elem,
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

	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		bt_mesh_adv_init();
	}

	err = bt_mesh_adv_enable();
	if (err) {
		atomic_set_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
		LOG_WRN("Re-enabling advertisers failed (err %d)", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) && bt_mesh_is_provisioned()) {
		err = bt_mesh_proxy_gatt_enable();
		if (err) {
			LOG_WRN("Re-enabling GATT proxy failed (err %d)", err);
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) && !bt_mesh_is_provisioned()) {
		err = bt_mesh_pb_gatt_srv_enable();
		if (err) {
			LOG_WRN("Re-enabling PB-GATT failed (err %d)", err);
			return err;
		}
	}

	err = bt_mesh_scan_enable();
	if (err) {
		LOG_WRN("Re-enabling scanning failed (err %d)", err);
		atomic_set_bit(bt_mesh.flags, BT_MESH_SUSPENDED);
		return err;
	}

	bt_mesh_hb_resume();

	if (bt_mesh_beacon_enabled() ||
	    bt_mesh_priv_beacon_get() == BT_MESH_PRIV_BEACON_ENABLED) {
		bt_mesh_beacon_enable();
	}

	bt_mesh_model_foreach(model_resume, NULL);

	err = bt_mesh_adv_gatt_send();
	if (err && (err != -ENOTSUP)) {
		LOG_WRN("GATT send failed (err %d)", err);
		return err;
	}

	return 0;
}

int bt_mesh_init(const struct bt_mesh_prov *prov,
		 const struct bt_mesh_comp *comp)
{
	int err;

	if (atomic_test_and_set_bit(bt_mesh.flags, BT_MESH_INIT)) {
		return -EALREADY;
	}

	err = bt_mesh_test();
	if (err) {
		return err;
	}

	err = bt_mesh_crypto_init();
	if (err) {
		return err;
	}

	err = bt_mesh_comp_register(comp);
	if (err) {
		return err;
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
	bt_mesh_access_init();
	bt_mesh_hb_init();
	bt_mesh_beacon_init();
	bt_mesh_adv_init();

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_settings_init();
	}

	return 0;
}

static void model_start(const struct bt_mesh_model *mod, const struct bt_mesh_elem *elem,
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
		LOG_ERR("Failed enabling advertiser");
		return err;
	}


	if (bt_mesh_beacon_enabled() ||
	    bt_mesh_priv_beacon_get() == BT_MESH_PRIV_BEACON_ENABLED) {
		bt_mesh_beacon_enable();
	}

	if (!IS_ENABLED(CONFIG_BT_MESH_PROV) || !bt_mesh_prov_active() ||
	    bt_mesh_prov_link.bearer->type == BT_MESH_PROV_ADV) {
		if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
			(void)bt_mesh_pb_gatt_srv_disable();
		}

		if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
			(void)bt_mesh_proxy_gatt_enable();
		}
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_CLIENT)) {
		bt_mesh_gatt_client_init();
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
