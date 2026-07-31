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
 * functions (Wi-Fi/BT companion transports and similar). It adds a pooled,
 * reference-counted packet abstraction, per-function RX/TX FIFOs, an
 * asynchronous receive path and a @c poll() style wait, on top of the plain
 * register-window / FIFO model of @ref sdio_device_function.
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
#include <zephyr/sys/atomic.h>
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

/** @brief Packet direction */
enum sdio_pkt_dir {
	SDIO_PKT_TX = 0, /**< device -> host */
	SDIO_PKT_RX = 1, /**< host -> device */
};

/**
 * @brief Pooled, reference-counted SDIO stream packet.
 *
 * Allocated from fixed pools (see @ref sdio_pkt_alloc). The first member is
 * reserved for @c k_fifo bookkeeping.
 */
struct sdio_pkt {
	/** @cond INTERNAL_HIDDEN */
	void *fifo_reserved;
	/** @endcond */
	/** Payload buffer (pool-owned) */
	uint8_t *data;
	/** Payload length in bytes */
	uint16_t len;
	/** Direction, see @ref sdio_pkt_dir */
	uint8_t dir;
	/** Reference count */
	atomic_t ref;
};

/**
 * @brief Streaming-capable device function.
 *
 * Wraps an @ref sdio_device_function with RX/TX FIFOs. Initialize with
 * @ref sdio_stream_function_init, then register @c base with
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
 * @brief Allocate a stream packet with an owned data buffer.
 *
 * @param dir packet direction
 * @return packet with refcount 1, or NULL if the pool is exhausted
 */
struct sdio_pkt *sdio_pkt_alloc(enum sdio_pkt_dir dir);

/**
 * @brief Take an additional reference on a packet.
 *
 * @param pkt packet
 * @return @p pkt
 */
struct sdio_pkt *sdio_pkt_ref(struct sdio_pkt *pkt);

/**
 * @brief Drop a reference; frees the packet (and buffer) on the last ref.
 *
 * @param pkt packet (NULL tolerated)
 */
void sdio_pkt_free(struct sdio_pkt *pkt);

/**
 * @brief Initialize a streaming function.
 *
 * Sets up @c base with a FIFO data port at @p fifo_reg and installs the
 * streaming FIFO handler, and initializes the RX/TX FIFOs. The caller
 * registers @c base with @ref sdio_device_register_function.
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
 * @return received packet (caller owns, must @ref sdio_pkt_free), or NULL on
 *         timeout
 */
struct sdio_pkt *sdio_stream_read_pkt(struct sdio_stream_function *sf,
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
 * @brief Queue a packet for transmission to the host.
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
 * Takes ownership of @p pkt and sends it without copying its payload. On the
 * zero-copy path the packet is freed once the host has read it; on the
 * fallback path it is freed when the host drains the data port.
 *
 * @param sf  stream function
 * @param pkt packet to send (allocated with @ref sdio_pkt_alloc, payload in
 *            @c pkt->data, length in @c pkt->len)
 * @retval 0 on success (ownership transferred)
 * @retval -EINVAL invalid argument
 */
int sdio_stream_write_pkt(struct sdio_stream_function *sf, struct sdio_pkt *pkt);

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
