/*
 * Copyright (c) 2018, Oticon A/S
 * Copyright (c) 2025, Nordic Semiconductor ASA
 * Copyright (c) 2026, Canonical
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_native_common.h"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <nsi_host_trampolines.h>
#include <nsi_tracing.h>

#define ERROR posix_print_error_and_exit

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void nc_uart_irq_tx_enable(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	bool kick_thread = !data->irq.tx_enabled;

	data->irq.tx_enabled = true;

	if (!atomic_set(&data->irq.thread_started, 1)) {
		nc_uart_irq_thread_start(dev);
	}

	if (kick_thread) {
		/* Let's ensure the thread wakes to allow the Tx right away */
		k_wakeup(&data->irq.poll_thread);
	}
}

void nc_uart_irq_tx_disable(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	data->irq.tx_enabled = false;
}

void nc_uart_irq_rx_enable(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	if (data->stdin_disconnected) {
		/* There won't ever be data => we ignore the request */
		return;
	}

	bool kick_thread = !data->irq.rx_enabled;

	data->irq.rx_enabled = true;

	if (!atomic_set(&data->irq.thread_started, 1)) {
		nc_uart_irq_thread_start(dev);
	}

	if (kick_thread) {
		/* Let's ensure the thread wakes to try to check for data */
		k_wakeup(&data->irq.poll_thread);
	}
}

void nc_uart_irq_rx_disable(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	data->irq.rx_enabled = false;
}

int nc_uart_irq_tx_complete(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

int nc_uart_irq_rx_ready(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	if (data->irq.rx_enabled && data->irq.char_ready) {
		return 1;
	}

	return 0;
}

int nc_uart_irq_tx_ready(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	return data->irq.tx_enabled ? 1 : 0;
}

int nc_uart_irq_is_pending(const struct device *dev)
{
	return nc_uart_irq_rx_ready(dev) ||
		nc_uart_irq_tx_ready(dev);
}

void nc_uart_irq_callback_set(const struct device *dev,
			      uart_irq_callback_user_data_t cb,
			      void *cb_data)
{
	struct native_common_data *data = dev->data;

	data->irq.callback = cb;
	data->irq.cb_data = cb_data;
}

void nc_uart_irq_handler(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	if (data->irq.callback) {
		data->irq.callback(dev, data->irq.cb_data);
	} else {
		ERROR("%s: No callback registered\n", __func__);
	}
}

int nc_uart_fifo_read(const struct device *dev, uint8_t *rx_data, int size)
{
	uint32_t len = 0;
	int ret;
	struct native_common_data *data = dev->data;

	if ((size <= 0) || data->stdin_disconnected) {
		return 0;
	}

	if (data->irq.char_ready) {
		rx_data[0] = data->irq.char_store;
		rx_data++;
		size--;
		len = 1;
		data->irq.char_ready = false;
		/* Note this native_sim driver code cannot be interrupted,
		 * so there is no race with np_uart_irq_thread()
		 */
	}

	ret = nc_uart_read_n(data, rx_data, size);

	if (ret > 0) {
		len += ret;
		nc_uart_irq_read_1_ahead(data);
	}

	return len;
}

int nc_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	return nc_uart_poll_out_n(dev->data, tx_data, size);
}

void nc_uart_irq_read_1_ahead(struct native_common_data *data)
{
	int ret = nc_uart_read_n(data, &data->irq.char_store, 1);

	if (ret == 1) {
		data->irq.char_ready = true;
	}

	if (data->stdin_disconnected) {
		/* There won't be any more data ever */
		data->irq.rx_enabled = false;
	}
}

/*
 * Emulate uart interrupts using a polling thread
 */
void nc_uart_irq_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct device *dev = (struct device *)arg1;
	struct native_common_data *data = dev->data;

	while (1) {
		if (data->irq.rx_enabled) {
			if (!data->irq.char_ready) {
				nc_uart_irq_read_1_ahead(data);
			}

			if (data->irq.char_ready) {
				nc_uart_irq_handler(dev);
			}
		}
		if (data->irq.tx_enabled) {
			nc_uart_irq_handler(dev);
		}

		if ((data->irq.tx_enabled) ||
		    ((data->irq.rx_enabled) && (data->irq.char_ready))) {
			/* There is pending work. Let's handle it right away */
			continue;
		}

		k_timeout_t wait = K_FOREVER;

		if (data->irq.rx_enabled) {
			wait = K_MSEC(10);
		}
		(void)k_sleep(wait);
	}
}

void nc_uart_irq_thread_start(const struct device *dev)
{
	struct native_common_data *data = dev->data;

	/* Create a thread which will wait for data - replacement for IRQ */
	k_thread_create(&data->irq.poll_thread, data->irq.poll_stack,
			K_KERNEL_STACK_SIZEOF(data->irq.poll_stack),
			nc_uart_irq_thread,
			(void *)dev, NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/*
 * @brief Output len characters towards the serial port
 *
 * @param data UART native common device struct
 * @param buf Pointer to the characters to send.
 * @param len Number of characters to send
 */
int nc_uart_poll_out_n(struct native_common_data *data, const unsigned char *buf, size_t len)
{
	if (data->uart_poll_out_pre) {
		data->uart_poll_out_pre(data, buf, len);
	}

	return nsi_host_write(data->out_fd, buf, len);
}

/*
 * @brief Output a character towards the serial port
 *
 * @param dev UART device struct
 * @param out_char Character to send.
 */
void nc_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	(void)nc_uart_poll_out_n(dev->data, &out_char, 1);
}

/**
 * @brief Poll the device for up to len input characters
 *
 * @param data UART native common device struct
 * @param p_char Pointer to character.
 *
 * @retval > 0 If a character arrived and was stored in p_char
 * @retval -1 If no character was available to read
 */
int nc_uart_read_n(struct native_common_data *data, unsigned char *p_char, int len)
{
	int rc = -1;
	int in_f = data->in_fd;

	if (len <= 0) {
		return -1;
	}

	if (data->uart_read_n) {
		rc = data->uart_read_n(data, p_char, len);
	} else {
		rc = nsi_host_read(in_f, p_char, len);
	}

	if (rc > 0) {
		return rc;
	}

	return -1;
}

int nc_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct native_common_data *data = dev->data;

	int ret = nc_uart_read_n(data, p_char, 1);

	if (ret == -1) {
		return -1;
	}

	return 0;
}
