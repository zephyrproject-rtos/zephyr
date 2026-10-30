/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * State of an IPC link instance, internal to the backend and its transports: a transport
 * defines the state and the buffers of an instance with IPC_LINK_DEFINE() and fills its
 * ipc_link_config with IPC_LINK_CONFIG().
 */

#ifndef ZEPHYR_SUBSYS_IPC_IPC_SERVICE_BACKENDS_IPC_LINK_PRIV_H_
#define ZEPHYR_SUBSYS_IPC_IPC_SERVICE_BACKENDS_IPC_LINK_PRIV_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_link.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>

#include <ipc_link_core.h>

/* Payloads in flight: no more than the peer, configured alike, has credits for */
#define IPC_LINK_WINDOW       (CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS - 1)
#define IPC_LINK_TX_BUF_COUNT IPC_LINK_TX_BUFS(IPC_LINK_WINDOW)

struct ipc_link_ept {
	const struct ipc_ept_cfg *cfg;
	/* The peer's address of the endpoint, 0 while unbound */
	uint8_t remote_addr;
	/* Control messages to send, IPC_LINK_EPT_* */
	uint8_t ctrl;
	/* An unbound callback is due */
	bool was_bound;
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
