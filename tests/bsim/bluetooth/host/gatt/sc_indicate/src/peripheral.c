/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_peripheral, LOG_LEVEL_DBG);

#include "bs_bt_utils.h"

#define UUID_1 BT_UUID_DECLARE_128(0xdb, 0x1f, 0xe2, 0x52, 0xf3, 0xc6, 0x43, 0x66,                 \
				   0xb3, 0x92, 0x5d, 0xc6, 0xe7, 0xc9, 0x59, 0x9d)
#define UUID_2 BT_UUID_DECLARE_128(0x3f, 0xa4, 0x7f, 0x44, 0x2e, 0x2a, 0x43, 0x05,                 \
				   0xab, 0x38, 0x07, 0x8d, 0x16, 0xbf, 0x99, 0xf1)

static void new_svc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_DBG("CCC Update: notification %s", notif_enabled ? "enabled" : "disabled");
}

static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(UUID_1),
	BT_GATT_CHARACTERISTIC(UUID_2, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(new_svc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service svc = {
	.attrs = attrs,
	.attr_count = ARRAY_SIZE(attrs),
};

void peripheral(void)
{
	/*
	 * test goal: check that service changed indication is sent on
	 * reconnection when the server's GATT database has been updated since
	 * last connection
	 *
	 * the peripheral will wait for connection/disconnection, when
	 * disconnected it will register a new service, when reconnecting, the
	 * central should receive an indication
	 */

	int err;
	struct bt_le_ext_adv *adv = NULL;

	err = bt_enable(NULL);
	BSIM_ASSERT(!err, "bt_enable failed (%d)\n", err);

	err = settings_load();
	BSIM_ASSERT(!err, "settings_load failed (%d)\n", err);

	create_adv(&adv);
	start_adv(adv);
	wait_connected();

	stop_adv(adv);

	wait_disconnected();
	clear_g_conn();

	/* add a new service to trigger the service changed indication */
	err = bt_gatt_service_register(&svc);
	BSIM_ASSERT(!err, "bt_gatt_service_register failed (%d)\n", err);
	LOG_DBG("New service added");

	start_adv(adv);
	wait_connected();

	PASS("Done\n");
}
