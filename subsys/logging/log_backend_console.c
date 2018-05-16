/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <logging/log_backend_console.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <device.h>
#include <uart.h>

int char_out(u8_t *p_data, size_t length, void *p_ctx)
{
	struct device *p_dev = (struct device *)p_ctx;

	for (size_t i = 0; i < length; i++) {
		uart_poll_out(p_dev, p_data[i]);
	}
	return length;
}

static u8_t buf;

static struct log_output_ctx ctx = {
	.func = char_out,
	.p_data = &buf,
	.length = 1,
	.offset = 0
};

static void put(struct log_backend const *const p_backend,
		struct log_msg *p_msg)
{
	log_msg_get(p_msg);

	u32_t flags = 0;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_CONSOLE_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_CONSOLE_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(p_msg, &ctx, flags);

	log_msg_put(p_msg);

}

void log_backend_console_init(void)
{
	ctx.p_ctx = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
}

static void panic(struct log_backend const *const p_backend)
{

}

const struct log_backend_api log_backend_console_api = {
	.put = put,
	.panic = panic,
};
