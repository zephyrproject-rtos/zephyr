/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
#include <device.h>
#include <drivers/uart.h>
#include <assert.h>

static const struct device *uart_dev;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);

	for (size_t i = 0; i < length; i++) {
		uart_poll_out(uart_dev, data[i]);
	}

	return length;
}

static uint8_t uart_output_buf;

LOG_OUTPUT_DEFINE(log_output_uart, char_out, &uart_output_buf, 1);

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_put(&log_output_uart, flag, msg);
}

static void log_backend_uart_init(void)
{
	uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	assert((void *)uart_dev);
}

static void panic(struct log_backend const *const backend)
{
	log_backend_std_panic(&log_output_uart);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_uart, cnt);
}

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, uint32_t timestamp,
		     const char *fmt, va_list ap)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_string(&log_output_uart, flag, src_level,
				    timestamp, fmt, ap);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_hexdump(&log_output_uart, flag, src_level,
				     timestamp, metadata, data, length);
}

const struct log_backend_api log_backend_uart_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_uart_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_uart, log_backend_uart_api, true);
