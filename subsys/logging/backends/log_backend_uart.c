/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
LOG_MODULE_REGISTER(log_uart);

/* Fixed size to avoid auto-added trailing '\0'.
 * Used if CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX.
 */
static const char LOG_HEX_SEP[10] = "##ZLOGV1##";

static const struct device *const uart_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static struct k_sem sem;
static volatile bool in_panic;
static bool use_async;
static uint32_t log_format_current = CONFIG_LOG_BACKEND_UART_OUTPUT_DEFAULT;

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

	if (pm_device_runtime_is_enabled(uart_dev) && !k_is_in_isr()) {
		if (pm_device_runtime_get(uart_dev) < 0) {
			/* Enabling the UART instance has failed but this
			 * function MUST return the number of bytes consumed.
			 */
			return length;
		}
	}

	if (IS_ENABLED(CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX)) {
		dict_char_out_hex(data, length);
		goto cleanup;
	}

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_UART_ASYNC) || in_panic || !use_async) {
		for (size_t i = 0; i < length; i++) {
			uart_poll_out(uart_dev, data[i]);
		}
		goto cleanup;
	}

	err = uart_tx(uart_dev, data, length, SYS_FOREVER_US);
	__ASSERT_NO_MSG(err == 0);

	err = k_sem_take(&sem, K_FOREVER);
	__ASSERT_NO_MSG(err == 0);

	(void)err;
cleanup:
	if (pm_device_runtime_is_enabled(uart_dev) && !k_is_in_isr()) {
		/* As errors cannot be returned, ignore the return value */
		(void)pm_device_runtime_put(uart_dev);
	}

	return length;
}

static uint8_t uart_output_buf[CONFIG_LOG_BACKEND_UART_BUFFER_SIZE];
LOG_OUTPUT_DEFINE(log_output_uart, char_out, uart_output_buf, sizeof(uart_output_buf));

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_uart, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void log_backend_uart_init(struct log_backend const *const backend)
{
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
	/* Ensure that the UART device is in active mode */
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	if (pm_device_runtime_is_enabled(uart_dev)) {
		if (k_is_in_isr()) {
			/* pm_device_runtime_get cannot be used from ISRs */
			pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
		} else {
			pm_device_runtime_get(uart_dev);
		}
	}
#elif defined(CONFIG_PM_DEVICE)
	enum pm_device_state pm_state;
	int rc;

	rc = pm_device_state_get(uart_dev, &pm_state);
	if ((rc == 0) && (pm_state == PM_DEVICE_STATE_SUSPENDED)) {
		pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
	}
#endif /* CONFIG_PM_DEVICE */

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

const struct log_backend_api log_backend_uart_api = {
	.process = process,
	.panic = panic,
	.init = log_backend_uart_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_uart, log_backend_uart_api,
		IS_ENABLED(CONFIG_LOG_BACKEND_UART_AUTOSTART));
