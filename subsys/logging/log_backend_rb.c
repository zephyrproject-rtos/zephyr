/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <sys/ring_buffer.h>

static struct ring_buf ringbuf;

/*
 * Log message format:
 * Logging started with magic number 0x55aa followed by log message id.
 * Log message ended with null terminator and takes
 * CONFIG_LOG_BACKEND_RB_SLOT_SIZE slot. The long log message can occupy
 * several logging slots.
 */

/*
 * All log messages are split to similar sized logging slots. Since ring
 * buffer slots get rewritten we need to check that all slots are fit to
 * the ring buffer
 */
BUILD_ASSERT(CONFIG_LOG_BACKEND_RB_MEM_SIZE %
	     CONFIG_LOG_BACKEND_RB_SLOT_SIZE == 0);

static void init(void)
{
	ring_buf_init(&ringbuf, CONFIG_LOG_BACKEND_RB_MEM_SIZE,
		      (void *)CONFIG_LOG_BACKEND_RB_MEM_BASE);
}

static void trace(const uint8_t *data, size_t length)
{
	const uint16_t magic = 0x55aa;
	static uint16_t log_id;
	volatile uint8_t *t, *region;
	int space;
	int i;

	space = ring_buf_space_get(&ringbuf);
	if (space < CONFIG_LOG_BACKEND_RB_SLOT_SIZE) {
		uint8_t *dummy;

		/* Remove oldest entry */
		ring_buf_get_claim(&ringbuf, &dummy,
				   CONFIG_LOG_BACKEND_RB_SLOT_SIZE);
		ring_buf_get_finish(&ringbuf, CONFIG_LOG_BACKEND_RB_SLOT_SIZE);
	}

	ring_buf_put_claim(&ringbuf, (uint8_t **)&t,
			   CONFIG_LOG_BACKEND_RB_SLOT_SIZE);
	region = t;

	/* Add magic number at the beginning of the slot */
	*(uint16_t *)t = magic;
	t += 2;

	/* Add log id */
	*(uint16_t *)t = log_id++;
	t += 2;

	for (i = 0; i < MIN(length, CONFIG_LOG_BACKEND_RB_SLOT_SIZE - 4); i++) {
		*t++ = data[i];
	}

	SOC_DCACHE_FLUSH((void *)region, CONFIG_LOG_BACKEND_RB_SLOT_SIZE);

	ring_buf_put_finish(&ringbuf, CONFIG_LOG_BACKEND_RB_SLOT_SIZE);
}

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	trace(data, length);

	return length;
}

/* magic and log id takes space */
static uint8_t rb_log_buf[CONFIG_LOG_BACKEND_RB_SLOT_SIZE - 4];

LOG_OUTPUT_DEFINE(log_output_rb, char_out, rb_log_buf, sizeof(rb_log_buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	log_msg_get(msg);

	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(&log_output_rb, msg, flags);

	log_msg_put(msg);
}

static void panic(struct log_backend const *const backend)
{
	log_output_flush(&log_output_rb);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_output_dropped_process(&log_output_rb, cnt);
}

static void sync_string(const struct log_backend *const backend,
			struct log_msg_ids src_level, uint32_t timestamp,
			const char *fmt, va_list ap)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL;
	uint32_t key;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	log_output_string(&log_output_rb, src_level,
			  timestamp, fmt, ap, flags);
	irq_unlock(key);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;
	uint32_t key;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	key = irq_lock();
	log_output_hexdump(&log_output_rb, src_level, timestamp,
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
	.init = init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_adsp, log_backend_adsp_api, true);
