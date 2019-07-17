/*
 * Copyright (c) 2019 Intel Corporation Inc.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
#include <xtensa/simcall.h>

#define CHAR_BUF_SIZE CONFIG_LOG_BACKEND_XTENSA_OUTPUT_BUFFER_SIZE

static u8_t buf[CHAR_BUF_SIZE];

static int char_out(u8_t *data, size_t length, void *ctx)
{
	register int a1 __asm__ ("a2") = SYS_write;
	register int b1 __asm__ ("a3") = 1;
	register int c1 __asm__ ("a4") = (int) data;
	register int d1 __asm__ ("a5") = length;
	register int ret_val __asm__ ("a2");
	register int ret_err __asm__ ("a3");

	__asm__ __volatile__ (
			"simcall\n"
			"mov %0, a2\n"
			"mov %1, a3\n"
			: "=a" (ret_val), "=a" (ret_err), "+r"(a1), "+r"(b1)
			: "r"(c1), "r"(d1)
			: "memory");
	return length;
}

LOG_OUTPUT_DEFINE(log_output, char_out, buf, sizeof(buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	log_backend_std_put(&log_output, 0, msg);

}

static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output);
}

static void dropped(const struct log_backend *const backend, u32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output, cnt);
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, u32_t timestamp,
		     const char *fmt, va_list ap)
{
	log_backend_std_sync_string(&log_output, 0, src_level,
				    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, u32_t timestamp,
			 const char *metadata, const u8_t *data, u32_t length)
{
	log_backend_std_sync_hexdump(&log_output, 0, src_level,
				     timestamp, metadata, data, length);
}

const struct log_backend_api log_backend_xtensa_sim_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_xtensa_sim, log_backend_xtensa_sim_api, true);
