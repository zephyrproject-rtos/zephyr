/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Captures the host console UART into a circular log that the BMC console
 * transports replay to their clients.
 */

#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/console.h>
#include <zephyr/spinlock.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

#define UART_NODE      DT_ALIAS(host_console_uart)
#define UART_MUXSEL_NODE DT_ALIAS(host_console_uart_muxsel)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(UART_NODE),
	     "CONFIG_BMC_CONSOLE_LOGGER needs an enabled host-console-uart alias");

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);

#if DT_NODE_HAS_STATUS_OKAY(UART_MUXSEL_NODE)
static const struct gpio_dt_spec uart_muxsel = GPIO_DT_SPEC_GET(UART_MUXSEL_NODE, gpios);
#endif

#define UART_RX_DELAY_US 50000

/*
 * Chunk size of the circular console log handed to the UART RX DMA. It costs
 * no extra memory, it just makes the oldest part of the log unavailable, so it
 * should stay a small fraction of the log. Two chunks are in flight at a time
 * because of the pipelined buffer allocation in the UART driver, which uses up
 * to an eighth of the log space.
 */
#define UART_RX_BUF_SIZE (CONFIG_BMC_CONSOLE_LOG_SIZE / 16)

/* Most interactive input is a handful of characters, so keep the TX DMA small. */
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

K_EVENT_DEFINE(bmc_console_events);

static struct console_log host_console_log;

/* DMA'ed to and from by the UART, so must be __nocache */
static __nocache uint8_t log_buffer[CONFIG_BMC_CONSOLE_LOG_SIZE];
static __nocache uint8_t tx_buffer[UART_TX_BUF_SIZE];

static void uart_rx_ready(struct uart_event *evt, struct console_log *log)
{
	k_spinlock_key_t key;

	key = k_spin_lock(&log->rx_lock);
	log->received += evt->data.rx.len;
	k_spin_unlock(&log->rx_lock, key);

	k_event_post(&bmc_console_events, BMC_CONSOLE_EVENT_DATA);
}

static void uart_rx_allocate(const struct device *dev, struct console_log *log)
{
	k_spinlock_key_t key;
	int off;
	int ret;

	key = k_spin_lock(&log->rx_lock);
	off = log->allocated % log->size;
	log->allocated += UART_RX_BUF_SIZE;
	k_spin_unlock(&log->rx_lock, key);

	ret = uart_rx_buf_rsp(dev, log->log_buffer + off, UART_RX_BUF_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to supply a UART RX buffer (err=%d)", ret);
	}
}

static void uart_rx_start(const struct device *dev, struct console_log *log)
{
	k_spinlock_key_t key;
	int off;
	int ret;

	key = k_spin_lock(&log->rx_lock);
	off = log->allocated % log->size;
	log->allocated += UART_RX_BUF_SIZE;
	k_spin_unlock(&log->rx_lock, key);

	ret = uart_rx_enable(dev, log->log_buffer + off, UART_RX_BUF_SIZE, UART_RX_DELAY_US);
	if (ret < 0) {
		LOG_ERR("Failed to enable UART RX (err=%d)", ret);
	}
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct console_log *log = user_data;

	switch (evt->type) {
	case UART_RX_RDY:
		uart_rx_ready(evt, log);
		break;

	case UART_RX_DISABLED:
		uart_rx_start(dev, log);
		break;

	case UART_RX_BUF_REQUEST:
		uart_rx_allocate(dev, log);
		break;

	case UART_TX_DONE:
		k_sem_give(&log->tx_sem);
		break;

	default:
		break;
	}
}

ssize_t bmc_console_read(uint8_t *buf, size_t size, uint64_t *ppos)
{
	struct console_log *log = &host_console_log;
	k_spinlock_key_t key;
	uint64_t pos = *ppos;
	uint64_t start;
	size_t copied = 0;
	ssize_t ret = 0;

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

	if (start > pos) {
		/* Characters were lost, skip ahead to what is still available. */
		pos = start;
	}

	while (copied < size) {
		int off = pos % log->size;
		int len = MIN(log->received - pos, log->size - off);

		len = MIN(len, size - copied);
		if (len == 0) {
			break;
		}

		memcpy(buf + copied, log->log_buffer + off, len);
		pos += len;
		copied += len;
	}

out:
	k_spin_unlock(&log->rx_lock, key);

	*ppos = pos;

	return copied ? (ssize_t)copied : ret;
}

int bmc_console_seek_end(uint64_t *ppos)
{
	struct console_log *log = &host_console_log;
	k_spinlock_key_t key;

	key = k_spin_lock(&log->rx_lock);
	*ppos = log->received;
	k_spin_unlock(&log->rx_lock, key);

	return 0;
}

ssize_t bmc_console_write(const uint8_t *buf, size_t size)
{
	struct console_log *log = &host_console_log;
	size_t copied = 0;

	while (copied < size) {
		size_t len = MIN(size - copied, UART_TX_BUF_SIZE);
		int ret;

		k_sem_take(&log->tx_sem, K_FOREVER);
		memcpy(log->tx_buffer, buf + copied, len);

		ret = uart_tx(log->uart, log->tx_buffer, len, SYS_FOREVER_US);
		if (ret < 0) {
			LOG_WRN("UART TX error (err=%d)", ret);
			k_sem_give(&log->tx_sem);
			return copied ? (ssize_t)copied : ret;
		}

		copied += len;
	}

	return copied;
}

static const struct uart_config uart_cfg = {
	.baudrate = CONFIG_BMC_CONSOLE_BAUDRATE,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
};

static int console_logger_init(void)
{
	struct console_log *log = &host_console_log;
	int ret;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("Host console UART not ready");
		return -ENODEV;
	}

	memset(log, 0, sizeof(*log));
	log->uart = uart_dev;
	log->size = sizeof(log_buffer);
	log->log_buffer = log_buffer;
	log->tx_buffer = tx_buffer;
	k_sem_init(&log->tx_sem, 1, 1);

#if DT_NODE_HAS_STATUS_OKAY(UART_MUXSEL_NODE)
	if (!gpio_is_ready_dt(&uart_muxsel)) {
		LOG_ERR("Host console UART mux select GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&uart_muxsel, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure the UART mux select GPIO (err=%d)", ret);
		return ret;
	}
#endif

	ret = uart_configure(uart_dev, &uart_cfg);
	if (ret == -ENOSYS || ret == -ENOTSUP) {
		LOG_WRN("UART runtime reconfiguration unavailable, keeping device settings");
	} else if (ret < 0) {
		LOG_ERR("Could not configure the host console UART (err=%d)", ret);
		return ret;
	}

	ret = uart_callback_set(uart_dev, uart_callback, log);
	if (ret < 0) {
		LOG_ERR("Could not set the UART callback (err=%d)", ret);
		return ret;
	}

	uart_rx_start(uart_dev, log);

	LOG_INF("Host console logger started");

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_console_logger, BMC_INIT_PHASE_PLATFORM, console_logger_init, true);
