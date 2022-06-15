/*
 * Copyright (c) 2019 Intel Corporation Inc.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include <xtensa/simcall.h>

#define CHAR_BUF_SIZE (IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? \
		1 : CONFIG_LOG_BACKEND_XTENSA_OUTPUT_BUFFER_SIZE)

static uint8_t xtensa_log_buf[CHAR_BUF_SIZE];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_XTENSA_SIM_OUTPUT_DEFAULT;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	register int a2 __asm__ ("a2") = SYS_write;
	register int a3 __asm__ ("a3") = 1;
	register int a4 __asm__ ("a4") = (int) data;
	register int a5 __asm__ ("a5") = length;

	__asm__ volatile("simcall"
			 : "=a"(a2), "=a"(a3)
			 : "a"(a2), "a"(a3), "a"(a4), "a"(a5));

	return length;
}

LOG_OUTPUT_DEFINE(log_output_xsim, char_out,
		  xtensa_log_buf, sizeof(xtensa_log_buf));

static void process(const struct log_backend *const backend,
		    union log_msg2_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_xsim, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output_xsim);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_xsim, cnt);
}

const struct log_backend_api log_backend_xtensa_sim_api = {
	.process = process,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_xtensa_sim, log_backend_xtensa_sim_api, true);
