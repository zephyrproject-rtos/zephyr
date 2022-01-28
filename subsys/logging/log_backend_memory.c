/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <logging/log_output_dict.h>
#include <logging/log_backend_std.h>
#include <logging/log.h>
#include <device.h>
#include <sys/__assert.h>
#include <sys/winstream.h>
#include <drivers/flash.h>

static volatile bool in_panic;

// TODO: Do I really need all these...? Can't I just trust them not to kill themselves?
BUILD_ASSERT(
	CONFIG_LOG_BACKEND_MEMORY_BUFFER_SIZE > CONFIG_LOG_BACKEND_MEMORY_BLOCK_BUFFER_SIZE,
	    "Block buffer size is larger than memory buffer size.");

BUILD_ASSERT(CONFIG_LOG_BACKEND_MEMORY_BLOCK_BUFFER_SIZE > 0,
	    "Block buffer size must be larger than 0.");

/* Prevent winstream from dropping bytes if using preserve-newest strategy */
BUILD_ASSERT(
	IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_PRESERVE_OLDEST) ||
	(CONFIG_LOG_BACKEND_MEMORY_BUFFER_SIZE % CONFIG_LOG_BACKEND_MEMORY_BLOCK_BUFFER_SIZE) == 0,
	    "Memory buffer size is not a multiple of block buffer size.");

/* Need space for winstream struct if using preserve-newest strategy */
BUILD_ASSERT(
	 IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_PRESERVE_OLDEST) ||
	 CONFIG_LOG_BACKEND_MEMORY_BUFFER_SIZE >= sizeof(struct sys_winstream),
	     "Memory buffer size is not large enough to hold winstream struct.");

BUILD_ASSERT(
	 IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_PRESERVE_NEWEST) ||
	 CONFIG_LOG_BACKEND_MEMORY_BUFFER_SIZE > 0,
	     "Memory buffer size must be larger than 0.");

static uint32_t block_buf_size = CONFIG_LOG_BACKEND_MEMORY_BLOCK_BUFFER_SIZE;
static uint32_t block_buf_idx = 0;
static uint8_t block_buf[CONFIG_LOG_BACKEND_MEMORY_BLOCK_BUFFER_SIZE];

#ifdef CONFIG_LOG_BACKEND_MEMORY_PRESERVE_OLDEST
static uint32_t memory_buf_len = CONFIG_LOG_BACKEND_MEMORY_BUFFER_SIZE;
static uint32_t memory_buf_idx = 0;
#else
static struct sys_winstream block_buffer;
#endif

// TODO: REMOVE, temporary for testing
static uint8_t test_memory_buf[CONFIG_LOG_BACKEND_MEMORY_BUFFER_SIZE]; // ...?

void test_memory_write(void) {
	for (int i = 0; i < block_buf_size; i++, memory_buf_idx++) {
		test_memory_buf[memory_buf_idx] = block_buf[i];
	}
}
// END TODO

static void dict_char_out_hex(uint8_t *data, size_t length)
{
	// TODO
}

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
	int err;

	/* Copy as many characters in data into block buffer as can fit */
	int data_idx;
	for (data_idx = 0; data_idx < length && block_buf_idx < block_buf_size; 
			data_idx++, block_buf_idx++) {
		block_buf[block_buf_idx] = data[data_idx];
	}

	/* Block buffer is full */
	if (block_buf_idx == block_buf_size) {
		/* Write block out to memory */
		// Whatever method is used, memory_buf_idx must be incremented here
		test_memory_write(); // TODO: Remove

#ifdef CONFIG_LOG_BACKEND_MEMORY_PRESERVE_OLDEST
		// TODO: Write to buffer directly
#else
		// TODO: Write to winstream
#endif

		/* Copy any leftover characters in data into start of block buffer */
		// As long as the mem_output_buf is the same size or smaller as block buffer,
		// leftover chars shouldn't overflow the new block 
		for (block_buf_idx = 0; data_idx < length && block_buf_idx < block_buf_size; 
				data_idx++, block_buf_idx++) {
			block_buf[block_buf_idx] = data[data_idx];
		}
	}

	return length;
}

// Can't use as block buffer itself since it can contain old and new data
// Issues with using a buffer larger than 1, e.g. same size as block buffer:
// - (CRITICAL) If Zephyr crashes before the logging subsystem fills this buffer party or fully
//   and calls process again, leftover characters from the last char_out call could remain in 
//   the buffer and be lost.
//   (Note that this is different from the characters that would normally be lost if the
//   buffer hadn't been emptied before Zephyr crashed)
// - Takes up a lot of space in addition to block buffer
// Must be smaller or equal in size to block buffer
static uint8_t memory_output_buf[CONFIG_LOG_BACKEND_MEMORY_BLOCK_BUFFER_SIZE];
LOG_OUTPUT_DEFINE(log_output_memory, char_out, memory_output_buf, sizeof(memory_output_buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_put(&log_output_memory, flag, msg);
}

static void process(const struct log_backend *const backend,
		union log_msg2_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	flags |= IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ? LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_OUTPUT_DICTIONARY)) {
		log_dict_output_msg2_process(&log_output_memory,
					     &msg->log, flags);
	} else {
		log_output_msg2_process(&log_output_memory, &msg->log, flags);
	}
}

static void log_backend_memory_init(struct log_backend const *const backend)
{
	// TODO: Reserve space for memory buffer on flash or disk

	// TODO: Test to verify can write / readback memory and if not... should I LOG / ASSERT...?

	#ifdef CONFIG_LOG_BACKEND_MEMORY_PRESERVE_NEWEST
	// TODO: Initialize winstream struct
	#endif

	// TODO: Immediately deactivate if log flush on crash? 
	// (beware if get activated after this...)
}

static void panic(struct log_backend const *const backend)
{
	in_panic = true;
	log_backend_std_panic(&log_output_memory);

	// TODO: If in log flush on crash log_backend_activate?
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_OUTPUT_DICTIONARY)) {
		log_dict_output_dropped_process(&log_output_memory, cnt);
	} else {
		log_backend_std_dropped(&log_output_memory, cnt);
	}
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, uint32_t timestamp,
		     const char *fmt, va_list ap)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_string(&log_output_memory, flag, src_level,
				    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_MEMORY_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_hexdump(&log_output_memory, flag, src_level,
				     timestamp, metadata, data, length);
}

const struct log_backend_api log_backend_memory_api = {
	.process = IS_ENABLED(CONFIG_LOG2) ? process : NULL,
	.put = IS_ENABLED(CONFIG_LOG1_DEFERRED) ? put : NULL,
	.put_sync_string = IS_ENABLED(CONFIG_LOG1_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG1_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_memory_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_memory, log_backend_memory_api, true);
