/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_output.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/uart.h>
#include <platform-zephyr.h>
#include "log_backend_std.h"

#ifndef CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE
#define CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE 0
#endif

static uint8_t char_buf[CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE];
static bool panic_mode;
static uint16_t last_log_level;

static int write(uint8_t *data, size_t length, void *ctx);

LOG_OUTPUT_DEFINE(log_output_spinel, write, char_buf, sizeof(char_buf));

static inline bool is_panic_mode(void)
{
	return panic_mode;
}

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	/* prevent adding CRLF, which may crash spinel decoding */
	uint32_t flag = LOG_OUTPUT_FLAG_CRLF_NONE;

	last_log_level = msg->hdr.ids.level;

	log_backend_std_put(&log_output_spinel, flag, msg);
}

static void sync_string(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *fmt, va_list ap)
{
	/* prevent adding CRLF, which may crash spinel decoding */
	uint32_t flag = LOG_OUTPUT_FLAG_CRLF_NONE;

	log_backend_std_sync_string(&log_output_spinel, flag, src_level,
				    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data,
			 uint32_t length)
{
	/* prevent adding CRLF, which may crash spinel decoding */
	uint32_t flag = LOG_OUTPUT_FLAG_CRLF_NONE;

	log_backend_std_sync_hexdump(&log_output_spinel, flag, src_level,
				     timestamp, metadata, data, length);
}

static void log_backend_spinel_init(void)
{
	memset(char_buf, '\0', sizeof(char_buf));
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);
	panic_mode = true;
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_spinel, cnt);
}

static int write(uint8_t *data, size_t length, void *ctx)
{
	otLogLevel log_level;

	if (is_panic_mode()) {
		/* In panic mode otPlatLog implemented for Spinel protocol
		 * cannot be used, because it cannot be called from interrupt.
		 * In such situation raw data bytes without encoding are send.
		 */
		platformUartPanic();
		otPlatUartSend(data, length);
	} else {
		switch (last_log_level) {
		case LOG_LEVEL_ERR:
			log_level = OT_LOG_LEVEL_CRIT;
			break;
		case LOG_LEVEL_WRN:
			log_level = OT_LOG_LEVEL_WARN;
			break;
		case LOG_LEVEL_INF:
			log_level = OT_LOG_LEVEL_INFO;
			break;
		case LOG_LEVEL_DBG:
			log_level = OT_LOG_LEVEL_DEBG;
			break;
		default:
			log_level = OT_LOG_LEVEL_NONE;
			break;
		}
		otPlatLog(log_level, OT_LOG_REGION_PLATFORM, "%s", data);
	}

	/* make sure that buffer will be clean in next attempt */
	memset(char_buf, '\0', length);
	return length;
}

const struct log_backend_api log_backend_spinel_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_spinel_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_spinel, log_backend_spinel_api, true);
