/* btp_a2dp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "btp/btp.h"

static uint8_t supported_commands(const void *cmd, uint16_t cmd_len, void *rsp, uint16_t *rsp_len)
{
	struct btp_avdtp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_A2DP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_AVDTP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
};

uint8_t tester_init_avdtp(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_AVDTP, handlers, ARRAY_SIZE(handlers));
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_avdtp(void)
{
	return BTP_STATUS_SUCCESS;
}
