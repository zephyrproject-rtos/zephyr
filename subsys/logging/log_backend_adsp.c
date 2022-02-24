/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <logging/log_backend_std.h>

void intel_adsp_trace_out(int8_t *str, size_t len);

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	intel_adsp_trace_out(data, length);

	return length;
}

/* Trace output goes in 64 byte chunks with a 4-byte header, no point
 * in buffering more than 60 bytes at a time
 */
static uint8_t log_buf[60];

LOG_OUTPUT_DEFINE(log_output_adsp, char_out, log_buf, 1);

static uint32_t format_flags(void)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}
	return flags;
}

static inline void put(const struct log_backend *const backend,
		       struct log_msg *msg)
{
	log_backend_std_put(&log_output_adsp, format_flags(), msg);
}
static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output_adsp);
}

static inline void dropped(const struct log_backend *const backend,
			   uint32_t cnt)
{
	log_output_dropped_process(&log_output_adsp, cnt);
}

static inline void put_sync_string(const struct log_backend *const backend,
				   struct log_msg_ids src_level,
				   uint32_t timestamp, const char *fmt,
				   va_list ap)
{
	log_output_string(&log_output_adsp, src_level,
			  timestamp, fmt, ap, format_flags());
}

static inline void put_sync_hexdump(const struct log_backend *const backend,
				    struct log_msg_ids src_level,
				    uint32_t timestamp, const char *metadata,
				    const uint8_t *data, uint32_t length)
{
	log_output_hexdump(&log_output_adsp, src_level, timestamp,
			   metadata, data, length, format_flags());
}

static void process(const struct log_backend *const backend,
		union log_msg2_generic *msg)
{
	log_output_msg2_process(&log_output_adsp, &msg->log, format_flags());
}

const struct log_backend_api log_backend_adsp_api = {
	.process = IS_ENABLED(CONFIG_LOG2) ? process : NULL,
	.put_sync_string = IS_ENABLED(CONFIG_LOG1_IMMEDIATE) ?
		put_sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG1_IMMEDIATE) ?
		put_sync_hexdump : NULL,
	.put = IS_ENABLED(CONFIG_LOG1_DEFERRED) ? put : NULL,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.panic = panic,
};

LOG_BACKEND_DEFINE(log_backend_adsp, log_backend_adsp_api, true);
