/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/util.h>
#include <nrf_ironside/bootmode.h>
#include <nrf_ironside/call.h>

#define BOOT_MODE_SECONDARY (0x1)

BUILD_ASSERT(IRONSIDE_BOOTMODE_SERVICE_NUM_ARGS <= NRF_IRONSIDE_CALL_NUM_ARGS);

int ironside_bootmode_secondary_reboot(const uint8_t *msg, size_t msg_size)
{
	int err;
	struct ironside_call_buf *buf;
	uint8_t *buf_msg;

	if (msg_size > IRONSIDE_BOOTMODE_SERVICE_MSG_MAX_SIZE) {
		return -IRONSIDE_BOOTMODE_ERROR_MESSAGE_TOO_LARGE;
	}

	buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_BOOTMODE_SERVICE_V1;

	buf->args[IRONSIDE_BOOTMODE_SERVICE_MODE_IDX] = BOOT_MODE_SECONDARY;

	buf_msg = (uint8_t *)&buf->args[IRONSIDE_BOOTMODE_SERVICE_MSG_0_IDX];

	memset(buf_msg, 0, IRONSIDE_BOOTMODE_SERVICE_MSG_MAX_SIZE);

	if (msg_size > 0) {
		memcpy(buf_msg, msg, msg_size);
	}

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_BOOTMODE_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}
