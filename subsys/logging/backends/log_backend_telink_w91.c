/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>

static uint8_t buf[1];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_TELINK_W91_OUTPUT_DEFAULT;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
	extern int arch_printk_char_out(int c);

	for (size_t i = 0; i < length; i++) {
		arch_printk_char_out(data[i]);
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_telink_w91, char_out, buf, sizeof(buf));


static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_telink_w91, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	ARG_UNUSED(backend);

	log_format_current = log_type;
	return 0;
}

static void log_backend_telink_w91_init(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);
}

static void panic(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	log_backend_std_panic(&log_output_telink_w91);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_telink_w91, cnt);
}

const struct log_backend_api log_backend_telink_w91_api = {
	.process = process,
	.panic = panic,
	.init = log_backend_telink_w91_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(test, log_backend_telink_w91_api, true);
