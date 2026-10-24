/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The console this core does not have, presented to Zephyr as a
 * serial device so the shell can use it.
 *
 * Why this exists
 * ---------------
 * UART0 on this device belongs to the system controller: it owns the port
 * and its CLI reads it, while this core never touches the UART registers.
 * So this core has no console of its own -- but it does have the mailbox,
 * whose LOG channel already carries outbound output: a string sent with
 * MB_CMD_LOG_OUT is printed by the system controller on UART0 verbatim.
 *
 * That makes the LOG channel a perfectly good shell transport in both
 * directions:
 *
 * output poll_out() accumulates a line and hands it to
 * soc_bk7258_mbox_log_write(), i.e. the system controller prints it.
 *
 * input poll_in() drains what the system controller forwarded: a line typed on the
 * system controller's console
 * as `ap_cmd <text>` arrives on the same channel as
 * MB_CMD_USER_INPUT and is buffered by the mailbox driver
 * (soc_ipc.c, soc_bk7258_mbox_console_getc()).
 *
 * The system controller registers `ap_cmd` in cli_main.c:1200, builds the line at :1051 and
 * sends it from shell_task.c:2411.
 *
 * Why a serial device rather than a shell backend
 * -----------------------------------------------
 * Zephyr's serial shell backend is the only supported way to attach a shell to
 * something that is not a real UART, and in polling mode it only needs
 * poll_in/poll_out. That is all this file implements, on purpose: there is no
 * interrupt to expose, and no configure() -- the "baud rate" is the system controller's.
 *
 * Therefore the build has to select SHELL_BACKEND_SERIAL_API_POLLING; the
 * interrupt-driven backend would call uart_irq_* on this device, which does not
 * implement them.
 *
 * Line buffering, and why a timer is needed
 * -----------------------------------------
 * The mailbox is metered per message, so one message per character would swamp
 * it: a line is accumulated and sent whole. A newline flushes it.
 *
 * A newline alone is not enough, though: a shell prompt ("uart:~$ ") never ends
 * in one, so with newline-only flushing the prompt would sit in the buffer and
 * the shell would look dead. A short idle timer flushes whatever is left, which
 * costs a few tens of milliseconds of latency on the prompt and nothing at all
 * on ordinary output.
 *
 * The buffers are safe because soc_bk7258_mbox_log_write() copies the line into
 * its own rotating slots before posting: this one is reusable as soon as the
 * call returns.
 */

#define DT_DRV_COMPAT beken_bk7258_mbox_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <soc.h>

/*
 * How long a line may sit in the buffer without a newline before it is sent
 * anyway. Has to cover the case of a prompt. Longer means less mailbox traffic,
 * but above a few tens of milliseconds, typing starts to feel laggy.
 */
#define UART_MBOX_IDLE_FLUSH_MS 20

struct uart_mbox_bk7258_data {
	/* Line being accumulated, not NUL terminated: the length is len */
	char line[SOC_MB_LOG_LINE_LEN];
	uint32_t len;

	/* Lines lost because the mailbox channel was busy */
	uint32_t dropped;

	struct k_work_delayable flush_work;
};

/*
 * Hand the accumulated line to the system controller.
 *
 * Thread context only: it copies into the transport's rotating slot and posts
 * to the mailbox, which is not something to do from an interrupt for console
 * output.
 */
static void uart_mbox_flush(struct uart_mbox_bk7258_data *data)
{
	if (data->len == 0U) {
		return;
	}

	if (soc_bk7258_mbox_log_write(data->line, data->len) != 0) {
		data->dropped++;
	}

	data->len = 0U;
}

static void uart_mbox_flush_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_mbox_bk7258_data *data =
		CONTAINER_OF(dwork, struct uart_mbox_bk7258_data, flush_work);

	uart_mbox_flush(data);
}

static int uart_mbox_bk7258_poll_in(const struct device *dev, unsigned char *p_char)
{
	ARG_UNUSED(dev);

	int c = soc_bk7258_mbox_console_getc();

	/* Zephyr's contract: 0 and the character, or -1 when there is none */
	if (c < 0) {
		return -1;
	}

	*p_char = (unsigned char)c;

	return 0;
}

static void uart_mbox_bk7258_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_mbox_bk7258_data *data = dev->data;

	/*
	 * Terminals want CRLF and the system controller prints whatever it is given, so a bare LF
	 * gets a CR in front of it. The shell may already send CRLF; in that
	 * case the LF follows a CR and nothing is added, otherwise the console
	 * would show one blank line per newline.
	 */
	if ((c == '\n') && ((data->len == 0U) || (data->line[data->len - 1U] != '\r'))) {
		if (data->len >= sizeof(data->line)) {
			uart_mbox_flush(data);
		}

		data->line[data->len++] = '\r';
	}

	/* An overlong line is sent in pieces rather than dropped */
	if (data->len >= sizeof(data->line)) {
		uart_mbox_flush(data);
	}

	data->line[data->len++] = (char)c;

	if (c == '\n') {
		uart_mbox_flush(data);
		return;
	}

	/*
	 * poll_out() is normally called from the shell thread, but the serial
	 * API does not promise that, so the timer is only armed from a
	 * sleepable context.
	 */
	if (k_is_in_isr()) {
		return;
	}

	(void)k_work_reschedule(&data->flush_work, K_MSEC(UART_MBOX_IDLE_FLUSH_MS));
}

static int uart_mbox_bk7258_init(const struct device *dev)
{
	struct uart_mbox_bk7258_data *data = dev->data;

	/*
	 * Nothing to reset and nothing to configure: the transport is the
	 * mailbox, which is already up by now (soc_ipc.c initialises it at
	 * POST_KERNEL priority 0), and the "line settings" belong to the system controller.
	 */
	k_work_init_delayable(&data->flush_work, uart_mbox_flush_handler);

	return 0;
}

static DEVICE_API(uart, uart_mbox_bk7258_driver_api) = {
	.poll_in = uart_mbox_bk7258_poll_in,
	.poll_out = uart_mbox_bk7258_poll_out,
};

#define UART_MBOX_BK7258_INIT(n)                                                                   \
	static struct uart_mbox_bk7258_data uart_mbox_bk7258_data_##n;                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_mbox_bk7258_init, NULL, &uart_mbox_bk7258_data_##n, NULL,    \
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,                            \
			      &uart_mbox_bk7258_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_MBOX_BK7258_INIT)
