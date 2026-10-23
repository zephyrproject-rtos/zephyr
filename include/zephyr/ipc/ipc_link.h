/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IPC link backend: the transport interface
 *
 * The IPC link backend runs the IPC service over any transport that moves frames between two
 * processors. A transport driver, part of the backend, defines a device with
 * @ref ipc_link_backend_ops as its API and a config whose first member is a
 * @ref ipc_link_config. It implements @ref ipc_link_transport_api, and drives the link with
 * ipc_link_pending(), ipc_link_tx_prepare(), ipc_link_rx_buf() and ipc_link_rx(), which may be
 * called from any context.
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_LINK_H_
#define ZEPHYR_INCLUDE_IPC_IPC_LINK_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC link backend transport interface
 * @defgroup ipc_link_api IPC link transport interface
 * @ingroup ipc
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Counters of a link
 *
 * Kept when CONFIG_IPC_SERVICE_LINK_STATS is enabled, read with ipc_link_stats_get() and
 * cleared with ipc_link_stats_reset().
 */
struct ipc_link_stats {
	/** Frames received */
	uint32_t rx_frames;
	/** Payloads sent for the first time */
	uint32_t tx_payloads;
	/** Payloads sent again */
	uint32_t retransmits;
	/** Payloads received in order and delivered */
	uint32_t rx_payloads;
	/** Failed transfers and headers with a bad CRC */
	uint32_t header_errors;
	/** Headers with another protocol version or frame size */
	uint32_t peer_mismatches;
	/** Payloads with a bad CRC or layout */
	uint32_t payload_errors;
	/** Valid payloads dropped, out of order or without a free receive buffer */
	uint32_t rx_dropped;
	/** Peer reboots */
	uint32_t peer_resets;
	/** Sessions ended after too many timeouts, or by an incompatible peer */
	uint32_t disconnects;
};

/** @brief Events returned by ipc_link_rx() */
enum ipc_link_event {
	/** A session started */
	IPC_LINK_EVT_CONNECTED = BIT(0),
	/** The frame had no valid header */
	IPC_LINK_EVT_HDR_ERR = BIT(1),
	/** The peer runs another protocol version or frame size */
	IPC_LINK_EVT_PEER_MISMATCH = BIT(2),
};

/** @brief Transport operations */
struct ipc_link_transport_api {
	/**
	 * @brief Start exchanging frames
	 *
	 * Called from ipc_service_open_instance().
	 *
	 * @param dev Link instance
	 *
	 * @retval 0 on success
	 * @retval -errno on failure
	 */
	int (*open)(const struct device *dev);

	/**
	 * @brief A frame is pending
	 *
	 * Called from any context, possibly with the link locked: the transport wakes up what
	 * sends the frame and returns, without calling the link back.
	 *
	 * @param dev Link instance
	 */
	void (*kick)(const struct device *dev);
};

/** @brief Link state, defined by the transport with the backend's private header */
struct ipc_link;

/** @brief Link configuration, the first member of the transport's device config */
struct ipc_link_config {
	/** Transport operations */
	const struct ipc_link_transport_api *api;
	/** Link state */
	struct ipc_link *link;
	/** Transmit buffers */
	uint8_t *tx_bufs;
	/** Receive buffers */
	uint8_t *rx_bufs;
	/** Frame size in bytes, a multiple of 4 */
	uint16_t mtu;
	/**
	 * Time without acknowledgment before a payload is sent again, in milliseconds. Also the
	 * interval of the boot announcements and of the polls for credits.
	 */
	uint32_t retransmit_timeout_ms;
};

/** @brief The IPC service backend operations of a link instance */
extern const struct ipc_service_backend ipc_link_backend_ops;

/**
 * @brief Initialize a link instance
 *
 * Called from the transport's device init function.
 *
 * @param dev Link instance
 *
 * @retval 0 on success
 * @retval -EINVAL if the frame size or the retransmission timeout is invalid
 */
int ipc_link_init(const struct device *dev);

/**
 * @brief Whether a frame is to be sent
 *
 * @param dev Link instance
 *
 * @return True if ipc_link_tx_prepare() has a frame to build
 */
bool ipc_link_pending(const struct device *dev);

/**
 * @brief Build the next frame to send
 *
 * @param dev Link instance
 * @param len Set to the frame length
 *
 * @return The frame, valid until the next call
 */
const uint8_t *ipc_link_tx_prepare(const struct device *dev, size_t *len);

/**
 * @brief Get the buffer the next frame is received into
 *
 * @param dev Link instance
 *
 * @return A buffer of the frame size, valid until the next ipc_link_rx()
 */
uint8_t *ipc_link_rx_buf(const struct device *dev);

/**
 * @brief Process the frame in the receive buffer
 *
 * The CRCs are computed in the caller's context, before the link is locked.
 *
 * @param dev Link instance
 * @param len Received length, at most the frame size
 * @param ok False if the transfer failed
 *
 * @return @ref ipc_link_event flags
 */
uint32_t ipc_link_rx(const struct device *dev, size_t len, bool ok);

/**
 * @brief Get the counters of a link instance
 *
 * @param dev Link instance
 * @param stats Filled with the counters
 *
 * @retval 0 on success
 * @retval -ENOTSUP if CONFIG_IPC_SERVICE_LINK_STATS is disabled
 */
int ipc_link_stats_get(const struct device *dev, struct ipc_link_stats *stats);

/**
 * @brief Clear the counters of a link instance
 *
 * @param dev Link instance
 *
 * @retval 0 on success
 * @retval -ENOTSUP if CONFIG_IPC_SERVICE_LINK_STATS is disabled
 */
int ipc_link_stats_reset(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_LINK_H_ */
