/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <logging/log_backend.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
#include <../core/serial.h>

static int char_out(u8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
	for (int i = 0; i < length; ++i) {
		serout(data[i]);
	}
	return length;
}

static char buf;
LOG_OUTPUT_DEFINE(log_output, char_out, &buf, sizeof(buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	ARG_UNUSED(backend);
	log_backend_std_put(&log_output, 0, msg);
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);
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
	ARG_UNUSED(backend);
	log_backend_std_sync_string(&log_output, 0, src_level,
			    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, u32_t timestamp,
			 const char *metadata, const u8_t *data, u32_t length)
{
	ARG_UNUSED(backend);
	log_backend_std_sync_hexdump(&log_output, 0, src_level,
				     timestamp, metadata, data, length);
}

const struct log_backend_api log_backend_qemu_x86_64_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_qemu_x86_64, log_backend_qemu_x86_64_api, true);
