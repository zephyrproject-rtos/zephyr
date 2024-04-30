/* btp_csis.c - Bluetooth CSIS Tester */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/csip.h>

#include "btp/btp.h"
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_csis
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define BTP_STATUS_VAL(err) (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS

static struct bt_csip_set_member_svc_inst *csis_svc_inst;
static bool enc_sirk;

static uint8_t csis_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_csis_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_CSIS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_CSIS_SET_MEMBER_LOCK);
	tester_set_bit(rp->data, BTP_CSIS_GET_MEMBER_RSI);
	tester_set_bit(rp->data, BTP_CSIS_ENC_SIRK_TYPE);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t csis_set_member_lock(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_csis_set_member_lock_cmd *cp = cmd;
	int err = -1;

	if (csis_svc_inst) {
		err = bt_csip_set_member_lock(csis_svc_inst, cp->lock, cp->force);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t csis_get_member_rsi(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	struct btp_csis_get_member_rsi_rp *rp = rsp;
	int err = -1;

	if (csis_svc_inst) {
		err = bt_csip_set_member_generate_rsi(csis_svc_inst, rp->rsi);
	}

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_VAL(err);
}

static uint8_t csis_set_sirk_type(const void *cmd, uint16_t cmd_len, void *rsp,
				  uint16_t *rsp_len)
{
	const struct btp_csis_set_sirk_type_cmd *cp = cmd;

	enc_sirk = cp->encrypted != 0U;

	LOG_DBG("Set SIRK type: %s", enc_sirk ? "encrypted" : "plain text");

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler csis_handlers[] = {
	{
		.opcode = BTP_CSIS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = csis_supported_commands
	},
	{
		.opcode = BTP_CSIS_SET_MEMBER_LOCK,
		.expect_len = sizeof(struct btp_csis_set_member_lock_cmd),
		.func = csis_set_member_lock
	},
	{
		.opcode = BTP_CSIS_GET_MEMBER_RSI,
		.expect_len = sizeof(struct btp_csis_get_member_rsi_cmd),
		.func = csis_get_member_rsi
	},
	{
		.opcode = BTP_CSIS_ENC_SIRK_TYPE,
		.expect_len = sizeof(struct btp_csis_set_sirk_type_cmd),
		.func = csis_set_sirk_type,
	},
};

static void lock_changed_cb(struct bt_conn *conn, struct bt_csip_set_member_svc_inst *svc_inst,
			    bool locked)
{
	LOG_DBG("%s", locked ? "locked" : "unlocked");
}

static uint8_t sirk_read_cb(struct bt_conn *conn, struct bt_csip_set_member_svc_inst *svc_inst)
{
	if (enc_sirk) {
		return BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT_ENC;
	} else {
		return BT_CSIP_READ_SIRK_REQ_RSP_ACCEPT;
	}
}

static struct bt_csip_set_member_cb csis_cb = {
	.lock_changed = lock_changed_cb,
	.sirk_read_req = sirk_read_cb,
};

uint8_t tester_init_csis(void)
{
	const struct bt_csip_set_member_register_param register_params = {
		.set_size = 1,
		.set_sirk = { 0xB8, 0x03, 0xEA, 0xC6, 0xAF, 0xBB, 0x65, 0xA2,
			      0x5A, 0x41, 0xF1, 0x53, 0x05, 0x68, 0x8E, 0x83 },
		.lockable = true,
		.rank = 1,
		.cb = &csis_cb,
	};
	int err = bt_csip_set_member_register(&register_params, &csis_svc_inst);

	tester_register_command_handlers(BTP_SERVICE_ID_CSIS, csis_handlers,
					 ARRAY_SIZE(csis_handlers));

	return BTP_STATUS_VAL(err);
}

uint8_t tester_unregister_csis(void)
{
	return BTP_STATUS_SUCCESS;
}
