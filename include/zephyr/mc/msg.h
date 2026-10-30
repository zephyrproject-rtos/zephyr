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

struct mc_msg;
typedef void (*mc_msg_callback_t)(void *context, uint16_t id,
					 const uint8_t *data, size_t length);

struct mc_msg_config {
	struct mc_comm_config comm;
	size_t message_capacity;
	size_t subscription_capacity;
};

size_t mc_msg_required_size(const struct mc_msg_config *config);

int mc_msg_init(struct mc_msg **instance, void *storage, size_t storage_size,
		const struct mc_msg_config *config);
/** Return the transport owned by an initialized message instance. */
struct mc_comm *mc_msg_comm(struct mc_msg *instance);
int mc_msg_update(struct mc_msg *instance);
int mc_msg_subscribe(struct mc_msg *instance, uint16_t id,
			    mc_msg_callback_t callback, void *context);
int mc_msg_unsubscribe(struct mc_msg *instance, uint16_t id);
int mc_msg_send(struct mc_msg *instance, uint16_t id, const void *data,
		       size_t length, uint32_t timeout_us);
int mc_msg_signal(struct mc_msg *instance, uint16_t id, uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif
