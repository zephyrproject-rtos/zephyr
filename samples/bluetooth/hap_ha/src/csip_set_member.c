/** @file
 *  @brief Bluetooth Coordinated Set Identifier Profile (CSIP) Set Member role.
 *
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csis.h>

#define CSIS_SIRK_DEBUG	{ 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce, \
			  0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 }

static struct bt_csis *csis;

static void csis_lock_changed_cb(struct bt_conn *conn, struct bt_csis *csis, bool locked)
{
	printk("Client %p %s the lock\n", conn, locked ? "locked" : "released");
}

static uint8_t sirk_read_req_cb(struct bt_conn *conn, struct bt_csis *csis)
{
	return BT_CSIS_READ_SIRK_REQ_RSP_ACCEPT;
}

static struct bt_csis_cb csis_cb = {
	.lock_changed = csis_lock_changed_cb,
	.sirk_read_req = sirk_read_req_cb,
};

int csip_set_member_init(void)
{
	struct bt_csis_register_param param = {
		.set_size = 2,
		.rank = CONFIG_HAP_HA_SET_RANK,
		.lockable = false,
		.set_sirk = CSIS_SIRK_DEBUG,
		.cb = &csis_cb,
	};

	return bt_cap_acceptor_register(&param, &csis);
}

int csip_generate_rsi(uint8_t *rsi)
{
	int err;

	if (csis == NULL) {
		return -ENODEV;
	}

	err = bt_csis_generate_rsi(csis, rsi);
	if (err) {
		printk("Failed to generate RSI (err %d)\n", err);
		return err;
	}

	return 0;
}
