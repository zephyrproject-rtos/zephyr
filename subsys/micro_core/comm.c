/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/mc/comm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#define MC_MAGIC             0xc7e9U
#define MC_TYPE_DATA         1U
#define MC_TYPE_ACK          2U
#define MC_HEADER_SIZE       12U
#define MC_LENGTH_SIZE       sizeof(uint32_t)
#define MC_DEFAULT_RETRIES   3U
#define MC_MAX_FRAME_SIZE    4096U
#define MC_RETRY_INTERVAL_US 10000U
#define MC_POLL_INTERVAL_US  1000U

struct mc_comm {
	struct mc_comm_cfg config;
	uint8_t *tx_frame;
	uint8_t *rx_frame;
	uint8_t *message;
	size_t tx_length;
	size_t message_length;
	uint32_t tx_sequence;
	uint32_t rx_sequence;
	bool waiting_ack;
	bool acknowledged;
	bool message_pending;
};

static size_t align_up(size_t value)
{
	return (value + (sizeof(uint64_t) - 1U)) & ~(sizeof(uint64_t) - 1U);
}

static uint64_t now_us(void)
{
	return k_cyc_to_us_floor64(k_cycle_get_64());
}

static bool valid_config(const struct mc_comm_cfg *config)
{
	if (config == NULL || config->io.receive == NULL || config->io.transmit == NULL) {
		return false;
	}

	return config->send.size >= MC_HEADER_SIZE + MC_LENGTH_SIZE &&
	       config->send.size <= MC_MAX_FRAME_SIZE &&
	       config->recv.size >= MC_HEADER_SIZE + MC_LENGTH_SIZE &&
	       config->recv.size <= MC_MAX_FRAME_SIZE && config->send.capacity <= 1U &&
	       config->recv.capacity <= 1U && config->receive_capacity > 0U &&
	       config->receive_capacity <= config->recv.size - MC_HEADER_SIZE - MC_LENGTH_SIZE;
}

size_t mc_comm_req_size(const struct mc_comm_cfg *config)
{
	if (!valid_config(config)) {
		return 0U;
	}

	return align_up(sizeof(struct mc_comm)) + align_up(config->send.size) +
	       align_up(config->recv.size) + config->receive_capacity;
}

int mc_comm_init(struct mc_comm **instance, void *storage, size_t storage_size,
		 const struct mc_comm_cfg *config)
{
	if (instance == NULL || storage == NULL || !valid_config(config)) {
		return -EINVAL;
	}
	if (((uintptr_t)storage % sizeof(uint64_t)) != 0U ||
	    storage_size < mc_comm_req_size(config)) {
		return -ENOMEM;
	}

	struct mc_comm *comm = storage;
	uint8_t *cursor = (uint8_t *)storage + align_up(sizeof(*comm));

	memset(comm, 0, sizeof(*comm));
	comm->config = *config;
	comm->tx_frame = cursor;
	cursor += align_up(config->send.size);
	comm->rx_frame = cursor;
	cursor += align_up(config->recv.size);
	comm->message = cursor;
	*instance = comm;
	return 0;
}

static size_t encode_frame(uint8_t *frame, uint8_t type, uint32_t sequence, size_t length)
{
	sys_put_be16(MC_MAGIC, &frame[0]);
	frame[2] = type;
	frame[3] = 0U;
	sys_put_be32(sequence, &frame[4]);
	sys_put_be16((uint16_t)length, &frame[8]);
	sys_put_be16(0U, &frame[10]);
	sys_put_be16(crc16_itu_t(0xffffU, frame, MC_HEADER_SIZE + length), &frame[10]);
	return MC_HEADER_SIZE + length;
}

static int transmit_frame(struct mc_comm *comm, const uint8_t *frame, size_t length)
{
	int sent = comm->config.io.transmit(comm->config.io.context, frame, length);

	if (sent < 0) {
		return sent;
	}
	return (size_t)sent == length ? 0 : -EIO;
}

static int send_ack(struct mc_comm *comm, uint32_t sequence)
{
	uint8_t frame[MC_HEADER_SIZE];
	size_t length = encode_frame(frame, MC_TYPE_ACK, sequence, 0U);

	return transmit_frame(comm, frame, length);
}

static int process_data(struct mc_comm *comm, uint32_t sequence, const uint8_t *payload,
			size_t length)
{
	if (sequence == comm->rx_sequence - 1U) {
		return send_ack(comm, sequence);
	}
	if (sequence != comm->rx_sequence || comm->message_pending) {
		return 0;
	}
	if (length < MC_LENGTH_SIZE || sys_get_be32(payload) != length - MC_LENGTH_SIZE) {
		return -EBADMSG;
	}
	if (length - MC_LENGTH_SIZE > comm->config.receive_capacity) {
		return -EMSGSIZE;
	}

	comm->message_length = length - MC_LENGTH_SIZE;
	if (comm->message_length > 0U) {
		memcpy(comm->message, &payload[MC_LENGTH_SIZE], comm->message_length);
	}
	comm->message_pending = true;
	comm->rx_sequence++;
	return send_ack(comm, sequence);
}

static int process_frame(struct mc_comm *comm, uint8_t *frame, size_t length)
{
	if (length < MC_HEADER_SIZE || length > comm->config.recv.size ||
	    sys_get_be16(&frame[0]) != MC_MAGIC || frame[3] != 0U ||
	    sys_get_be16(&frame[8]) != length - MC_HEADER_SIZE) {
		return -EBADMSG;
	}

	uint16_t received_crc = sys_get_be16(&frame[10]);

	sys_put_be16(0U, &frame[10]);
	if (crc16_itu_t(0xffffU, frame, length) != received_crc) {
		return -EBADMSG;
	}

	uint32_t sequence = sys_get_be32(&frame[4]);

	if (frame[2] == MC_TYPE_ACK) {
		if (length != MC_HEADER_SIZE) {
			return -EBADMSG;
		}
		if (comm->waiting_ack && sequence == comm->tx_sequence) {
			comm->acknowledged = true;
		}
		return 0;
	}
	if (frame[2] != MC_TYPE_DATA) {
		return -EBADMSG;
	}
	return process_data(comm, sequence, &frame[MC_HEADER_SIZE], length - MC_HEADER_SIZE);
}

int mc_comm_update(struct mc_comm *instance)
{
	if (instance == NULL) {
		return -EINVAL;
	}
	int received = instance->config.io.receive(instance->config.io.context, instance->rx_frame,
						   instance->config.recv.size);

	if (received == -EAGAIN || received == 0) {
		return 0;
	}
	if (received < 0) {
		return received;
	}
	if ((size_t)received > instance->config.recv.size) {
		return -EMSGSIZE;
	}
	return process_frame(instance, instance->rx_frame, (size_t)received);
}

int mc_comm_send(struct mc_comm *instance, const void *data, size_t length, uint32_t timeout_us)
{
	if (instance == NULL || (data == NULL && length != 0U)) {
		return -EINVAL;
	}
	if (length > instance->config.send.size - MC_HEADER_SIZE - MC_LENGTH_SIZE) {
		return -EMSGSIZE;
	}
	if (instance->waiting_ack) {
		if (instance->tx_length != MC_HEADER_SIZE + MC_LENGTH_SIZE + length ||
		    (length > 0U && memcmp(&instance->tx_frame[MC_HEADER_SIZE + MC_LENGTH_SIZE],
					   data, length) != 0)) {
			return -EBUSY;
		}
	} else {
		sys_put_be32((uint32_t)length, &instance->tx_frame[MC_HEADER_SIZE]);
		if (length > 0U) {
			memcpy(&instance->tx_frame[MC_HEADER_SIZE + MC_LENGTH_SIZE], data, length);
		}
		instance->tx_length = encode_frame(instance->tx_frame, MC_TYPE_DATA,
						   instance->tx_sequence, MC_LENGTH_SIZE + length);
		instance->waiting_ack = true;
		instance->acknowledged = false;
	}

	uint64_t deadline = now_us() + timeout_us;
	uint64_t next_transmit = 0U;
	uint16_t attempts = 0U;
	uint16_t attempt_limit =
		(instance->config.retry_limit == 0U ? MC_DEFAULT_RETRIES
						    : instance->config.retry_limit) +
		1U;

	for (;;) {
		if (instance->acknowledged) {
			instance->waiting_ack = false;
			instance->tx_sequence++;
			return 0;
		}
		uint64_t now = now_us();

		if (next_transmit == 0U || now >= next_transmit) {
			if (attempts >= attempt_limit || (attempts > 0U && now >= deadline)) {
				return -ETIMEDOUT;
			}
			int err = transmit_frame(instance, instance->tx_frame, instance->tx_length);

			if (err != 0 && err != -EAGAIN) {
				return err;
			}
			if (err == 0) {
				attempts++;
			}
			next_transmit = now_us() + MC_RETRY_INTERVAL_US;
		}
		int err = mc_comm_update(instance);

		if (err != 0 && err != -EBADMSG && err != -EAGAIN) {
			return err;
		}
		if (now_us() >= deadline && !instance->acknowledged) {
			return -ETIMEDOUT;
		}
		k_usleep(MC_POLL_INTERVAL_US);
	}
}

int mc_comm_recv(struct mc_comm *instance, void *data, size_t capacity, size_t *length,
		 uint32_t timeout_us)
{
	if (instance == NULL || data == NULL || length == NULL) {
		return -EINVAL;
	}
	uint64_t deadline = now_us() + timeout_us;

	while (!instance->message_pending) {
		int err = mc_comm_update(instance);

		if (err != 0 && err != -EBADMSG && err != -EAGAIN) {
			return err;
		}
		if (!instance->message_pending && now_us() >= deadline) {
			return -ETIMEDOUT;
		}
		k_usleep(MC_POLL_INTERVAL_US);
	}
	if (capacity < instance->message_length) {
		return -ENOSPC;
	}
	if (instance->message_length > 0U) {
		memcpy(data, instance->message, instance->message_length);
	}
	*length = instance->message_length;
	instance->message_pending = false;
	instance->message_length = 0U;
	return 0;
}
