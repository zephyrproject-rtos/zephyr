/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief A driver for sending and receiving mcumgr packets over UART.
 *
 * @see include/zephyr/mgmt/mcumgr/transport/serial.h
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CONSOLE_UART_MCUMGR_H_
#define ZEPHYR_INCLUDE_DRIVERS_CONSOLE_UART_MCUMGR_H_

#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Framing of mcumgr packets on a UART.
 */
enum uart_mcumgr_framing {
	/** SMP over console: base64-encoded frames with a CRC16, each ending in a newline. */
	UART_MCUMGR_FRAMING_SMP_OVER_CONSOLE,
	/** Raw mcumgr packets, without encoding. */
	UART_MCUMGR_FRAMING_RAW,
};

/**
 * @brief Contains an mcumgr fragment received over UART.
 */
struct uart_mcumgr_rx_buf {
	void *fifo_reserved;   /* 1st word reserved for use by fifo */
	uint8_t data[CONFIG_UART_MCUMGR_RX_BUF_SIZE];
	int length;
};

/**
 * @brief Function that gets called when an mcumgr fragment is received.
 *
 * Function that gets called when an mcumgr fragment is received.  This function
 * gets called in the interrupt context.  Ownership of the specified buffer is
 * transferred to the callback, which frees it with uart_mcumgr_free_rx_buf().
 * A fragment is one line with SMP over console framing and one byte with raw
 * framing.
 *
 * @param rx_buf                A buffer containing the incoming mcumgr fragment.
 * @param user_data             The pointer passed to uart_mcumgr_register().
 */
typedef void uart_mcumgr_recv_fn(struct uart_mcumgr_rx_buf *rx_buf, void *user_data);

/**
 * @brief An mcumgr UART instance.
 *
 * Define instances with UART_MCUMGR_DEFINE(). The members are internal.
 */
struct uart_mcumgr {
	/** @cond INTERNAL_HIDDEN */
	const struct device *dev;
	struct k_mem_slab *rx_slab;
	enum uart_mcumgr_framing framing;
	uart_mcumgr_recv_fn *recv_cb;
	void *user_data;
	struct uart_mcumgr_rx_buf *cur_buf;
	bool ignoring;
#if defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
	uint8_t async_current;
	uint8_t (*async_buf)[CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUF_SIZE];
#endif
	/** @endcond */
};

/**
 * @brief Defines an mcumgr UART instance.
 *
 * Defines static instance @p _name for UART @p _dev, with its own
 * CONFIG_UART_MCUMGR_RX_BUF_COUNT receive buffers. A UART can have one instance.
 * The framing must be built: CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE
 * for SMP over console, CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE
 * for raw framing.
 *
 * @param _name                 Name of the instance.
 * @param _dev                  UART device.
 * @param _framing              Framing, an @ref uart_mcumgr_framing constant.
 */
#define UART_MCUMGR_DEFINE(_name, _dev, _framing)					\
	BUILD_ASSERT((_framing) != UART_MCUMGR_FRAMING_SMP_OVER_CONSOLE ||		\
		     IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE),	\
		     "SMP over console framing is not built");				\
	BUILD_ASSERT((_framing) != UART_MCUMGR_FRAMING_RAW ||				\
		     IS_ENABLED(							\
			     CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE), \
		     "Raw framing is not built");					\
	K_MEM_SLAB_DEFINE_STATIC_TYPE(_name##_rx_slab, struct uart_mcumgr_rx_buf,	\
				      CONFIG_UART_MCUMGR_RX_BUF_COUNT);			\
	IF_ENABLED(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC,					\
		   (static uint8_t _name##_async_buf						\
			    [CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUFS]				\
			    [CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUF_SIZE];))			\
	static struct uart_mcumgr _name = {						\
		.dev = (_dev),								\
		.rx_slab = &_name##_rx_slab,						\
		.framing = (_framing),							\
		IF_ENABLED(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC,				\
			   (.async_buf = _name##_async_buf,))				\
	}

/**
 * @brief Sends an mcumgr packet over UART.
 *
 * The packet is framed for the instance and transmitted with uart_poll_out().
 * Calls for the same instance must not overlap.
 *
 * @param mcumgr                The instance.
 * @param data                  Buffer containing the mcumgr packet to send.
 * @param len                   The length of the buffer, in bytes.
 *
 * @return                      0 on success; negative error code on failure.
 */
int uart_mcumgr_send(struct uart_mcumgr *mcumgr, const uint8_t *data, int len);

/**
 * @brief Frees the supplied receive buffer.
 *
 * @param mcumgr                The instance that received the buffer.
 * @param rx_buf                The buffer to free.
 */
void uart_mcumgr_free_rx_buf(struct uart_mcumgr *mcumgr, struct uart_mcumgr_rx_buf *rx_buf);

/**
 * @brief Registers an mcumgr UART receive handler.
 *
 * Configures the instance's UART and calls the specified function when an
 * mcumgr fragment is received. Call once per instance.
 *
 * @param mcumgr                The instance.
 * @param cb                    The callback to execute when an mcumgr fragment
 *                                  is received.
 * @param user_data             Pointer passed to @p cb.
 *
 * @retval 0 Success.
 * @retval -ENODEV The UART is not ready.
 * @retval <0 Error from the UART driver.
 */
int uart_mcumgr_register(struct uart_mcumgr *mcumgr, uart_mcumgr_recv_fn *cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
