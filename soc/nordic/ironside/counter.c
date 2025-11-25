/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_ironside/counter.h>
#include <nrf_ironside/call.h>

int ironside_counter_set(enum ironside_counter counter_id, uint32_t value)
{
	int err;
	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_COUNTER_SET_V1;
	buf->args[IRONSIDE_COUNTER_SET_SERVICE_COUNTER_ID_IDX] = (uint32_t)counter_id;
	buf->args[IRONSIDE_COUNTER_SET_SERVICE_VALUE_IDX] = value;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_COUNTER_SET_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}

int ironside_counter_get(enum ironside_counter counter_id, uint32_t *value)
{
	int err;
	struct ironside_call_buf *buf;

	if (value == NULL) {
		return -IRONSIDE_COUNTER_ERROR_INVALID_PARAM;
	}

	buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_COUNTER_GET_V1;
	buf->args[IRONSIDE_COUNTER_GET_SERVICE_COUNTER_ID_IDX] = (uint32_t)counter_id;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_COUNTER_GET_SERVICE_RETCODE_IDX];
		if (err == 0) {
			*value = buf->args[IRONSIDE_COUNTER_GET_SERVICE_VALUE_IDX];
		}
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}

int ironside_counter_lock(enum ironside_counter counter_id)
{
	int err;
	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_COUNTER_LOCK_V1;
	buf->args[IRONSIDE_COUNTER_LOCK_SERVICE_COUNTER_ID_IDX] = (uint32_t)counter_id;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_COUNTER_LOCK_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}
