/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>

void shell_init_from_work(struct k_work *work)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	bool log_backend = CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL > 0;
	uint32_t level =
		(CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL > LOG_LEVEL_DBG) ?
		CONFIG_LOG_MAX_LEVEL : CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL;

	shell_init(shell_backend_uart_get_ptr(), dev,
		   shell_backend_uart_get_ptr()->ctx->cfg.flags,
		   log_backend, level);
}

static void shell_reinit_trigger(void)
{
	static struct k_work shell_init_work;

	k_work_init(&shell_init_work, shell_init_from_work);
	int err = k_work_submit(&shell_init_work);

	(void)err;
	__ASSERT_NO_MSG(err >= 0);
}

static void direct_uart_callback(const struct device *dev, void *user_data)
{
	static uint8_t buf[1];
	static bool tx_busy;

	uart_irq_update(dev);


	if (uart_irq_rx_ready(dev)) {
		while (uart_fifo_read(dev, buf, sizeof(buf))) {
			if (!tx_busy) {
				uart_irq_tx_enable(dev);
			}
		}
	}

	if (uart_irq_tx_ready(dev)) {
		if (!tx_busy) {
			(void)uart_fifo_fill(dev, buf, sizeof(buf));
			tx_busy = true;
		} else {
			tx_busy = false;
			uart_irq_tx_disable(dev);
			if (buf[0] == 'x') {
				uart_irq_rx_disable(dev);
				shell_reinit_trigger();
			}
		}
	}
}

static void uart_poll_timer_stopped(struct k_timer *timer)
{
	shell_reinit_trigger();
}

static void uart_poll_timeout(struct k_timer *timer)
{
	char c;
	const struct device *dev = k_timer_user_data_get(timer);

	while (uart_poll_in(dev, &c) == 0) {
		if (c != 'x') {
			uart_poll_out(dev, c);
		} else {
			k_timer_stop(timer);
		}
	}
}

K_TIMER_DEFINE(uart_poll_timer, uart_poll_timeout, uart_poll_timer_stopped);

static void shell_uninit_cb(const struct shell *sh, int res)
{
	__ASSERT_NO_MSG(res >= 0);
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_INTERRUPT_DRIVEN)) {
		/* connect uart to my handler */
		uart_irq_callback_user_data_set(dev, direct_uart_callback, NULL);
		uart_irq_rx_enable(dev);
	} else {
		k_timer_user_data_set(&uart_poll_timer, (void *)dev);
		k_timer_start(&uart_poll_timer, K_MSEC(10), K_MSEC(10));
	}
}

static int cmd_uart_release(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (sh != shell_backend_uart_get_ptr()) {
		shell_error(sh, "Command dedicated for shell over uart");
		return -EINVAL;
	}

	shell_print(sh, "Uninitializing shell, use 'x' to reinitialize");
	shell_uninit(sh, shell_uninit_cb);

	return 0;
}

SHELL_CMD_REGISTER(shell_uart_release, NULL,
		"Uninitialize shell instance and release uart, start loopback "
		"on uart. Shell instance is reinitialized when 'x' is pressed",
		cmd_uart_release);
