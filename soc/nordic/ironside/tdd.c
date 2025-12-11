/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_ironside/tdd.h>
#include <nrf_ironside/call.h>

int ironside_se_tdd_configure(const enum ironside_se_tdd_config config)
{
	int err;
	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_SE_CALL_ID_TDD_V0;
	buf->args[IRONSIDE_SE_TDD_SERVICE_REQ_CONFIG_IDX] = (uint32_t)config;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_SE_TDD_SERVICE_RSP_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}
