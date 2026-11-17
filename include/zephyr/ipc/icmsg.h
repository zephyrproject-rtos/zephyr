/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Inter-core messaging (icmsg) library.
 * @ingroup ipc_icmsg_api
 */

#ifndef ZEPHYR_INCLUDE_IPC_ICMSG_H_
#define ZEPHYR_INCLUDE_IPC_ICMSG_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/ipc/pbuf.h>
#include <zephyr/sys/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Icmsg IPC library API
 * @defgroup ipc_icmsg_api Icmsg IPC library API
 * @ingroup ipc
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/** @brief State of an icmsg instance. */
enum icmsg_state {
	/** Instance is not initialized yet. In this state: sending will fail, opening allowed.
	 */
	ICMSG_STATE_OFF,

	/** Instance is initializing without session handshake. In this state: sending will fail,
	 * opening will fail.
	 */
	ICMSG_STATE_INITIALIZING_SID_DISABLED,

	/** Instance is initializing with session handshake. It is waiting for remote to acknowledge
	 * local session id. In this state: sending will fail, opening is allowed (local session id
	 * will change, so the remote may get unbound() callback).
	 */
	ICMSG_STATE_INITIALIZING_SID_ENABLED,

	/** Instance is initializing with detection of session handshake support on remote side.
	 * It is waiting for remote to acknowledge local session id or to send magic bytes.
	 * In this state: sending will fail, opening is allowed (local session id
	 * will change, so the remote may get unbound() callback if it supports it).
	 */
	ICMSG_STATE_INITIALIZING_SID_DETECT,

	/** Instance was closed on remote side. The unbound() callback was send on local side.
	 * In this state: sending will be silently discarded (there may be outdated sends),
	 * opening is allowed.
	 */
	ICMSG_STATE_DISCONNECTED,

	/* Connected states must be at the end. */

	/** Instance is connected without session handshake support. In this state: sending will be
	 * successful, opening will fail.
	 */
	ICMSG_STATE_CONNECTED_SID_DISABLED,

	/** Instance is connected with session handshake support. In this state: sending will be
	 * successful, opening is allowed (session will change and remote will get unbound()
	 * callback).
	 */
	ICMSG_STATE_CONNECTED_SID_ENABLED,
};

/** @endcond */

/**
 * @brief Session handshake mode of an icmsg instance.
 *
 * Selects whether the instance exchanges session identifiers with the remote, which is what
 * allows the ipc_service_cb::unbound callback to be called. Matches the @c unbound property of
 * @dtcompatible{zephyr\,ipc-icmsg}. Each mode can be compiled out with the matching
 * @kconfig_regex{CONFIG_IPC_SERVICE_ICMSG_UNBOUND_.*_ALLOWED} option.
 */
enum icmsg_unbound_mode {
	/**
	 * No session handshake: the unbound callback is never called. The remote must use
	 * @ref ICMSG_UNBOUND_MODE_DISABLE or @ref ICMSG_UNBOUND_MODE_DETECT.
	 */
	ICMSG_UNBOUND_MODE_DISABLE = ICMSG_STATE_INITIALIZING_SID_DISABLED,
	/**
	 * Session handshake: the unbound callback is called when the remote closes or restarts
	 * its instance. The remote must use @ref ICMSG_UNBOUND_MODE_ENABLE or
	 * @ref ICMSG_UNBOUND_MODE_DETECT.
	 */
	ICMSG_UNBOUND_MODE_ENABLE = ICMSG_STATE_INITIALIZING_SID_ENABLED,
	/**
	 * Session handshake if the remote supports it, no handshake otherwise. Both packet buffers
	 * must use a data cache alignment of at least 8 bytes.
	 */
	ICMSG_UNBOUND_MODE_DETECT = ICMSG_STATE_INITIALIZING_SID_DETECT,
};

/** @brief Configuration of an icmsg instance. */
struct icmsg_config_t {
	/** MBOX channel used to signal the remote. */
	struct mbox_dt_spec mbox_tx;
	/** MBOX channel on which the remote signals this instance. */
	struct mbox_dt_spec mbox_rx;
	/** Session handshake mode. */
	enum icmsg_unbound_mode unbound_mode;
};

/**
 * @brief Run-time data of an icmsg instance.
 *
 * Set @c tx_pb and @c rx_pb before calling icmsg_open() and zero-initialize the other members,
 * which are managed by the library. The structure must stay valid while the instance is open.
 */
struct icmsg_data_t {
	/** Packet buffer used to send data to the remote, as defined with PBUF_DEFINE(). */
	struct pbuf *tx_pb;
	/** Packet buffer used to receive data from the remote, as defined with PBUF_DEFINE(). */
	struct pbuf *rx_pb;

	/** @cond INTERNAL_HIDDEN */
#ifdef CONFIG_IPC_SERVICE_ICMSG_SHMEM_ACCESS_SYNC
	struct k_mutex tx_lock;
#endif

	/* Callbacks for an endpoint. */
	const struct ipc_service_cb *cb;
	void *ctx;

	/* General */
	const struct icmsg_config_t *cfg;
#ifdef CONFIG_MULTITHREADING
	struct k_work mbox_work;
#endif
	uint16_t remote_sid;
	uint16_t local_sid;
	atomic_t state;
	/** @endcond */
};

/** @brief Open an icmsg instance
 *
 *  Open an icmsg instance to be able to send and receive messages to a remote
 *  instance.
 *  This function is blocking until the handshake with the remote instance is
 *  completed.
 *  This function is intended to be called late in the initialization process,
 *  possibly from a thread which can be safely blocked while handshake with the
 *  remote instance is being performed.
 *
 *  @param[in] conf Structure containing configuration parameters for the icmsg
 *                  instance.
 *  @param[inout] dev_data Structure containing run-time data used by the icmsg
 *                         instance.
 *  @param[in] cb Structure containing callback functions to be called on
 *                events generated by this icmsg instance. The pointed memory
 *                must be preserved while the icmsg instance is active.
 *  @param[in] ctx Pointer to context passed as an argument to callbacks.
 *
 *
 *  @retval 0 on success.
 *  @retval -EALREADY when the instance is already opened.
 *  @retval -errno Other negative errno code from dependent modules.
 */
int icmsg_open(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data,
	       const struct ipc_service_cb *cb, void *ctx);

/** @brief Close an icmsg instance
 *
 *  Closing an icmsg instance results in releasing all resources used by given
 *  instance including the shared memory regions and mbox devices.
 *
 *  @param[in] conf Structure containing configuration parameters for the icmsg
 *                  instance being closed. Its content must be the same as used
 *                  for creating this instance with @ref icmsg_open.
 *  @param[inout] dev_data Structure containing run-time data used by the icmsg
 *                         instance.
 *
 *  @retval 0 on success.
 *  @retval -errno Other negative errno code from dependent modules.
 */
int icmsg_close(const struct icmsg_config_t *conf,
		struct icmsg_data_t *dev_data);

/** @brief Send a message to the remote icmsg instance.
 *
 *  @param[in] conf Structure containing configuration parameters for the icmsg
 *                  instance.
 *  @param[inout] dev_data Structure containing run-time data used by the icmsg
 *                         instance.
 *  @param[in] msg Pointer to a buffer containing data to send.
 *  @param[in] len Size of data in the @p msg buffer.
 *
 *
 *  @return Number of sent bytes on success.
 *  @retval -EBUSY when the instance has not finished handshake with the remote
 *                 instance.
 *  @retval -ENODATA when the requested data to send is empty.
 *  @retval -EBADMSG when the requested data to send is too big.
 *  @retval -ENOBUFS when there are no TX buffers available.
 *  @retval -errno Other negative errno code from dependent modules.
 */
int icmsg_send(const struct icmsg_config_t *conf,
	       struct icmsg_data_t *dev_data,
	       const void *msg, size_t len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_ICMSG_H_ */
