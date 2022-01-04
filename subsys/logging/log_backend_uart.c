/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
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
#include <drivers/uart.h>
#include <sys/__assert.h>
LOG_MODULE_REGISTER(log_uart);

/* Fixed size to avoid auto-added trailing '\0'.
 * Used if CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX.
 */
static const char LOG_HEX_SEP[10] = "##ZLOGV1##";

static const struct device *uart_dev;
static struct k_sem sem;
static volatile bool in_panic;
static bool use_async;

static void uart_callback(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&sem);
		break;
	default:
		break;
	}
}

static void dict_char_out_hex(uint8_t *data, size_t length)
{
	for (size_t i = 0; i < length; i++) {
		char c;
		uint8_t x;

		/* upper 8-bit */
		x = data[i] >> 4;
		(void)hex2char(x, &c);
		uart_poll_out(uart_dev, c);

		/* lower 8-bit */
		x = data[i] & 0x0FU;
		(void)hex2char(x, &c);
		uart_poll_out(uart_dev, c);
	}
}

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
	int err;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX)) {
		dict_char_out_hex(data, length);
		return length;
	}

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_UART_ASYNC) || in_panic || !use_async) {
		for (size_t i = 0; i < length; i++) {
			uart_poll_out(uart_dev, data[i]);
		}

		return length;
	}

	err = uart_tx(uart_dev, data, length, SYS_FOREVER_US);
	__ASSERT_NO_MSG(err == 0);

	err = k_sem_take(&sem, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	(void)err;

	return length;
}

static uint8_t uart_output_buf[IS_ENABLED(CONFIG_LOG_BACKEND_UART_ASYNC) ? 32 : 1];
LOG_OUTPUT_DEFINE(log_output_uart, char_out, uart_output_buf, sizeof(uart_output_buf));

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_UART_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_put(&log_output_uart, flag, msg);
}

static void process(const struct log_backend *const backend,
		union log_msg2_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY)) {
		log_dict_output_msg2_process(&log_output_uart,
					     &msg->log, flags);
	} else {
		log_output_msg2_process(&log_output_uart, &msg->log, flags);
	}
}

static void log_backend_uart_init(struct log_backend const *const backend)
{
	uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	__ASSERT_NO_MSG(device_is_ready(uart_dev));

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX)) {
		/* Print a separator so the output can be fed into
		 * log parser directly. This is useful when capturing
		 * from UART directly where there might be other output
		 * (e.g. bootloader).
		 */
		for (int i = 0; i < sizeof(LOG_HEX_SEP); i++) {
			uart_poll_out(uart_dev, LOG_HEX_SEP[i]);
		}

		return;
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART_ASYNC)) {
		int err = uart_callback_set(uart_dev, uart_callback, NULL);

		if (err == 0) {
			use_async = true;
			k_sem_init(&sem, 0, 1);
		} else {
			LOG_WRN("Failed to initialize asynchronous mode (err:%d). "
				"Fallback to polling.", err);
		}
	}
}

static void panic(struct log_backend const *const backend)
{
	in_panic = true;
	log_backend_std_panic(&log_output_uart);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY)) {
		log_dict_output_dropped_process(&log_output_uart, cnt);
	} else {
		log_backend_std_dropped(&log_output_uart, cnt);
	}
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
	.process = IS_ENABLED(CONFIG_LOG2) ? process : NULL,
	.put = IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) ? put : NULL,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.init = log_backend_uart_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_uart, log_backend_uart_api, true);
