/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/mc/comm.h>
#include <zephyr/mc/msg.h>

static uint8_t storage[1024] __aligned(8);
static volatile int result;

#if defined(CONFIG_MC_COMM)
static int receive_frame(void *context, uint8_t *data, size_t capacity)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(capacity);
	return -EAGAIN;
}

static int transmit_frame(void *context, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	return (int)length;
}

static const struct mc_comm_cfg comm_config = {
	.io.receive = receive_frame,
	.io.transmit = transmit_frame,
	.recv.size = 64U,
	.send.size = 64U,
	.receive_capacity = 64U,
};
#endif

#if defined(CONFIG_MC_MSG)
static void on_message(void *context, uint16_t id, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(id);
	ARG_UNUSED(data);
	ARG_UNUSED(length);
}
#endif

int main(void)
{
	storage[0] = 1U;
	result = storage[0];

#if defined(CONFIG_MC_MSG)
	struct mc_msg *msg;
	struct mc_msg_cfg config = {
		.comm = comm_config,
		.pool_size = 32U,
		.ids_capacity = 2U,
	};

	result = (int)mc_msg_req_size(&config);
	result = mc_msg_init(&msg, storage, sizeof(storage), &config);
	result = mc_msg_comm(msg) == NULL;
	result = mc_msg_subscribe(msg, 1U, on_message, NULL);
	result = mc_msg_unsubscribe(msg, 1U);
	result = mc_msg_update(msg);
	result = mc_msg_send(msg, 1U, NULL, 0U, 0U);
	result = mc_msg_signal(msg, 1U, 0U);
	result = mc_msg_flush(msg, 0U);
#elif defined(CONFIG_MC_COMM)
	struct mc_comm *comm;
	uint8_t data[64];
	size_t length;

	result = (int)mc_comm_req_size(&comm_config);
	result = mc_comm_init(&comm, storage, sizeof(storage), &comm_config);
	result = mc_comm_update(comm);
	result = mc_comm_send(comm, NULL, 0U, 0U);
	result = mc_comm_recv(comm, data, sizeof(data), &length, 0U);
	result = mc_comm_flush(comm, 0U);
#endif
	return result;
}
