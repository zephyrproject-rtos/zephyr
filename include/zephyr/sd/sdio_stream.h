/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Optional device-side SDIO streaming (packet/poll) extension
 *
 * A streaming layer on top of @ref sdio_device for high-throughput device/slave
 * functions (Wi-Fi/BT companion transports and similar). It adds per-function
 * RX/TX FIFOs, an asynchronous receive path and a @c poll() style wait, using
 * @ref net_buf packets so payloads cross into Wi-Fi/BT/USB stacks without a
 * copy, on top of the plain register-window / FIFO model of
 * @ref sdio_device_function.
 *
 * The data-path model is adapted from the NXP @c sd_dev proposal
 * (zephyrproject-rtos/zephyr#111009) but re-homed onto the SDIO
 * device subsystem: instead of a second card-centered stack it is a thin,
 * opt-in extension bound to an @ref sdio_device_function.
 *
 * Two receive sources are supported and both land in the same RX FIFO:
 *  - controllers that expose raw host accesses feed RX through the function
 *    FIFO handler installed by @ref sdio_stream_function_init (host writes);
 *  - packet-oriented controllers (e.g. real SDIO device silicon) submit
 *    received frames directly with @ref sdio_stream_rx_submit.
 */

#ifndef ZEPHYR_INCLUDE_SDIO_SDIO_STREAM_H_
#define ZEPHYR_INCLUDE_SDIO_SDIO_STREAM_H_

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sd/sdio_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDIO device streaming extension
 * @defgroup sdio_stream SDIO device streaming extension
 * @ingroup sdio_device
 * @{
 */

/** Poll: RX data available */
#define SDIO_STREAM_POLLIN  BIT(0)
/** Poll: TX space available */
#define SDIO_STREAM_POLLOUT BIT(1)

/**
 * @brief Streaming-capable device function.
 *
 * Wraps an @ref sdio_device_function with RX/TX FIFOs of @ref net_buf packets.
 * Initialize with @ref sdio_stream_function_init, then register @c base with
 * @ref sdio_device_register_function.
 */
struct sdio_stream_function {
	/** Underlying device function (register this with the device) */
	struct sdio_device_function base;
	/** @cond INTERNAL_HIDDEN */
	struct k_fifo rx_fifo;
	struct k_fifo tx_fifo;
	bool zero_copy; /* controller offers the buffer-ownership path */
	/** @endcond */
};

/**
 * @brief Allocate a stream packet from the streaming pool.
 *
 * The returned buffer is empty; fill it with @c net_buf_add() / related helpers
 * before passing it to @ref sdio_stream_write_pkt. Release an unused buffer with
 * @c net_buf_unref().
 *
 * @param timeout wait timeout if the pool is empty
 * @return buffer with one reference, or NULL on timeout/exhaustion
 */
struct net_buf *sdio_stream_alloc(k_timeout_t timeout);

/**
 * @brief Initialize a streaming function.
 *
 * Sets up @c base with a FIFO data port at @p fifo_reg and installs the
 * streaming handlers, and initializes the RX/TX FIFOs. The caller registers
 * @c base with @ref sdio_device_register_function.
 *
 * @param sf       stream function to initialize
 * @param num      function number
 * @param fifo_reg register offset mapped to the streaming data port
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_stream_function_init(struct sdio_stream_function *sf,
			      enum sdio_func_num num, uint32_t fifo_reg);

/**
 * @brief Start the streaming data path.
 *
 * Call after the function is registered and the device is enabled. If the
 * backing controller offers a zero-copy path, this posts receive buffers so
 * inbound frames land directly in pool packets; otherwise it is a no-op and
 * the synchronous FIFO-handler path is used.
 *
 * @param sf stream function
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 * @retval -ENOMEM could not post any receive buffer
 */
int sdio_stream_function_start(struct sdio_stream_function *sf);

/**
 * @brief Blocking read of one received packet.
 *
 * @param sf      stream function
 * @param timeout wait timeout
 * @return received packet (caller owns, must @c net_buf_unref()), or NULL on
 *         timeout
 */
struct net_buf *sdio_stream_read_pkt(struct sdio_stream_function *sf,
				     k_timeout_t timeout);

/**
 * @brief Blocking read into a caller buffer.
 *
 * @param sf      stream function
 * @param data    destination buffer
 * @param maxlen  buffer capacity
 * @param timeout wait timeout
 * @retval >=0 number of bytes copied
 * @retval -EAGAIN timed out
 */
int sdio_stream_read(struct sdio_stream_function *sf, uint8_t *data,
		     uint16_t maxlen, k_timeout_t timeout);

/**
 * @brief Queue data for transmission to the host.
 *
 * The data is copied into a pool buffer, queued, and the SDIO interrupt is
 * asserted towards the host. The host retrieves it by reading the function
 * data port (register-access controllers) or via the controller's push path.
 *
 * @param sf   stream function
 * @param data payload
 * @param len  payload length
 * @retval 0 on success
 * @retval -ENOMEM pool exhausted
 * @retval -EINVAL invalid argument
 */
int sdio_stream_write(struct sdio_stream_function *sf, const uint8_t *data,
		      uint16_t len);

/**
 * @brief Queue a caller-owned packet for transmission (zero-copy).
 *
 * Takes ownership of @p buf and sends it without copying its payload. On the
 * zero-copy path the packet is freed once the host has read it; on the
 * fallback path it is freed when the host drains the data port.
 *
 * @param sf  stream function
 * @param buf packet to send (from @ref sdio_stream_alloc, payload length set
 *            with @c net_buf_add())
 * @retval 0 on success (ownership transferred)
 * @retval -EINVAL invalid argument
 */
int sdio_stream_write_pkt(struct sdio_stream_function *sf, struct net_buf *buf);

/**
 * @brief Wait for stream events.
 *
 * @param sf      stream function
 * @param events  requested events (SDIO_STREAM_POLL*)
 * @param revents filled with the ready events
 * @param timeout wait timeout
 * @retval 0 on success (check @p revents)
 * @retval -EAGAIN timed out with no requested event ready
 */
int sdio_stream_poll(struct sdio_stream_function *sf, uint32_t events,
		     uint32_t *revents, k_timeout_t timeout);

/**
 * @brief Submit a received frame from a packet-oriented controller.
 *
 * Used by device controllers that deliver whole frames (rather than raw host
 * accesses) to push data into the RX FIFO. Equivalent to the frame arriving
 * through the function FIFO handler.
 *
 * @param sf   stream function
 * @param data received payload
 * @param len  payload length
 * @retval 0 on success
 * @retval -ENOMEM pool exhausted
 */
int sdio_stream_rx_submit(struct sdio_stream_function *sf, const uint8_t *data,
			  uint16_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SDIO_SDIO_STREAM_H_ */
