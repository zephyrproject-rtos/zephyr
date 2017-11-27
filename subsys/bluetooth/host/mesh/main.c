/*  Bluetooth Mesh */

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
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG)
#include "common/log.h"

#include "test.h"
#include "adv.h"
#include "prov.h"
#include "net.h"
#include "beacon.h"
#include "lpn.h"
#include "friend.h"
#include "transport.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "mesh.h"

static bool provisioned;

int bt_mesh_provision(const u8_t net_key[16], u16_t net_idx,
		      u8_t flags, u32_t iv_index, u32_t seq,
		      u16_t addr, const u8_t dev_key[16])
{
	int err;

	BT_INFO("Primary Element: 0x%04x", addr);
	BT_DBG("net_idx 0x%04x flags 0x%02x iv_index 0x%04x",
	       net_idx, flags, iv_index);

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		bt_mesh_proxy_prov_disable();
	}

	err = bt_mesh_net_create(net_idx, flags, net_key, iv_index);
	if (err) {
		if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
			bt_mesh_proxy_prov_enable();
		}

		return err;
	}

	bt_mesh.seq = seq;

	bt_mesh_comp_provision(addr);

	memcpy(bt_mesh.dev_key, dev_key, 16);

	provisioned = true;

	if (bt_mesh_beacon_get() == BT_MESH_BEACON_ENABLED) {
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
		bt_mesh_prov_complete(net_idx, addr);
	}

	return 0;
}

void bt_mesh_reset(void)
{
	if (!provisioned) {
		return;
	}

	bt_mesh_comp_unprovision();

	bt_mesh.iv_index = 0;
	bt_mesh.seq = 0;
	bt_mesh.iv_update = 0;
	bt_mesh.pending_update = 0;
	bt_mesh.valid = 0;
	bt_mesh.last_update = 0;
	bt_mesh.ivu_initiator = 0;

	k_delayed_work_cancel(&bt_mesh.ivu_complete);

	bt_mesh_cfg_reset();

	bt_mesh_rx_reset();
	bt_mesh_tx_reset();

	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
		bt_mesh_lpn_disable(true);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
		bt_mesh_friend_clear_net_idx(BT_MESH_KEY_ANY);
	}

	if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
		bt_mesh_proxy_gatt_disable();
	}

	memset(bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));

	memset(bt_mesh.rpl, 0, sizeof(bt_mesh.rpl));

	provisioned = false;

	bt_mesh_scan_disable();
	bt_mesh_beacon_disable();

	if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
		bt_mesh_prov_reset();
	}
}

bool bt_mesh_is_provisioned(void)
{
	return provisioned;
}

int bt_mesh_prov_enable(bt_mesh_prov_bearer_t bearers)
{
	if (bt_mesh_is_provisioned()) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
	    (bearers & BT_MESH_PROV_ADV)) {
		/* Make sure we're scanning for provisioning inviations */
		bt_mesh_scan_enable();
		/* Enable unprovisioned beacon sending */
		bt_mesh_beacon_enable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
	    (bearers & BT_MESH_PROV_GATT)) {
		bt_mesh_proxy_prov_enable();
		bt_mesh_adv_update();
	}

	return 0;
}

int bt_mesh_prov_disable(bt_mesh_prov_bearer_t bearers)
{
	if (bt_mesh_is_provisioned()) {
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
	    (bearers & BT_MESH_PROV_ADV)) {
		bt_mesh_beacon_disable();
		bt_mesh_scan_disable();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
	    (bearers & BT_MESH_PROV_GATT)) {
		bt_mesh_proxy_prov_disable();
		bt_mesh_adv_update();
	}

	return 0;
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

	if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
		err = bt_mesh_prov_init(prov);
		if (err) {
			return err;
		}
	}

	bt_mesh_net_init();
	bt_mesh_trans_init();
	bt_mesh_beacon_init();
	bt_mesh_adv_init();

	if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
		bt_mesh_proxy_init();
	}

	return 0;
}
