/** @file
 *  @brief Bluetooth Coordinated Set Identifier Profile (CSIP) Set Member role.
 *
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define CSIP_SIRK_DEBUG	{ 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce, \
			  0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 }

static struct bt_csip_set_member_svc_inst *svc_inst;

static void csip_lock_changed_cb(struct bt_conn *conn,
				 struct bt_csip_set_member_svc_inst *inst,
				 bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn,
				struct bt_csip_set_member_svc_inst *inst)
{
	return BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;
}

static struct bt_csip_set_member_cb csip_cb = {
	.lock_changed = csip_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

int csip_set_member_init(void)
{
	struct bt_csip_set_member_register_param param = {
		.set_size = 2,
		.rank = CONFIG_HAP_HA_SET_RANK,
		.lockable = false,
		.sirk = CSIP_SIRK_DEBUG,
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
