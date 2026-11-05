/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MC_MSG_H_
#define ZEPHYR_INCLUDE_MC_MSG_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/mc/comm.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup micro_core
 * @{
 */

struct mc_msg;

/**
 * @brief Receive a complete message. Data is valid only during the callback.
 *
 * @param context Application-owned subscription context.
 * @param id Received message ID.
 * @param data Message payload in instance storage.
 * @param length Payload length in bytes.
 */
typedef void (*mc_msg_callback_t)(void *context, uint16_t id,
				  const uint8_t *data, size_t length);

/** @brief Message-layer configuration. */
struct mc_msg_cfg {
	struct mc_comm_cfg comm; /**< Transport and communication configuration. */
	size_t pool_size;       /**< Maximum message payload size in bytes. */
	size_t ids_capacity;    /**< Maximum number of subscribed message IDs. */
};

/**
 * @brief Get the storage size required by a message instance.
 *
 * @param config Message configuration.
 * @return Required size in bytes, or 0 for an invalid configuration.
 */
size_t mc_msg_req_size(const struct mc_msg_cfg *config);

/**
 * @brief Initialize a message instance in caller-owned storage.
 *
 * @param instance Output instance pointer.
 * @param storage Storage aligned to 8 bytes and valid for the instance lifetime.
 * @param storage_size Available storage size in bytes.
 * @param config Message configuration.
 * @retval 0 Instance initialized.
 * @retval -EINVAL Invalid argument.
 * @retval -ENOMEM Storage is insufficient, misaligned, or configuration is invalid.
 */
int mc_msg_init(struct mc_msg **instance, void *storage, size_t storage_size,
		const struct mc_msg_cfg *config);

/**
 * @brief Get the communication instance owned by a message instance.
 *
 * @param instance Message instance, or NULL.
 * @return Communication instance, or NULL for a NULL argument.
 */
struct mc_comm *mc_msg_comm(struct mc_msg *instance);

/**
 * @brief Process received frames and dispatch one complete message.
 *
 * @param instance Message instance.
 * @retval 0 Message dispatched or no message available.
 * @retval -EINVAL Invalid instance.
 * @retval -EBADMSG Invalid message.
 * @return Other errors returned by the communication layer.
 */
int mc_msg_update(struct mc_msg *instance);

/**
 * @brief Subscribe to a message ID.
 *
 * @param instance Message instance.
 * @param id Message ID.
 * @param callback Function called when a complete message with this ID arrives.
 * @param context Application-owned callback context.
 * @retval 0 Subscription added.
 * @retval -EINVAL Invalid instance or callback.
 * @retval -EEXIST ID already subscribed.
 * @retval -ENOSPC Subscription table is full.
 */
int mc_msg_subscribe(struct mc_msg *instance, uint16_t id,
		    mc_msg_callback_t callback, void *context);

/**
 * @brief Remove a message ID subscription.
 *
 * @param instance Message instance.
 * @param id Message ID.
 * @retval 0 Subscription removed.
 * @retval -EINVAL Invalid instance.
 * @retval -ENOENT ID was not subscribed.
 */
int mc_msg_unsubscribe(struct mc_msg *instance, uint16_t id);

/**
 * @brief Send a message with a 16-bit ID.
 *
 * @param instance Message instance.
 * @param id Message ID.
 * @param data Payload, or NULL for a zero-length payload.
 * @param length Payload length in bytes.
 * @param timeout_us Timeout in microseconds.
 * @retval 0 Message acknowledged.
 * @retval -EINVAL Invalid instance or payload pointer.
 * @retval -EMSGSIZE Payload exceeds the configured or wire capacity.
 * @return Other errors returned by the communication layer.
 */
int mc_msg_send(struct mc_msg *instance, uint16_t id, const void *data,
		 size_t length, uint32_t timeout_us);

/**
 * @brief Send a message ID with no payload.
 *
 * @param instance Message instance.
 * @param id Message ID.
 * @param timeout_us Timeout in microseconds.
 * @retval 0 Signal acknowledged.
 * @return Errors returned by mc_msg_send().
 */
int mc_msg_signal(struct mc_msg *instance, uint16_t id, uint32_t timeout_us);

/**
 * @brief Wait for pending communication to finish.
 *
 * Sending is synchronous in this implementation, so a valid idle instance
 * returns immediately.
 *
 * @param instance Message instance.
 * @param timeout_us Reserved timeout argument; currently ignored.
 * @retval 0 No transmission is pending.
 * @retval -EINVAL Invalid instance.
 */
int mc_msg_flush(struct mc_msg *instance, uint32_t timeout_us);

/** @} */

#ifdef __cplusplus
}
#endif

#endif
