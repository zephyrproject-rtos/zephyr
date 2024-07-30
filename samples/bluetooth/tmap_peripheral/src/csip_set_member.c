/** @file
 *  @brief Bluetooth Coordinated Set Identifier Profile (CSIP) Set Member role.
 *
 *  Copyright (c) 2022 Codecoup
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>

static struct bt_csip_set_member_svc_inst *svc_inst;

static void csip_lock_changed_cb(struct bt_conn *conn,
				 struct bt_csip_set_member_svc_inst *inst,
				 bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
}

static struct bt_csip_set_member_cb csip_cb = {
	.lock_changed = csip_lock_changed_cb,
};

int csip_set_member_init(void)
{
	struct bt_csip_set_member_register_param param = {
		.set_size = 2,
		.rank = CONFIG_TMAP_PERIPHERAL_SET_RANK,
		.lockable = false,
		.cb = &csip_cb,
	};

	return bt_cap_acceptor_register(&param, &svc_inst);
}

int csip_generate_rsi(uint8_t *rsi)
{
	int err;

	if (svc_inst == NULL) {
		return -ENODEV;
	}

	err = bt_csip_set_member_generate_rsi(svc_inst, rsi);
	if (err) {
		printk("Failed to generate RSI (err %d)\n", err);
		return err;
	}

	return 0;
}
