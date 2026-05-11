/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(console_logger, LOG_LEVEL_INF);

#include <zephyr/spinlock.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#include "synch.h"

#define UART_RX_DELAY_US 50000

#define UART_NODE DT_ALIAS(host_console_uart)
static const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);

BUILD_ASSERT(DT_NODE_EXISTS(UART_NODE), "host-console-uart node missing");

/* UART mux select GPIO from device tree */
#define UARTMUXSEL_NODE DT_ALIAS(host_console_uart_muxsel)
#if DT_NODE_EXISTS(UARTMUXSEL_NODE)
static const struct gpio_dt_spec host_console_uart_muxsel_gpio = GPIO_DT_SPEC_GET(UARTMUXSEL_NODE, gpios);
#endif

/*
 * Chunk size of the circular console log that we give to UART RX DMA. This does not
 * consume any memory (it's just a chunk of the console log buffer region), but it
 * does make the oldest part of the log unavailable so should not be a big fraction of
 * the console log size. Two chunks will be in-flight at any time due to the pipelined
 * buffer allocation in the uart code, so this uses up to 1/8th of the log space.
 */
#define UART_RX_BUF_SIZE (CONFIG_BMC_APP_CONSOLE_LOG_SIZE / 16)

/*
 * TX buf is the size of the DMA buffer we can give to the UART.
 * Probably does not have to be this large, most interactive input
 * will be a handful of characters.
 */
#define UART_TX_BUF_SIZE 32

struct console_log {
	const struct device *uart;
	uint64_t allocated;
	uint64_t received;
	int size;
	struct k_spinlock rx_lock;
	struct k_sem tx_sem;
	uint8_t *log_buffer;
	uint8_t *tx_buffer;
};

static struct console_log host_console_log;

/* DMA'ed to/from by the UART, so must be __nocache */
static __nocache uint8_t log_buffer[CONFIG_BMC_APP_CONSOLE_LOG_SIZE];
static __nocache uint8_t tx_buffer[UART_TX_BUF_SIZE];

static void uart_rx_ready(const struct device *dev, struct uart_event *evt, struct console_log *log)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&log->rx_lock);
	log->received += evt->data.rx.len;
	k_spin_unlock(&log->rx_lock, key);

	k_event_post(&events, EVENT_CONSOLE_LOG_DATA);
}

static void uart_rx_allocate(const struct device *dev, struct console_log *log)
{
	int off;
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&log->rx_lock);
	off = log->allocated % log->size;
	log->allocated += UART_RX_BUF_SIZE;
	k_spin_unlock(&log->rx_lock, key);

	ret = uart_rx_buf_rsp(dev, log->log_buffer + off, UART_RX_BUF_SIZE);
	if (ret < 0)
		LOG_ERR("Failed to supply buffer UART RX: %d", ret);
}

static void uart_rx_start(const struct device *dev, struct console_log *log)
{
	int off;
	int ret;
	k_spinlock_key_t key;

	key = k_spin_lock(&log->rx_lock);
	off = log->allocated % log->size;
	log->allocated += UART_RX_BUF_SIZE;
	k_spin_unlock(&log->rx_lock, key);

	ret = uart_rx_enable(dev, log->log_buffer + off, UART_RX_BUF_SIZE, UART_RX_DELAY_US);
	if (ret < 0)
		LOG_ERR("Failed to enable UART RX: %d", ret);
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct console_log *log = user_data;

	switch (evt->type) {
	case UART_RX_RDY:
		uart_rx_ready(dev, evt, log);
		break;

	case UART_RX_DISABLED:
		uart_rx_start(dev, log);
		break;

	case UART_RX_BUF_REQUEST:
		uart_rx_allocate(dev, log);
		break;

	case UART_RX_STOPPED:
		break;

	case UART_TX_DONE:
		k_sem_give(&log->tx_sem);
		break;

	default:
		break;
	}
}

static ssize_t console_log_read(struct console_log *log, uint8_t *buf, size_t size, uint64_t *ppos)
{
	uint64_t pos = *ppos;
	uint64_t start;
	int off, len;
	k_spinlock_key_t key;
	ssize_t ret = 0;
	size_t copied = 0;

	key = k_spin_lock(&log->rx_lock);
	if (pos > log->received) {
		ret = -EINVAL;
		goto out;
	}

	if (log->received < log->size) {
		start = 0;
	} else {
		start = log->allocated - log->size;
	}

	if (start > pos)
		pos = start; /* lost characters, advance pos */

	while (copied < size) {
		off = pos % log->size;
		len = MIN(log->received - pos, log->size - off);
		len = MIN(len, size - copied);
		if (len == 0)
			break;
		memcpy(buf + copied, log->log_buffer + off, len);
		pos += len;
		copied += len;
	}

out:
	k_spin_unlock(&log->rx_lock, key);

	*ppos = pos;

	return copied ? copied : ret;
}

static ssize_t console_log_write(struct console_log *log, const uint8_t *buf, size_t size)
{
	size_t copied = 0;

	while (copied < size) {
		size_t len = MIN(size - copied, UART_TX_BUF_SIZE);
		int ret;

		k_sem_take(&log->tx_sem, K_FOREVER);
		memcpy(log->tx_buffer, buf + copied, len);

		ret = uart_tx(log->uart, log->tx_buffer, len, SYS_FOREVER_US);
		if (ret < 0) {
			LOG_WRN("UART TX error: %d", ret);
			k_sem_give(&log->tx_sem);
			return copied ? copied : ret;
		}

		copied += len;
	}

	return copied;
}

ssize_t host_console_read(uint8_t *buf, size_t size, uint64_t *ppos)
{
	struct console_log *log = &host_console_log;
	return console_log_read(log, buf, size, ppos);
}

int host_console_seek_end(uint64_t *ppos)
{
	struct console_log *log = &host_console_log;
	k_spinlock_key_t key;

	key = k_spin_lock(&log->rx_lock);
	*ppos = log->received;
	k_spin_unlock(&log->rx_lock, key);

	return 0;
}

ssize_t host_console_write(const uint8_t *buf, size_t size)
{
	struct console_log *log = &host_console_log;

	return console_log_write(log, buf, size);
}

static const struct uart_config uart_cfg = {
	.baudrate = 115200,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
};

int console_logger_init(void)
{
	struct console_log *log = &host_console_log;
	int ret;

	LOG_INF("Starting host console logger");

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -1;
	}

	memset(log, 0, sizeof(struct console_log));
	log->uart = uart_dev;
	log->size = sizeof(log_buffer);
	k_sem_init(&log->tx_sem, 1, 1);
	log->log_buffer = log_buffer;
	log->tx_buffer = tx_buffer;

#if DT_NODE_EXISTS(UARTMUXSEL_NODE)
	/* Initialize host_console_uart_muxsel GPIO */
	if (!gpio_is_ready_dt(&host_console_uart_muxsel_gpio)) {
		LOG_ERR("host_console_uart_muxsel GPIO device not ready");
		return -1;
	}

	ret = gpio_pin_configure_dt(&host_console_uart_muxsel_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure host_console_uart_muxsel GPIO: %d", ret);
		return ret;
	}

	gpio_pin_set_dt(&host_console_uart_muxsel_gpio, 1);
	LOG_INF("host_console_uart_muxsel GPIO set");
#endif

	ret = uart_configure(uart_dev, &uart_cfg);
	if (ret < 0) {
		if ((ret == -ENOSYS) || (ret == -ENOTSUP)) {
			LOG_WRN("UART runtime reconfiguration is unavailable, using existing device settings");
		} else {
			LOG_ERR("Failed to configure UART: %d", ret);
			return ret;
		}
	}

	/* Set up UART callback for async RX */
	ret = uart_callback_set(uart_dev, uart_callback, log);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	/* Enable UART RX to start receiving data */
	uart_rx_start(uart_dev, log);

	LOG_INF("UART callback configured for console logger");

	return 0;
}
