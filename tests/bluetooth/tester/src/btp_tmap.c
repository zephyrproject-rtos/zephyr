/* btp_tmap.c - Bluetooth TMAP Tester */

/*
 * Copyright (c) 2024 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/tmap.h>
#include "btp/btp.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bttester_tmap, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t read_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				       uint16_t *rsp_len)
{
	struct btp_tmap_read_supported_commands_rp *rp = rsp;

	tester_set_bit(rp->data, BTP_TMAP_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_TMAP_DISCOVER);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static void tmap_discover_cb(enum bt_tmap_role role, struct bt_conn *conn, int err)
{
	struct btp_tmap_discovery_complete_ev ev;

	if (err) {
		LOG_ERR("Discovery failed (%d)", err);
	}

	bt_addr_le_copy(&ev.address, bt_conn_get_dst(conn));
	ev.status = err;
	ev.role = role;

	tester_event(BTP_SERVICE_ID_TMAP, BT_TMAP_EV_DISCOVERY_COMPLETE, &ev, sizeof(ev));
}

static const struct bt_tmap_cb tmap_cb = {
	.discovery_complete = tmap_discover_cb,
};

static uint8_t tmap_discover(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	const struct btp_tmap_discover_cmd *cp = cmd;
	struct bt_conn *conn;
	int err;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &cp->address);
	if (!conn) {
		LOG_ERR("Unknown connection");
		return BTP_STATUS_FAILED;
	}

	err = bt_tmap_discover(conn, &tmap_cb);
	if (err != 0) {
		LOG_ERR("Failed to discover remote TMAP: %d", err);
	}

	bt_conn_unref(conn);

	return BTP_STATUS_VAL(err);
}

static const struct btp_handler tmap_handlers[] = {
	{
		.opcode = BTP_TMAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = read_supported_commands,
	},
	{
		.opcode = BTP_TMAP_DISCOVER,
		.expect_len = sizeof(struct btp_tmap_discover_cmd),
		.func = tmap_discover,
	},
};

uint8_t tester_init_tmap(void)
{
	const enum bt_tmap_role role = (BT_TMAP_CG_SUPPORTED ? BT_TMAP_ROLE_CG : 0U) |
				       (BT_TMAP_CT_SUPPORTED ? BT_TMAP_ROLE_CT : 0U) |
				       (BT_TMAP_UMS_SUPPORTED ? BT_TMAP_ROLE_UMS : 0U) |
				       (BT_TMAP_UMR_SUPPORTED ? BT_TMAP_ROLE_UMR : 0U) |
				       (BT_TMAP_BMS_SUPPORTED ? BT_TMAP_ROLE_BMS : 0U) |
				       (BT_TMAP_BMR_SUPPORTED ? BT_TMAP_ROLE_BMR : 0U);
	int err;

	err = bt_tmap_register(role);

	if (err != 0) {
		LOG_ERR("Failed to register TMAP (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_TMAP, tmap_handlers,
					 ARRAY_SIZE(tmap_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_tmap(void)
{
	return BTP_STATUS_SUCCESS;
}
