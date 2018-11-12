/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TTY_H_
#define ZEPHYR_INCLUDE_TTY_H_

#include <sys/types.h>
#include <zephyr/types.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tty_serial {
	struct device *uart_dev;

	struct k_sem rx_sem;
	u8_t *rx_ringbuf;
	u32_t rx_ringbuf_sz;
	u16_t rx_get, rx_put;
	s32_t rx_timeout;

	struct k_sem tx_sem;
	u8_t *tx_ringbuf;
	u32_t tx_ringbuf_sz;
	u16_t tx_get, tx_put;
	s32_t tx_timeout;
};

/**
 * @brief Initialize buffered serial port (classically known as tty).
 *
 * "tty" device provides buffered, interrupt-driver access to an
 * underlying UART device.
 *
 * @param tty tty device structure to initialize
 * @param uart_dev underlying UART device to use (should support
 *                 interrupt-driven operation)
 * @param rxbuf pointer to receive buffer
 * @param rxbuf_sz size of receive buffer
 * @param txbuf pointer to transmit buffer
 * @param txbuf_sz size of transmit buffer
 *
 * @return N/A
 */
void tty_init(struct tty_serial *tty, struct device *uart_dev,
	      u8_t *rxbuf, u16_t rxbuf_sz,
	      u8_t *txbuf, u16_t txbuf_sz);

/**
 * @brief Set receive timeout for tty device.
 *
 * Set timeout for getchar() operation. Default timeout after
 * device initialization is K_FOREVER.
 *
 * @param tty tty device structure
 * @param timeout timeout in milliseconds, or K_FOREVER, or K_NO_WAIT
 */
static inline void tty_set_rx_timeout(struct tty_serial *tty, s32_t timeout)
{
	tty->rx_timeout = timeout;
}

/**
 * @brief Set transmit timeout for tty device.
 *
 * Set timeout for putchar() operation, for a case when output buffer is full.
 * Default timeout after device initialization is K_FOREVER.
 *
 * @param tty tty device structure
 * @param timeout timeout in milliseconds, or K_FOREVER, or K_NO_WAIT
 */
static inline void tty_set_tx_timeout(struct tty_serial *tty, s32_t timeout)
{
	tty->tx_timeout = timeout;
}

/**
 * @brief Read data from a tty device.
 *
 * @param tty tty device structure
 * @param buf buffer to read data to
 * @param size maximum number of bytes to read
 * @return >0, number of actually read bytes (can be less than size param)
 *         =0, for EOF-like condition (e.g., break signalled)
 *         <0, in case of error (e.g. -EAGAIN if timeout expired). errno
 *             variable is also set.
 */
ssize_t tty_read(struct tty_serial *tty, void *buf, size_t size);

/**
 * @brief Write data to tty device.
 *
 * @param tty tty device structure
 * @param buf buffer containg data
 * @param size maximum number of bytes to write
 * @return =>0, number of actually written bytes (can be less than size param)
 *         <0, in case of error (e.g. -EAGAIN if timeout expired). errno
 *             variable is also set.
 */
ssize_t tty_write(struct tty_serial *tty, const void *buf, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_TTY_H_ */
