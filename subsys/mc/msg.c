/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/mc/msg.h>

struct mc_subscription {
	uint16_t id;
	mc_msg_callback_t callback;
	void *context;
};

struct mc_msg {
	struct mc_comm *comm;
	struct mc_msg_config config;
	struct mc_subscription *subscriptions;
	uint8_t *message;
	size_t subscription_count;
};

static size_t align_up(size_t value)
{
	return (value + (sizeof(uint64_t) - 1U)) & ~(sizeof(uint64_t) - 1U);
}

static void dispatch(struct mc_msg *instance, uint16_t id, const uint8_t *data,
			     size_t length);

size_t mc_msg_required_size(const struct mc_msg_config *config)
{
	if (config == NULL || config->message_capacity < 4U ||
	    config->subscription_capacity == 0U ||
	    config->comm.receive_capacity < 4U ||
	    config->message_capacity > config->comm.receive_capacity - 4U ||
	    mc_comm_required_size(&config->comm) == 0U) {
		return 0U;
	}
	return align_up(sizeof(struct mc_msg)) +
		align_up(mc_comm_required_size(&config->comm)) +
		align_up(config->message_capacity + 4U) +
		config->subscription_capacity * sizeof(struct mc_subscription);
}

int mc_msg_init(struct mc_msg **instance, void *storage, size_t storage_size,
		const struct mc_msg_config *config)
{
	if (instance == NULL || storage == NULL || config == NULL) {
		return -EINVAL;
	}
	size_t required = mc_msg_required_size(config);

	if (required == 0U || storage_size < required ||
	    ((uintptr_t)storage % sizeof(uint64_t)) != 0U) {
		return -ENOMEM;
	}
	struct mc_msg *msg = storage;

	msg->config = *config;
	uint8_t *cursor = (uint8_t *)storage + align_up(sizeof(*msg));
	size_t comm_size = mc_comm_required_size(&config->comm);
	int err = mc_comm_init(&msg->comm, cursor, comm_size, &config->comm);

	if (err != 0) {
		return err;
	}
	cursor += align_up(comm_size);
	msg->message = cursor;
	cursor += align_up(config->message_capacity + 4U);
	msg->subscriptions = (struct mc_subscription *)cursor;
	msg->subscription_count = 0U;
	memset(msg->subscriptions, 0,
		config->subscription_capacity * sizeof(*msg->subscriptions));
	*instance = msg;
	return 0;
}

struct mc_comm *mc_msg_comm(struct mc_msg *instance)
{
	return instance == NULL ? NULL : instance->comm;
}

int mc_msg_update(struct mc_msg *instance)
{
	if (instance == NULL) {
		return -EINVAL;
	}
	int err = mc_comm_update(instance->comm);

	if (err != 0 && err != -EAGAIN && err != -EBADMSG) {
		return err;
	}
	size_t length = 0U;

	err = mc_comm_receive(instance->comm, instance->message,
		instance->config.message_capacity + 4U, &length, 0U);
	if (err == -ETIMEDOUT) {
		return 0;
	}
	if (err != 0) {
		return err;
	}
	if (length < 4U) {
		return -EBADMSG;
	}
	uint16_t id = ((uint16_t)instance->message[0] << 8U) | instance->message[1];
	size_t payload_length = ((size_t)instance->message[2] << 8U) |
		instance->message[3];
	if (payload_length != length - 4U) {
		return -EBADMSG;
	}
	dispatch(instance, id, &instance->message[4], payload_length);
	return 0;
}

int mc_msg_subscribe(struct mc_msg *instance, uint16_t id,
			    mc_msg_callback_t callback, void *context)
{
	if (instance == NULL || callback == NULL) {
		return -EINVAL;
	}
	for (size_t i = 0U; i < instance->subscription_count; i++) {
		if (instance->subscriptions[i].id == id) {
			return -EEXIST;
		}
	}
	if (instance->subscription_count >= instance->config.subscription_capacity) {
		return -ENOSPC;
	}
	struct mc_subscription *sub =
		&instance->subscriptions[instance->subscription_count++];
	sub->id = id;
	sub->callback = callback;
	sub->context = context;
	return 0;
}

int mc_msg_unsubscribe(struct mc_msg *instance, uint16_t id)
{
	if (instance == NULL) {
		return -EINVAL;
	}
	for (size_t i = 0U; i < instance->subscription_count; i++) {
		if (instance->subscriptions[i].id == id) {
			instance->subscriptions[i] =
				instance->subscriptions[--instance->subscription_count];
			return 0;
		}
	}
	return -ENOENT;
}

static void dispatch(struct mc_msg *instance, uint16_t id, const uint8_t *data,
			     size_t length)
{
	for (size_t i = 0U; i < instance->subscription_count; i++) {
		if (instance->subscriptions[i].id == id) {
			instance->subscriptions[i].callback(instance->subscriptions[i].context,
				id, data, length);
			return;
		}
	}
}

int mc_msg_send(struct mc_msg *instance, uint16_t id, const void *data,
		       size_t length, uint32_t timeout_us)
{
	if (instance == NULL || (data == NULL && length != 0U)) {
		return -EINVAL;
	}
	if (length > instance->config.message_capacity || length > UINT16_MAX) {
		return -EMSGSIZE;
	}
	uint8_t *wire = instance->message;

	wire[0] = (uint8_t)(id >> 8U);
	wire[1] = (uint8_t)id;
	wire[2] = (uint8_t)(length >> 8U);
	wire[3] = (uint8_t)length;
	if (length > 0U) {
		memcpy(&wire[4], data, length);
	}
	return mc_comm_send(instance->comm, wire, length + 4U, timeout_us);
}

int mc_msg_signal(struct mc_msg *instance, uint16_t id, uint32_t timeout_us)
{
	return mc_msg_send(instance, id, NULL, 0U, timeout_us);
}
