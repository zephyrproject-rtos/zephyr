/* csip.c - CAP Commander specific CSIP mocks */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/conn.h>

static struct bt_csip_set_coordinator_cb *csip_cb;

struct bt_csip_set_coordinator_svc_inst {
	struct bt_conn *conn;
	struct bt_csip_set_coordinator_set_info *set_info;
} svc_inst;

static struct bt_csip_set_coordinator_set_member member = {
	.insts = {
		{
			.info =	{
				.set_size = 2,
				.rank = 1,
				.lockable = false,
			},
			.svc_inst = &svc_inst,
		},
	},
};

struct bt_csip_set_coordinator_csis_inst *
bt_csip_set_coordinator_csis_inst_by_handle(struct bt_conn *conn, uint16_t start_handle)
{
	return &member.insts[0];
}

int bt_csip_set_coordinator_register_cb(struct bt_csip_set_coordinator_cb *cb)
{
	csip_cb = cb;

	return 0;
}

int bt_csip_set_coordinator_discover(struct bt_conn *conn)
{
	if (csip_cb != NULL) {
		svc_inst.conn = conn;
		svc_inst.set_info = &member.insts[0].info;
		csip_cb->discover(conn, &member, 0, 1);
	}

	return 0;
}

struct bt_csip_set_coordinator_set_member *
bt_csip_set_coordinator_set_member_by_conn(const struct bt_conn *conn)
{
	if (conn == NULL) {
		return NULL;
	}

	return &member;
}

void mock_bt_csip_init(void)
{
}

void mock_bt_csip_cleanup(void)
{
	csip_cb = NULL;
}
