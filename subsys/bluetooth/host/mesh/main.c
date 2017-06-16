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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_MESH_DEBUG)
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

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_GATT)) {
		bt_mesh_proxy_prov_disable();
	}

	err = bt_mesh_net_create(net_idx, flags, net_key, iv_index);
	if (err) {
		if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_GATT)) {
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

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_GATT_PROXY) &&
	    bt_mesh_gatt_proxy_get() != BT_MESH_GATT_PROXY_NOT_SUPPORTED) {
		bt_mesh_proxy_gatt_enable();
		bt_mesh_adv_update();
	}

	/* If PB-ADV is disabled then scanning will have been disabled */
	if (!IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_ADV)) {
		bt_mesh_scan_enable();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_LOW_POWER)) {
		bt_mesh_lpn_init();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_FRIEND)) {
		bt_mesh_friend_init();
	}

	return 0;
}

void bt_mesh_reset(void)
{
	if (!provisioned) {
		goto enable_beacon;
	}

	bt_mesh_comp_unprovision();

	bt_mesh.iv_index = 0;
	bt_mesh.seq = 0;
	bt_mesh.iv_update = 0;
	bt_mesh.valid = 0;
	bt_mesh.last_update = 0;
	bt_mesh.ivu_initiator = 0;

	k_delayed_work_cancel(&bt_mesh.ivu_complete);

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_LOW_POWER)) {
		bt_mesh_lpn_disable();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_GATT_PROXY)) {
		bt_mesh_proxy_gatt_disable();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_GATT)) {
		bt_mesh_proxy_prov_enable();
	}

	memset(bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));

	memset(bt_mesh.rpl, 0, sizeof(bt_mesh.rpl));

	provisioned = false;

enable_beacon:
	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_ADV)) {
		/* Make sure we're scanning for provisioning inviations */
		bt_mesh_scan_enable();
		/* Enable unprovisioned beacon sending */
		bt_mesh_beacon_enable();
	} else {
		bt_mesh_scan_disable();
		bt_mesh_beacon_disable();
	}
}

bool bt_mesh_is_provisioned(void)
{
	return provisioned;
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

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PROV)) {
		bt_mesh_prov_init(prov);
	}

	bt_mesh_net_init();
	bt_mesh_trans_init();
	bt_mesh_beacon_init();
	bt_mesh_adv_init();

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PROXY)) {
		bt_mesh_proxy_init();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_ADV)) {
		/* Make sure we're scanning for provisioning inviations */
		bt_mesh_scan_enable();
		/* Enable unprovisioned beacon sending */
		bt_mesh_beacon_enable();
	}

	if (IS_ENABLED(CONFIG_BLUETOOTH_MESH_PB_GATT)) {
		bt_mesh_proxy_prov_enable();
	}

	return 0;
}
