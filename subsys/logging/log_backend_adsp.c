/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <device.h>
#include <assert.h>

#define BUF_SIZE 256

#define MAILBOX_TRACE_BASE	0xbe008000
#define MAILBOX_TRACE_SIZE	0x2000

static inline void dcache_writeback_region(void *addr, size_t size)
{
#if XCHAL_DCACHE_SIZE > 0
	xthal_dcache_region_writeback(addr, size);
#endif
}

static void trace(const u8_t *data, size_t length)
{
	volatile u8_t *t = MAILBOX_TRACE_BASE;
	int i;

	for (i = 0; i < length; i++) {
		t[i] = data[i];
	}

	dcache_writeback_region(t, i);
}

static int char_out(u8_t *data, size_t length, void *ctx)
{
	trace(data, length);

	return length;
}

static u8_t buf[BUF_SIZE];

LOG_OUTPUT_DEFINE(log_output, char_out, buf, sizeof(buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	log_msg_get(msg);

	u32_t flags = LOG_OUTPUT_FLAG_LEVEL;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(&log_output, msg, flags);

	log_msg_put(msg);
}

static void panic(struct log_backend const *const backend)
{
	log_output_flush(&log_output);
}

static void dropped(const struct log_backend *const backend, u32_t cnt)
{
	ARG_UNUSED(backend);

	log_output_dropped_process(&log_output, cnt);
}

static void sync_string(const struct log_backend *const backend,
			struct log_msg_ids src_level, u32_t timestamp,
			const char *fmt, va_list ap)
{
	u32_t flags = LOG_OUTPUT_FLAG_LEVEL;
	u32_t key;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	log_output_string(&log_output, src_level, timestamp, fmt, ap, flags);
	irq_unlock(key);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, u32_t timestamp,
			 const char *metadata, const u8_t *data, u32_t length)
{
	u32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
	u32_t key;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	log_output_hexdump(&log_output, src_level, timestamp,
			   metadata, data, length, flags);
	irq_unlock(key);
}

const struct log_backend_api log_backend_adsp_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_adsp, log_backend_adsp_api, true);
