/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <adi_tmc_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc_uart, CONFIG_STEPPER_LOG_LEVEL);

/* TMC UART standard datagram size */
#define ADI_TMC_UART_DATAGRAM_SIZE          8
/* TMC UART read request datagram size */
#define ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE 4
/* TMC UART protocol constants */
#define ADI_TMC_UART_SYNC_BYTE              0x05
#define ADI_TMC_UART_WRITE_BIT              0x80
/* Bit times per byte: start bit, 8 data bits, stop bit */
#define ADI_TMC_UART_BITS_PER_BYTE          10
/* Maximum reply delay (SENDDELAY=15) in bit times */
#define ADI_TMC_UART_MAX_SEND_DELAY_BITS    (15 * 8)
/*
 * Bus idle time in bit times after each transfer: the node releases the line four bit times
 * after the stop bit of its reply and then needs twelve bit times of idle bus to recover from
 * a transmission error.
 */
#define ADI_TMC_UART_BUS_IDLE_BITS          (4 + 12)

static int tmc_uart_transfer(const struct device *uart, const uint8_t *tx_buf, size_t tx_len,
			     uint8_t *rx_buf, size_t rx_len);

uint8_t tmc_uart_calc_crc(const uint8_t *datagram, uint8_t len)
{
	uint8_t crc = 0;

	for (uint8_t i = 0; i < len; i++) {
		uint8_t current_byte = datagram[i];

		for (uint8_t j = 0; j < 8; j++) {
			if ((crc >> 7) ^ (current_byte & 0x01)) {
				crc = (crc << 1) ^ 0x07;
			} else {
				crc = (crc << 1) & 0xFF;
			}
			current_byte = current_byte >> 1;
		}
	}

	return crc;
}

/*
 * Discard stale received data, e.g. a byte with framing error caught while the line was
 * pulled low before the UART was enabled. Otherwise it would be taken as the echo of the
 * first datagram byte and shift all following echoes by one byte.
 */
static void tmc_uart_flush_rx(const struct device *uart)
{
	uint8_t byte;

	while (uart_poll_in(uart, &byte) == 0) {
		/* Discard */
	}
}

int tmc_uart_write_register(const struct device *uart, uint8_t device_addr,
			    uint8_t register_address, uint32_t data)
{
	uint8_t buffer[ADI_TMC_UART_DATAGRAM_SIZE];
	uint8_t echo[ADI_TMC_UART_DATAGRAM_SIZE];

	/* Format the UART TMC datagram */
	buffer[0] = ADI_TMC_UART_SYNC_BYTE;                    /* Sync byte */
	buffer[1] = device_addr;                               /* Device address */
	buffer[2] = register_address | ADI_TMC_UART_WRITE_BIT; /* Register address with write bit */
	sys_put_be32(data, &buffer[3]);                        /* Write data */
	buffer[7] = tmc_uart_calc_crc(buffer, ADI_TMC_UART_DATAGRAM_SIZE - 1U); /* CRC */

	return tmc_uart_transfer(uart, buffer, sizeof(buffer), echo, sizeof(echo));
}

int tmc_uart_read_register(const struct device *uart, uint8_t device_addr, uint8_t register_address,
			   uint32_t *data)
{
	uint8_t write_buffer[ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE];
	/* Echo of the read request followed by the reply datagram */
	uint8_t rx_buffer[ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE + ADI_TMC_UART_DATAGRAM_SIZE];
	const uint8_t *read_buffer = &rx_buffer[ADI_TMC_UART_READ_REQ_DATAGRAM_SIZE];
	int err;

	/* Format the UART TMC datagram for read */
	write_buffer[0] = ADI_TMC_UART_SYNC_BYTE; /* Sync byte */
	write_buffer[1] = device_addr;            /* Device address */
	write_buffer[2] = register_address;       /* Register address */
	write_buffer[3] = tmc_uart_calc_crc(write_buffer, 3);

	err = tmc_uart_transfer(uart, write_buffer, sizeof(write_buffer), rx_buffer,
				sizeof(rx_buffer));
	if (err < 0) {
		return err;
	}

	/* Print the received bytes */
	LOG_HEXDUMP_DBG(read_buffer, ADI_TMC_UART_DATAGRAM_SIZE, "Received bytes:");

	/* Validate CRC */
	uint8_t crc = tmc_uart_calc_crc(read_buffer, ADI_TMC_UART_DATAGRAM_SIZE - 1U);

	if (crc != read_buffer[7]) {
		LOG_ERR("CRC mismatch for register 0x%x: got 0x%x, expected 0x%x", register_address,
			read_buffer[7], crc);
		return -EIO;
	}

	if (read_buffer[2] != register_address) {
		LOG_ERR("Reply for register 0x%x instead of 0x%x", read_buffer[2],
			register_address);
		return -EIO;
	}

	/* Construct a 32-bit register value from received bytes */
	*data = sys_get_be32(&read_buffer[3]);

	return 0;
}

/*
 * The node discards a datagram if the pause between two bytes exceeds 63 bit times. The
 * datagram is therefore transmitted from the UART interrupt, so that preemption of the
 * calling thread cannot stretch it. The receiver collects the echo of the transmitted bytes
 * followed by the reply, if any.
 */
struct tmc_uart_xfer {
	const uint8_t *tx_buf;
	size_t tx_len;
	size_t tx_pos;
	uint8_t *rx_buf;
	size_t rx_len;
	size_t rx_pos;
	struct k_sem done;
};

/*
 * Several nodes can share one UART, and each transfer installs its own UART callback.
 * Serialize all transfers, as the bus is only identified by its UART device.
 */
static K_SEM_DEFINE(tmc_uart_bus_sem, 1, 1);

static void tmc_uart_isr(const struct device *uart, void *user_data)
{
	struct tmc_uart_xfer *xfer = user_data;
	uint8_t discard;
	int len;

	uart_irq_update(uart);

	while (uart_irq_rx_ready(uart) > 0) {
		if (xfer->rx_pos == xfer->rx_len) {
			/* Discard data beyond the expected length */
			if (uart_fifo_read(uart, &discard, 1) <= 0) {
				break;
			}
			continue;
		}

		len = uart_fifo_read(uart, &xfer->rx_buf[xfer->rx_pos],
				     (int)(xfer->rx_len - xfer->rx_pos));
		if (len <= 0) {
			break;
		}

		xfer->rx_pos += (size_t)len;
		if (xfer->rx_pos == xfer->rx_len) {
			k_sem_give(&xfer->done);
		}
	}

	if (uart_irq_tx_ready(uart) > 0) {
		if (xfer->tx_pos < xfer->tx_len) {
			len = uart_fifo_fill(uart, &xfer->tx_buf[xfer->tx_pos],
					     (int)(xfer->tx_len - xfer->tx_pos));
			if (len > 0) {
				xfer->tx_pos += (size_t)len;
			}
		} else {
			uart_irq_tx_disable(uart);
		}
	}
}

/*
 * Transmit tx_len bytes and receive rx_len bytes, starting with the echo of the transmitted
 * bytes.
 */
static int tmc_uart_transfer(const struct device *uart, const uint8_t *tx_buf, size_t tx_len,
			     uint8_t *rx_buf, size_t rx_len)
{
	struct tmc_uart_xfer xfer = {
		.tx_buf = tx_buf,
		.tx_len = tx_len,
		.rx_buf = rx_buf,
		.rx_len = rx_len,
	};
	struct uart_config uart_cfg;
	uint64_t timeout_us;
	size_t bits;
	int err;

	err = uart_config_get(uart, &uart_cfg);
	if (err < 0) {
		LOG_ERR("Failed to get UART configuration: %d", err);
		return -EIO;
	}

	/* Twice the transfer time including the maximum reply delay, plus 1 ms */
	bits = (tx_len + rx_len) * ADI_TMC_UART_BITS_PER_BYTE + ADI_TMC_UART_MAX_SEND_DELAY_BITS;
	timeout_us = (uint64_t)bits * 2U * USEC_PER_SEC / uart_cfg.baudrate + USEC_PER_MSEC;

	k_sem_init(&xfer.done, 0, 1);

	k_sem_take(&tmc_uart_bus_sem, K_FOREVER);

	uart_irq_tx_disable(uart);
	uart_irq_rx_disable(uart);
	tmc_uart_flush_rx(uart);

	err = uart_irq_callback_user_data_set(uart, tmc_uart_isr, &xfer);
	if (err < 0) {
		LOG_ERR("Failed to set UART callback: %d", err);
		k_sem_give(&tmc_uart_bus_sem);
		return err;
	}

	uart_irq_rx_enable(uart);
	uart_irq_tx_enable(uart);

	err = k_sem_take(&xfer.done, K_USEC(timeout_us));

	uart_irq_tx_disable(uart);
	uart_irq_rx_disable(uart);
	(void)uart_irq_callback_user_data_set(uart, NULL, NULL);

	k_busy_wait((ADI_TMC_UART_BUS_IDLE_BITS * USEC_PER_SEC) / uart_cfg.baudrate + 1U);

	k_sem_give(&tmc_uart_bus_sem);

	if (xfer.rx_pos < tx_len) {
		LOG_ERR("Echo timeout: received %zu of %zu bytes", xfer.rx_pos, tx_len);
		return -EIO;
	}

	if (memcmp(rx_buf, tx_buf, tx_len) != 0) {
		LOG_ERR("Echo mismatch");
		LOG_HEXDUMP_DBG(tx_buf, tx_len, "sent");
		LOG_HEXDUMP_DBG(rx_buf, tx_len, "received");
		return -EIO;
	}

	if (err != 0) {
		LOG_ERR("Reply timeout: received %zu of %zu bytes", xfer.rx_pos - tx_len,
			rx_len - tx_len);
		return -EAGAIN;
	}

	return 0;
}
