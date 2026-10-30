/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/mc/msg.h>

size_t mc_msg_required_size(const struct mc_msg_config *config)
{
	(void)config;
	return 0U;
}

int mc_msg_init(struct mc_msg **instance, void *storage, size_t storage_size,
		const struct mc_msg_config *config)
{
	(void)instance;
	(void)storage;
	(void)storage_size;
	(void)config;
	return -ENOSYS;
}

struct mc_comm *mc_msg_comm(struct mc_msg *instance)
{
	(void)instance;
	return NULL;
}

int mc_msg_update(struct mc_msg *instance)
{
	(void)instance;
	return -ENOSYS;
}

int mc_msg_subscribe(struct mc_msg *instance, uint16_t id,
			    mc_msg_callback_t callback, void *context)
{
	(void)instance;
	(void)id;
	(void)callback;
	(void)context;
	return -ENOSYS;
}

int mc_msg_unsubscribe(struct mc_msg *instance, uint16_t id)
{
	(void)instance;
	(void)id;
	return -ENOSYS;
}

int mc_msg_send(struct mc_msg *instance, uint16_t id, const void *data,
		       size_t length, uint32_t timeout_us)
{
	(void)instance;
	(void)id;
	(void)data;
	(void)length;
	(void)timeout_us;
	return -ENOSYS;
}

int mc_msg_signal(struct mc_msg *instance, uint16_t id, uint32_t timeout_us)
{
	(void)instance;
	(void)id;
	(void)timeout_us;
	return -ENOSYS;
}
