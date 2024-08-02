/* btp_cas.c - Bluetooth CAS Tester */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/cap.h>

#include "btp/btp.h"
#include "zephyr/sys/byteorder.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_cas
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static struct bt_csip_set_member_svc_inst *csis_svc_inst;

static uint8_t cas_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_cas_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_CAS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_CAS_SET_MEMBER_LOCK);
	tester_set_bit(rp->data, BTP_CAS_GET_MEMBER_RSI);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t cas_set_member_lock(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_cas_set_member_lock_cmd *cp = cmd;
	int err = -EINVAL;

	if (csis_svc_inst) {
		err = bt_csip_set_member_lock(csis_svc_inst, cp->lock, cp->force);
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t cas_get_member_rsi(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_cas_get_member_rsi_rp *rp = rsp;
	int err = -EINVAL;

	if (csis_svc_inst) {
		err = bt_csip_set_member_generate_rsi(csis_svc_inst, rp->rsi);
	}
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_VAL(err);
}

static const struct btp_handler cas_handlers[] = {
	{
		.opcode = BTP_CAS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = cas_supported_commands
	},
	{
		.opcode = BTP_CAS_SET_MEMBER_LOCK,
		.expect_len = sizeof(struct btp_cas_set_member_lock_cmd),
		.func = cas_set_member_lock
	},
	{
		.opcode = BTP_CAS_GET_MEMBER_RSI,
		.expect_len = sizeof(struct btp_cas_get_member_rsi_cmd),
		.func = cas_get_member_rsi
	}
};

uint8_t tester_init_cas(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_CAS, cas_handlers,
					 ARRAY_SIZE(cas_handlers));

#if defined(CONFIG_BT_CAP_ACCEPTOR) && defined(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)
	struct bt_csip_set_member_register_param register_params = {
		.set_size = 2,
		.sirk = { 0xB8, 0x03, 0xEA, 0xC6, 0xAF, 0xBB, 0x65, 0xA2,
			  0x5A, 0x41, 0xF1, 0x53, 0x05, 0x68, 0x8E, 0x83 },
		.lockable = true,
		.rank = 1,
		.cb = NULL,
	};
	int err = bt_cap_acceptor_register(&register_params, &csis_svc_inst);
#else
	int err = 0;
#endif /* CONFIG_BT_CAP_ACCEPTOR && CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER */

	return BTP_STATUS_VAL(err);
}

uint8_t tester_unregister_cas(void)
{
	return BTP_STATUS_SUCCESS;
}
