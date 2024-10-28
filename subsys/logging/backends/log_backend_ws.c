/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(log_backend_ws, CONFIG_LOG_DEFAULT_LEVEL);

#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_ws.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>

/* Set this to 1 if you want to see what is being sent to server */
#define DEBUG_PRINTING 0

#define DBG(fmt, ...) IF_ENABLED(DEBUG_PRINTING, (printk(fmt, ##__VA_ARGS__)))

static bool ws_init_done;
static bool panic_mode;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_WS_OUTPUT_DEFAULT;
static uint8_t output_buf[CONFIG_LOG_BACKEND_WS_MAX_BUF_SIZE];
static size_t pos;

static struct log_backend_ws_ctx {
	int sock;
} ctx = {
	.sock = -1,
};

static void wait(void)
{
	k_msleep(CONFIG_LOG_BACKEND_WS_TX_RETRY_DELAY_MS);
}

static int ws_send_all(int sock, const char *output, size_t len)
{
	int ret;

	while (len > 0) {
		ret = zsock_send(sock, output, len, ZSOCK_MSG_DONTWAIT);
		if ((ret < 0) && (errno == EAGAIN)) {
			return -EAGAIN;
		}

		if (ret < 0) {
			ret = -errno;
			return ret;
		}

		output += ret;
		len -= ret;
	}

	return 0;
}

static int ws_console_out(struct log_backend_ws_ctx *ctx, int c)
{
	static int max_cnt = CONFIG_LOG_BACKEND_WS_TX_RETRY_CNT;
	bool printnow = false;
	unsigned int cnt = 0;
	int ret = 0;

	if (pos >= (sizeof(output_buf) - 1)) {
		printnow = true;
	} else {
		if ((c != '\n') && (c != '\r')) {
			output_buf[pos++] = c;
		} else {
			printnow = true;
		}
	}

	if (printnow) {
		while (ctx->sock >= 0 && cnt < max_cnt) {
			ret = ws_send_all(ctx->sock, output_buf, pos);
			if (ret < 0) {
				if (ret == -EAGAIN) {
					wait();
					cnt++;
					continue;
				}
			}

			break;
		}

		if (ctx->sock >= 0 && ret == 0) {
			/* We could send data */
			pos = 0;
		} else {
			/* If the line is full and we cannot send, then
			 * ignore the output data in buffer.
			 */
			if (pos >= (sizeof(output_buf) - 1)) {
				pos = 0;
			}
		}
	}

	return cnt;
}

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	struct log_backend_ws_ctx *ctx = (struct log_backend_ws_ctx *)output_ctx;
	int ret = -ENOMEM;

	if (ctx == NULL || ctx->sock == -1) {
		return length;
	}

	for (int i = 0; i < length; i++) {
		ret = ws_console_out(ctx, data[i]);
		if (ret < 0) {
			goto fail;
		}
	}

	length = ret;

	DBG(data);
fail:
	return length;
}

LOG_OUTPUT_DEFINE(log_output_ws, line_out, output_buf, sizeof(output_buf));

static int do_ws_init(struct log_backend_ws_ctx *ctx)
{
	log_output_ctx_set(&log_output_ws, ctx);

	return 0;
}

static void process(const struct log_backend *const backend,
		    union log_msg_generic *msg)
{
	uint32_t flags = LOG_OUTPUT_FLAG_FORMAT_SYSLOG |
			 LOG_OUTPUT_FLAG_TIMESTAMP |
			 LOG_OUTPUT_FLAG_THREAD;
	log_format_func_t log_output_func;

	if (panic_mode) {
		return;
	}

	if (!ws_init_done && do_ws_init(&ctx) == 0) {
		ws_init_done = true;
	}

	log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_ws, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

void log_backend_ws_start(void)
{
	const struct log_backend *backend = log_backend_ws_get();

	if (!log_backend_is_active(backend)) {
		log_backend_activate(backend, backend->cb->ctx);
	}
}

int log_backend_ws_register(int fd)
{
	struct log_backend_ws_ctx *ctx = log_output_ws.control_block->ctx;

	ctx->sock = fd;

	return 0;
}

int log_backend_ws_unregister(int fd)
{
	struct log_backend_ws_ctx *ctx = log_output_ws.control_block->ctx;

	if (ctx->sock != fd) {
		DBG("Websocket sock mismatch (%d vs %d)", ctx->sock, fd);
	}

	ctx->sock = -1;

	return 0;
}

static void init_ws(struct log_backend const *const backend)
{
	ARG_UNUSED(backend);

	log_backend_deactivate(log_backend_ws_get());
}

static void panic(struct log_backend const *const backend)
{
	panic_mode = true;
}

const struct log_backend_api log_backend_ws_api = {
	.panic = panic,
	.init = init_ws,
	.process = process,
	.format_set = format_set,
};

/* Note that the backend can be activated only after we have networking
 * subsystem ready so we must not start it immediately.
 */
LOG_BACKEND_DEFINE(log_backend_ws, log_backend_ws_api,
		   IS_ENABLED(CONFIG_LOG_BACKEND_WS_AUTOSTART));

const struct log_backend *log_backend_ws_get(void)
{
	return &log_backend_ws;
}
