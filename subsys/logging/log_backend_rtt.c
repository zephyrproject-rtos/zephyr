/*
 * Copyright (c) 2018 Workaround GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <device.h>
#include <rtt/SEGGER_RTT.h>

static int buffer_out(u8_t *data, size_t length, void *ctx)
{
	int written = SEGGER_RTT_Write(0, data, length);

	return written;
}

static u8_t buf[CONFIG_LOG_BACKEND_RTT_BUFFER_SIZE];

LOG_OUTPUT_DEFINE(log_output, buffer_out, buf, sizeof(buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	u32_t flags = 0;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_SHOW_COLOR)) {
		flags |= LOG_OUTPUT_FLAG_COLORS;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	memset(buf, 0, sizeof(buf));

	log_msg_get(msg);

	log_output_msg_process(&log_output, msg, flags);

	log_msg_put(msg);
}

static void log_backend_rtt_init(void)
{
	SEGGER_RTT_Init();
}

static void panic(struct log_backend const *const backend)
{
	/* Nothing to be done, this backend can always process logs */
}

const struct log_backend_api log_backend_rtt_api = {
	.put = put,
	.panic = panic,
	.init = log_backend_rtt_init,
};

LOG_BACKEND_DEFINE(log_backend_rtt, log_backend_rtt_api);
