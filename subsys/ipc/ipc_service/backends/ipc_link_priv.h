/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The transport interface of the IPC link backend and the state of a link instance, internal to
 * the backend and its transports. A transport defines a device with ipc_link_backend_ops as its
 * API and a config whose first member is an ipc_link_config. It defines the state and the
 * buffers of the instance with IPC_LINK_DEFINE(), fills the config with IPC_LINK_CONFIG(),
 * implements ipc_link_transport_api, and drives the link with ipc_link_pending(),
 * ipc_link_tx_prepare(), ipc_link_rx_buf() and ipc_link_rx(), which may be called from any
 * context.
 */

#ifndef ZEPHYR_SUBSYS_IPC_IPC_SERVICE_BACKENDS_IPC_LINK_PRIV_H_
#define ZEPHYR_SUBSYS_IPC_IPC_SERVICE_BACKENDS_IPC_LINK_PRIV_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_link.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/dedicated_workq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>

#include <ipc_link_core.h>

/* Payloads in flight: no more than the peer, configured alike, has credits for */
#define IPC_LINK_WINDOW       (CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS - 1)
#define IPC_LINK_TX_BUF_COUNT IPC_LINK_TX_BUFS(IPC_LINK_WINDOW)

/* The work queue the endpoint callbacks are called from: the backend's own, or the system one */
K_DEDICATED_WORK_QUEUE_DECLARE(ipc_link, IPC_SERVICE_BACKEND_LINK);

/* Events returned by ipc_link_rx() */
enum ipc_link_event {
	/* A session started */
	IPC_LINK_EVT_CONNECTED = BIT(0),
	/* The frame had no valid header */
	IPC_LINK_EVT_HDR_ERR = BIT(1),
	/* The peer runs another protocol version or frame size */
	IPC_LINK_EVT_PEER_MISMATCH = BIT(2),
};

/* Transport operations */
struct ipc_link_transport_api {
	/*
	 * Start exchanging frames, called from ipc_service_open_instance().
	 *
	 * @param dev Link instance
	 *
	 * @retval 0 on success
	 * @retval -errno on failure
	 */
	int (*open)(const struct device *dev);

	/*
	 * A frame is pending. Called from any context, including an interrupt: the transport
	 * wakes up what sends the frame and returns, without calling the link back.
	 *
	 * @param dev Link instance
	 */
	void (*kick)(const struct device *dev);
};

struct ipc_link;

/* Link configuration, the first member of the transport's device config */
struct ipc_link_config {
	const struct ipc_link_transport_api *api;
	/* Link state and buffers, from IPC_LINK_DEFINE() */
	struct ipc_link *link;
	uint8_t *tx_bufs;
	uint8_t *rx_bufs;
	/* Frame size in bytes, a multiple of IPC_LINK_MSG_ALIGN */
	uint16_t mtu;
	/* Time without acknowledgment before a payload is sent again, in milliseconds. Also the
	 * interval of the boot announcements and of the polls for credits.
	 */
	uint32_t retransmit_timeout_ms;
};

/* The IPC service backend operations of a link instance */
extern const struct ipc_service_backend ipc_link_backend_ops;

/*
 * Initialize a link instance, from the transport's device init function.
 *
 * @param dev Link instance
 *
 * @retval 0 on success
 * @retval -EINVAL if the frame size or the retransmission timeout is invalid
 */
int ipc_link_init(const struct device *dev);

/*
 * Whether a frame is to be sent.
 *
 * @param dev Link instance
 *
 * @return True if ipc_link_tx_prepare() has a frame to build
 */
bool ipc_link_pending(const struct device *dev);

/*
 * Build the next frame to send. The CRC of a payload sent for the first time is computed in the
 * caller's context, after the link is unlocked.
 *
 * The frame is valid until the next call, with one exception. An acknowledgment that arrives
 * while a payload is sent again frees its buffer, and so does the end of the session: a message
 * queued from then on may be written into the frame while the transport still reads it. Such a
 * frame is one the peer drops whatever it holds, as it has that payload already, or the session
 * it belongs to is over.
 *
 * @param dev Link instance
 * @param len Set to the frame length
 *
 * @return The frame
 */
const uint8_t *ipc_link_tx_prepare(const struct device *dev, size_t *len);

/*
 * Get the buffer the next frame is received into.
 *
 * @param dev Link instance
 *
 * @return A buffer of the frame size, valid until the next ipc_link_rx()
 */
uint8_t *ipc_link_rx_buf(const struct device *dev);

/*
 * Process the frame in the receive buffer. The CRCs are computed in the caller's context,
 * before the link is locked.
 *
 * @param dev Link instance
 * @param len Received length, at most the frame size
 * @param ok False if the transfer failed
 *
 * @return ipc_link_event flags
 */
uint32_t ipc_link_rx(const struct device *dev, size_t len, bool ok);

struct ipc_link_ept {
	const struct ipc_ept_cfg *cfg;
	/* The peer's address of the endpoint, 0 while unbound */
	uint8_t remote_addr;
	/* Control messages to send, IPC_LINK_EPT_* */
	uint8_t ctrl;
	/* Counts the endpoints that left the slot. The token of an endpoint carries it, so that
	 * what the application still holds of one that left never names the next
	 */
	uint16_t gen;
	/* An unbind released an endpoint from the slot: its unbound callback is due, and the
	 * generation it had
	 */
	const struct ipc_ept_cfg *unbound_due;
	uint16_t unbound_gen;
	/* Deregistered, until the peer acknowledges the unbind: it may still send to the address */
	bool retired;
};

struct ipc_link_rx_item {
	/* The session the frame was received in: its messages are only delivered in that session */
	uint32_t session;
	uint8_t idx;
};

struct ipc_link {
	const struct device *dev;
	struct ipc_link_core core;
	struct k_spinlock lock;
	struct ipc_link_ept epts[CONFIG_IPC_SERVICE_BACKEND_LINK_NUM_EP];
	/* Peer addresses whose unbind is to be acknowledged, one bit per address */
	ATOMIC_DEFINE(unbind_ack_due, UINT8_MAX + 1);
	/* Holds of each receive buffer, and the buffers being delivered */
	uint16_t rx_holds[CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS];
	uint8_t rx_delivering;
	/* Received frames to deliver: a buffer is queued once between its arming and its release,
	 * so the queue holds them all
	 */
	struct k_msgq rxq;
	char rxq_buf[CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS * sizeof(struct ipc_link_rx_item)];
	/* Delivers the frames and the session changes to the endpoints */
	struct k_work rx_work;
	/* Held while a frame is delivered, callbacks included, for a deregistration to wait on */
	struct k_mutex deliver_lock;
	/* A session started or ended since the last delivery */
	atomic_t session_changed;
	/* Retransmissions, polls for credits and boot announcements */
	struct k_work_delayable timer;
	bool opened;
	bool mismatch_logged;
};

/*
 * Define the state and the buffers of a link instance.
 *
 * @param name Name of the link, for IPC_LINK_CONFIG()
 * @param _mtu Frame size in bytes
 */
#define IPC_LINK_DEFINE(name, _mtu)                                                                \
	BUILD_ASSERT((_mtu) >= IPC_LINK_MTU_MIN && (_mtu) <= UINT16_MAX &&                         \
			     (_mtu) % IPC_LINK_MSG_ALIGN == 0,                                     \
		     "Invalid frame size");                                                        \
	static uint8_t name##_tx_bufs[IPC_LINK_TX_BUF_COUNT * (_mtu)]                              \
		__aligned(IPC_LINK_MSG_ALIGN);                                                     \
	static uint8_t name##_rx_bufs[CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS * (_mtu)]         \
		__aligned(IPC_LINK_MSG_ALIGN);                                                     \
	static struct ipc_link name

/*
 * Initialize the members of an ipc_link_config that IPC_LINK_DEFINE() defined.
 *
 * @param name Name given to IPC_LINK_DEFINE()
 */
#define IPC_LINK_CONFIG(name) .link = &name, .tx_bufs = name##_tx_bufs, .rx_bufs = name##_rx_bufs

#endif /* ZEPHYR_SUBSYS_IPC_IPC_SERVICE_BACKENDS_IPC_LINK_PRIV_H_ */
