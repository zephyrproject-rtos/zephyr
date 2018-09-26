/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONSOLE_H_
#define ZEPHYR_INCLUDE_CONSOLE_H_

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

	u8_t *tx_ringbuf;
	u32_t tx_ringbuf_sz;
	u16_t tx_get, tx_put;
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
 * @brief Input a character from a tty device.
 *
 * @param tty tty device structure
 */
u8_t tty_getchar(struct tty_serial *tty);

/**
 * @brief Output a character from to tty device.
 *
 * @param tty tty device structure
 * @param c character to output
 * @return 0 if ok, <0 if error
 */
int tty_putchar(struct tty_serial *tty, u8_t c);

/** @brief Initialize console_getchar()/putchar() calls.
 *
 *  This function should be called once to initialize pull-style
 *  access to console via console_getchar() function and buffered
 *  output using console_putchar() function. This function supersedes,
 *  and incompatible with, callback (push-style) console handling
 *  (via console_input_fn callback, etc.).
 *
 *  @return N/A
 */
void console_init(void);

/** @brief Get next char from console input buffer.
 *
 *  Return next input character from console. If no characters available,
 *  this function will block. This function is similar to ANSI C
 *  getchar() function and is intended to ease porting of existing
 *  software. Before this function can be used, console_getchar_init()
 *  should be called once. This function is incompatible with native
 *  Zephyr callback-based console input processing, shell subsystem,
 *  or console_getline().
 *
 *  @return A character read, including control characters.
 */
u8_t console_getchar(void);

/** @brief Output a char to console (buffered).
 *
 *  Puts a character into console output buffer. It will be sent
 *  to a console asynchronously, e.g. using an IRQ handler.
 *
 *  @return -1 on output buffer overflow, otherwise 0.
 */
int console_putchar(char c);

/** @brief Initialize console_getline() call.
 *
 *  This function should be called once to initialize pull-style
 *  access to console via console_getline() function. This function
 *  supersedes, and incompatible with, callback (push-style) console
 *  handling (via console_input_fn callback, etc.).
 *
 *  @return N/A
 */
void console_getline_init(void);

/** @brief Get next line from console input buffer.
 *
 *  Return next input line from console. Until full line is available,
 *  this function will block. This function is similar to ANSI C
 *  gets() function (except a line is returned in system-owned buffer,
 *  and system takes care of the buffer overflow checks) and is
 *  intended to ease porting of existing software. Before this function
 *  can be used, console_getline_init() should be called once. This
 *  function is incompatible with native Zephyr callback-based console
 *  input processing, shell subsystem, or console_getchar().
 *
 *  @return A pointer to a line read, not including EOL character(s).
 *          A line resides in a system-owned buffer, so an application
 *          should finish any processing of this line immediately
 *          after console_getline() call, or the buffer can be reused.
 */
char *console_getline(void);

/** @brief Initialize legacy fifo-based line input
 *
 *  Input processing is started when string is typed in the console.
 *  Carriage return is translated to NULL making string always NULL
 *  terminated. Application before calling register function need to
 *  initialize two fifo queues mentioned below.
 *
 *  This is a special-purpose function, it's recommended to use
 *  console_getchar() or console_getline() functions instead.
 *
 *  @param avail_queue k_fifo queue keeping available line buffers
 *  @param out_queue k_fifo queue of entered lines which to be processed
 *         in the application code.
 *  @param completion callback for tab completion of entered commands
 */
void console_register_line_input(struct k_fifo *avail_queue,
				 struct k_fifo *out_queue,
				 u8_t (*completion)(char *str, u8_t len));


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_CONSOLE_H_ */
