/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log_output.h>
#include <openthread/platform/logging.h>
#include <utils/uart.h>
#include <platform-zephyr.h>

#ifndef CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE
#define CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE 0
#endif

static uint8_t char_buf[CONFIG_LOG_BACKEND_SPINEL_BUFFER_SIZE];
static bool panic_mode;
static uint16_t last_log_level;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_SPINEL_OUTPUT_DEFAULT;

static int write(uint8_t *data, size_t length, void *ctx);

LOG_OUTPUT_DEFINE(log_output_spinel, write, char_buf, sizeof(char_buf));

static inline bool is_panic_mode(void)
{
	return panic_mode;
}

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	/* prevent adding CRLF, which may crash spinel decoding */
	uint32_t flags = LOG_OUTPUT_FLAG_CRLF_NONE | log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_spinel, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void log_backend_spinel_init(struct log_backend const *const backend)
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
	.process = process,
	.panic = panic,
	.init = log_backend_spinel_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_spinel, log_backend_spinel_api, true);
