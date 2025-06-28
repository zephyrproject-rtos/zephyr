/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <nrf_ironside/call.h>
#include <nrf_ironside/cpuconf.h>

#define CPU_PARAMS_CPU_OFFSET (0)
#define CPU_PARAMS_CPU_MASK   (0xF)
#define CPU_PARAMS_WAIT_BIT   BIT(4)

int ironside_cpuconf(NRF_PROCESSORID_Type cpu, const void *vector_table, bool cpu_wait,
		     const uint8_t *msg, size_t msg_size)
{
	int err;
	struct ironside_call_buf *buf;
	uint8_t *buf_msg;

	if (msg_size > IRONSIDE_CPUCONF_SERVICE_MSG_MAX_SIZE) {
		return -IRONSIDE_CPUCONF_ERROR_MESSAGE_TOO_LARGE;
	}

	buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_CPUCONF_V0;

	buf->args[IRONSIDE_CPUCONF_SERVICE_CPU_PARAMS_IDX] =
		(((uint32_t)cpu << CPU_PARAMS_CPU_OFFSET) & CPU_PARAMS_CPU_MASK) |
		(cpu_wait ? CPU_PARAMS_WAIT_BIT : 0);

	buf->args[IRONSIDE_CPUCONF_SERVICE_VECTOR_TABLE_IDX] = (uint32_t)vector_table;

	buf_msg = (uint8_t *)&buf->args[IRONSIDE_CPUCONF_SERVICE_MSG_0];
	if (msg_size > 0) {
		memcpy(buf_msg, msg, msg_size);
	}
	if (msg_size < IRONSIDE_CPUCONF_SERVICE_MSG_MAX_SIZE) {
		memset(&buf_msg[msg_size], 0, IRONSIDE_CPUCONF_SERVICE_MSG_MAX_SIZE - msg_size);
	}

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_CPUCONF_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}
