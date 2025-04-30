/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/firmware/nrf_ironside/update.h>
#include <zephyr/drivers/firmware/nrf_ironside/call.h>

int ironside_update(const struct ironside_update_blob *update)
{
	int err;
	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_UPDATE_SERVICE_V0;
	buf->args[IRONSIDE_UPDATE_SERVICE_UPDATE_PTR_IDX] = (uintptr_t)update;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_UPDATE_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}
