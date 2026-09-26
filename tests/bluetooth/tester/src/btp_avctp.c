/* btp_avctp.c - Bluetooth AVCTP Tester */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_avctp
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t avctp_read_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	struct btp_avctp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_AVCTP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler avctp_handlers[] = {
	{.opcode = BTP_AVCTP_READ_SUPPORTED_COMMANDS,
	 .index = BTP_INDEX_NONE,
	 .expect_len = 0,
	 .func = avctp_read_supported_commands},
};

uint8_t tester_init_avctp(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_AVCTP, avctp_handlers,
					 ARRAY_SIZE(avctp_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_avctp(void)
{
	return BTP_STATUS_SUCCESS;
}
