/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/sys/util.h>

#include <zephyr/debug/coredump.h>
#include "coredump_internal.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <adsp_memory.h>
#include <adsp_debug_window.h>
LOG_MODULE_REGISTER(coredump_error, CONFIG_KERNEL_LOG_LEVEL);

static int error;
static uint32_t mem_wptr;

static void coredump_mem_window_backend_start(void)
{
	/* Reset error & mem write ptr */
	error = 0;
	mem_wptr = 0;
	ADSP_DW->descs[1].type = ADSP_DW_SLOT_TELEMETRY;

	while (LOG_PROCESS()) {
		;
	}

	LOG_PANIC();
	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_BEGIN_STR);
}

static void coredump_mem_window_backend_end(void)
{
	if (error != 0) {
		LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_ERROR_STR);
	}

	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_END_STR);
}

static void coredump_mem_window_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	uint32_t *mem_window_separator = (uint32_t *)(ADSP_DW->slots[1]);
	uint8_t *mem_window_sink = (uint8_t *)(ADSP_DW->slots[1]) + 4 + mem_wptr;
	uint8_t *coredump_data = buf;
	size_t data_left;
	/* Default place for telemetry dump is in memory window. Each data is easily find using
	 * separator. For telemetry that separator is 0x0DEC0DEB.
	 */
	*mem_window_separator = 0x0DEC0DEB;

	/* skip the overflow data. Don't wrap around to keep the most important data
	 * such as registers and call stack in the beginning of mem window.
	 */
	if (mem_wptr >= ADSP_DW_SLOT_SIZE - 4)
		return;

	if (buf) {
		for (data_left = buflen; data_left > 0; data_left--) {
			*mem_window_sink = *coredump_data;
			mem_window_sink++;
			coredump_data++;
		}

		mem_wptr += buflen;
	} else {
		error = -EINVAL;
	}
}

static int coredump_mem_window_backend_query(enum coredump_query_id query_id,
					     void *arg)
{
	int ret;

	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		ret = error;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int coredump_mem_window_backend_cmd(enum coredump_cmd_id cmd_id,
					   void *arg)
{
	int ret;

	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		ret = 0;
		error = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

struct coredump_backend_api coredump_backend_intel_adsp_mem_window = {
	.start = coredump_mem_window_backend_start,
	.end = coredump_mem_window_backend_end,
	.buffer_output = coredump_mem_window_backend_buffer_output,
	.query = coredump_mem_window_backend_query,
	.cmd = coredump_mem_window_backend_cmd,
};
