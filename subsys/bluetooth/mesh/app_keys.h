/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_MESH_APP_KEYS_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_MESH_APP_KEYS_H_

#include <zephyr/bluetooth/mesh.h>
#include "subnet.h"

/** @brief Reset the app keys module. */
void bt_mesh_app_keys_reset(void);

/** @brief Initialize a new application key with the given parameters.
 *
 *  @param app_idx AppIndex.
 *  @param net_idx NetIndex the application is bound to.
 *  @param old_key Current application key.
 *  @param new_key Updated application key, or NULL if not known.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_app_key_set(uint16_t app_idx, uint16_t net_idx,
			const uint8_t old_key[16], const uint8_t new_key[16]);

/** @brief Resolve the message encryption keys, given a message context.
 *
 *  Will use the @c ctx::app_idx and @c ctx::net_idx fields to find a pair of
 *  message encryption keys. If @c ctx::app_idx represents a device key, the
 *  @c ctx::net_idx will be used to determine the net key. Otherwise, the
 *  @c ctx::net_idx parameter will be ignored.
 *
 *  @param ctx     Message context.
 *  @param sub     Subnet return parameter.
 *  @param app_key Application return parameter.
 *  @param aid     Application ID return parameter.
 *
 *  @return 0 on success, or (negative) error code on failure.
 */
int bt_mesh_keys_resolve(struct bt_mesh_msg_ctx *ctx,
			 struct bt_mesh_subnet **sub,
			 const uint8_t **app_key, uint8_t *aid);

/** @brief Iterate through all matching application keys and call @c cb on each.
 *
 *  @param dev_key Whether to return device keys.
 *  @param aid     7 bit application ID to match.
 *  @param rx      RX structure to match against.
 *  @param cb      Callback to call for every valid app key.
 *  @param cb_data Callback data to pass to the callback.
 *
 *  @return The AppIdx that yielded a 0-return from the callback.
 */
uint16_t bt_mesh_app_key_find(bool dev_key, uint8_t aid,
			      struct bt_mesh_net_rx *rx,
			      int (*cb)(struct bt_mesh_net_rx *rx,
					const uint8_t key[16], void *cb_data),
			      void *cb_data);

/** @brief Store pending application keys in persistent storage. */
void bt_mesh_app_key_pending_store(void);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_MESH_APP_KEYS_H_ */
