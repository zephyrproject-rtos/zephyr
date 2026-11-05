/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MC_COMM_H_
#define ZEPHYR_INCLUDE_MC_COMM_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup micro_core Micro Core communication
 * @ingroup io_interfaces
 * @version 0.1.0
 * @{
 */

struct mc_comm;

/**
 * @brief Read one complete transport frame, or return a negative error code.
 *
 * @param context Application-owned transport context.
 * @param data Destination for a complete frame.
 * @param capacity Destination capacity in bytes.
 * @return Frame length, zero or -EAGAIN if no frame is ready, or another negative error code.
 */
typedef int (*mc_comm_receive_t)(void *context, uint8_t *data, size_t capacity);

/**
 * @brief Write one complete transport frame, or return a negative error code.
 *
 * @param context Application-owned transport context.
 * @param data Complete frame to transmit.
 * @param length Frame length in bytes.
 * @return Number of bytes written, or a negative error code.
 */
typedef int (*mc_comm_transmit_t)(void *context, const uint8_t *data, size_t length);

/** @brief Transport callbacks supplied by the application. */
struct mc_comm_io {
	mc_comm_receive_t receive;   /**< Read one complete transport frame. */
	mc_comm_transmit_t transmit; /**< Write one complete transport frame. */
	void *context;               /**< Transport callback context. */
};

/** @brief Frame size and number of buffered frames in one direction. */
struct mc_comm_wnd {
	size_t size;      /**< Maximum frame size, including protocol header. */
	uint8_t capacity; /**< Number of frames; zero selects one. */
};

/** @brief Communication instance configuration. */
struct mc_comm_cfg {
	struct mc_comm_io io;     /**< Transport operations and context. */
	struct mc_comm_wnd recv; /**< Incoming frame window. */
	struct mc_comm_wnd send; /**< Outgoing frame window. */
	size_t receive_capacity;  /**< Maximum complete payload size. */
	uint8_t retry_limit;     /**< Retries per frame; zero selects the default. */
};

/**
 * @brief Get the storage size required by a communication instance.
 *
 * @param config Communication configuration.
 * @return Required size in bytes, or 0 for an invalid configuration.
 */
size_t mc_comm_req_size(const struct mc_comm_cfg *config);

/**
 * @brief Initialize a communication instance in caller-owned storage.
 *
 * @param instance Output instance pointer.
 * @param storage Caller-owned storage aligned to 8 bytes. Keep it valid for the
 *                lifetime of the instance. All persistent instance state is placed here.
 * @param storage_size Available storage size in bytes.
 * @param config Communication configuration.
 * @retval 0 Instance initialized.
 * @retval -EINVAL Invalid argument or configuration.
 * @retval -ENOMEM Storage is insufficient or misaligned.
 */
int mc_comm_init(struct mc_comm **instance, void *storage, size_t storage_size,
		 const struct mc_comm_cfg *config);

/**
 * @brief Process one transport frame.
 *
 * @param instance Communication instance.
 * @retval 0 Frame processed or no frame available.
 * @retval -EINVAL Invalid instance.
 * @retval -EBADMSG Invalid frame.
 * @return Other errors returned by the transport callbacks.
 */
int mc_comm_update(struct mc_comm *instance);

/**
 * @brief Send one application payload and wait for acknowledgment.
 *
 * Payloads larger than one frame are segmented. After a timeout, delivery is
 * uncertain; retry the same payload before sending different data.
 *
 * @param instance Communication instance.
 * @param data Payload buffer, or NULL for an empty payload.
 * @param length Payload length in bytes.
 * @param timeout_us Timeout in microseconds.
 * @retval 0 Payload acknowledged.
 * @retval -EINVAL Invalid instance or payload buffer.
 * @retval -EMSGSIZE Payload length exceeds the 32-bit wire limit.
 * @retval -ETIMEDOUT Acknowledgment timed out.
 * @return Other errors returned by the transport callbacks.
 */
int mc_comm_send(struct mc_comm *instance, const void *data, size_t length, uint32_t timeout_us);

/**
 * @brief Receive one application payload.
 *
 * @param instance Communication instance.
 * @param data Output payload buffer.
 * @param capacity Output buffer capacity in bytes.
 * @param length Output payload length in bytes.
 * @param timeout_us Timeout in microseconds.
 * @retval 0 Payload received.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOSPC Output buffer is too small.
 * @retval -ETIMEDOUT No payload arrived before the timeout.
 * @return Other errors returned by the transport callbacks.
 */
int mc_comm_recv(struct mc_comm *instance, void *data, size_t capacity, size_t *length,
		    uint32_t timeout_us);

/**
 * @brief Check that no transmission is pending.
 *
 * Sending is synchronous in this implementation, so a valid idle instance
 * returns immediately.
 *
 * @param instance Communication instance.
 * @param timeout_us Reserved timeout argument; currently ignored.
 * @retval 0 No transmission is pending.
 * @retval -EINVAL Invalid instance.
 */
int mc_comm_flush(struct mc_comm *instance, uint32_t timeout_us);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
