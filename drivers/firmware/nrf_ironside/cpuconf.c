/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <nrfx.h>

#include <zephyr/drivers/firmware/nrf_ironside/call.h>
#include <zephyr/drivers/firmware/nrf_ironside/cpuconf.h>

int ironside_cpuconf(NRF_PROCESSORID_Type cpu, void *vector_table, bool cpu_wait, uint8_t *msg,
		     size_t msg_size)
{
	int err;
	struct ironside_call_buf *const buf = ironside_call_alloc();

	buf->id = IRONSIDE_CALL_ID_CPUCONF_V0;

	buf->args[IRONSIDE_CPUCONF_SERVICE_CPU_IDX] = cpu;
	buf->args[IRONSIDE_CPUCONF_SERVICE_VECTOR_TABLE_IDX] = (uint32_t)vector_table;
	buf->args[IRONSIDE_CPUCONF_SERVICE_CPU_WAIT_IDX] = cpu_wait;
	buf->args[IRONSIDE_CPUCONF_SERVICE_MSG_IDX] = (uint32_t)msg;
	buf->args[IRONSIDE_CPUCONF_SERVICE_MSG_SIZE_IDX] = msg_size;

	ironside_call_dispatch(buf);

	if (buf->status == IRONSIDE_CALL_STATUS_RSP_SUCCESS) {
		err = buf->args[IRONSIDE_CPUCONF_SERVICE_RETCODE_IDX];
	} else {
		err = buf->status;
	}

	ironside_call_release(buf);

	return err;
}
