/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/mc/comm.h>

#define MC_MAGIC           0xc7e9U
#define MC_TYPE_DATA       1U
#define MC_TYPE_ACK        2U
#define MC_HEADER_SIZE     8U
#define MC_DEFAULT_RETRIES 3U
#define MC_RETRY_US        10000U

struct mc_comm {
	struct mc_comm_config config;
	uint8_t *tx;
	uint8_t *rx;
	size_t frame_size;
	size_t rx_length;
	bool rx_pending;
	uint8_t tx_sequence;
	uint8_t rx_sequence;
	uint8_t retry_limit;
	bool waiting_ack;
	uint8_t waiting_sequence;
};

static size_t align_up(size_t value)
{
	return (value + (sizeof(uintptr_t) - 1U)) & ~(sizeof(uintptr_t) - 1U);
}

static uint16_t crc16(const uint8_t *data, size_t length)
{
	uint16_t crc = 0xffffU;

	for (size_t i = 0U; i < length; i++) {
		crc ^= (uint16_t)data[i] << 8U;
		for (uint8_t bit = 0U; bit < 8U; bit++) {
			crc = (crc & 0x8000U) != 0U ? (uint16_t)((crc << 1U) ^ 0x1021U)
						    : (uint16_t)(crc << 1U);
		}
	}

	return crc;
}

static bool valid_config(const struct mc_comm_config *config)
{
	return config != NULL && config->receive != NULL && config->transmit != NULL &&
	       config->frame_size >= MC_HEADER_SIZE + 1U && config->frame_size <= 1024U &&
	       config->receive_capacity >= config->frame_size &&
	       config->receive_capacity <= UINT16_MAX;
}

size_t mc_comm_required_size(const struct mc_comm_config *config)
{
	if (!valid_config(config)) {
		return 0U;
	}

	return align_up(sizeof(struct mc_comm)) + align_up(config->frame_size) +
	       config->receive_capacity;
}

int mc_comm_init(struct mc_comm **instance, void *storage, size_t storage_size,
		 const struct mc_comm_config *config)
{
	if (instance == NULL || storage == NULL || !valid_config(config)) {
		return -EINVAL;
	}
	if (((uintptr_t)storage % sizeof(uintptr_t)) != 0U ||
	    storage_size < mc_comm_required_size(config)) {
		return -ENOMEM;
	}

	struct mc_comm *comm = storage;

	comm->config = *config;
	comm->frame_size = config->frame_size;
	comm->retry_limit = config->retry_limit == 0U ? MC_DEFAULT_RETRIES : config->retry_limit;
	comm->tx = (uint8_t *)storage + align_up(sizeof(*comm));
	comm->rx = comm->tx + align_up(config->frame_size);
	comm->rx_length = 0U;
	comm->rx_pending = false;
	comm->tx_sequence = 0U;
	comm->rx_sequence = 0U;
	comm->waiting_ack = false;
	memset(comm->tx, 0, config->frame_size);
	memset(comm->rx, 0, config->receive_capacity);
	*instance = comm;

	return 0;
}

static int send_frame(struct mc_comm *comm, uint8_t type, uint8_t sequence, const uint8_t *payload,
		      size_t length)
{
	if (length > comm->frame_size - MC_HEADER_SIZE || length > UINT16_MAX) {
		return -EMSGSIZE;
	}

	sys_put_be16(MC_MAGIC, &comm->tx[0]);
	comm->tx[2] = type;
	comm->tx[3] = sequence;
	sys_put_be16((uint16_t)length, &comm->tx[4]);
	memset(&comm->tx[6], 0, 2U);
	if (length > 0U) {
		memcpy(&comm->tx[MC_HEADER_SIZE], payload, length);
	}
	uint16_t crc = crc16(comm->tx, MC_HEADER_SIZE + length);

	sys_put_be16(crc, &comm->tx[6]);

	int sent = comm->config.transmit(comm->config.context, comm->tx, MC_HEADER_SIZE + length);

	return sent < 0 ? sent : (sent == (int)(MC_HEADER_SIZE + length) ? 0 : -EIO);
}

static int process_frame(struct mc_comm *comm, const uint8_t *frame, size_t length)
{
	if (length < MC_HEADER_SIZE || sys_get_be16(&frame[0]) != MC_MAGIC ||
	    (length - MC_HEADER_SIZE) != sys_get_be16(&frame[4])) {
		return -EBADMSG;
	}
	uint16_t received_crc = sys_get_be16(&frame[6]);
	uint16_t calculated_crc = 0xffffU;

	for (size_t i = 0U; i < length; i++) {
		uint8_t byte = (i == 6U || i == 7U) ? 0U : frame[i];

		calculated_crc ^= (uint16_t)byte << 8U;
		for (uint8_t bit = 0U; bit < 8U; bit++) {
			calculated_crc = (calculated_crc & 0x8000U) != 0U
						 ? (uint16_t)((calculated_crc << 1U) ^ 0x1021U)
						 : (uint16_t)(calculated_crc << 1U);
		}
	}
	if (calculated_crc != received_crc) {
		return -EBADMSG;
	}

	uint8_t type = frame[2];
	uint8_t sequence = frame[3];
	uint16_t payload_length = sys_get_be16(&frame[4]);

	if (type == MC_TYPE_ACK) {
		if (comm->waiting_ack && sequence == comm->waiting_sequence) {
			comm->waiting_ack = false;
		}
		return 0;
	}
	if (type != MC_TYPE_DATA) {
		return -EBADMSG;
	}
	if (sequence == comm->rx_sequence && payload_length <= comm->config.receive_capacity) {
		memmove(comm->rx, &frame[MC_HEADER_SIZE], payload_length);
		comm->rx_length = payload_length;
		comm->rx_pending = true;
		comm->rx_sequence++;
	}
	return send_frame(comm, MC_TYPE_ACK, sequence, NULL, 0U);
}

int mc_comm_update(struct mc_comm *instance)
{
	if (instance == NULL) {
		return -EINVAL;
	}
	int received = instance->config.receive(instance->config.context, instance->rx,
						instance->config.receive_capacity);
	if (received < 0) {
		return received == -EAGAIN ? 0 : received;
	}
	if (received == 0) {
		return 0;
	}
	return process_frame(instance, instance->rx, (size_t)received);
}

int mc_comm_send(struct mc_comm *instance, const void *data, size_t length, uint32_t timeout_us)
{
	if (instance == NULL || (data == NULL && length != 0U)) {
		return -EINVAL;
	}
	if (length > instance->frame_size - MC_HEADER_SIZE) {
		return -EMSGSIZE;
	}
	int64_t deadline = k_uptime_get() + DIV_ROUND_UP(timeout_us, 1000U);
	uint8_t sequence = instance->tx_sequence++;

	instance->waiting_sequence = sequence;
	instance->waiting_ack = true;
	for (uint8_t attempt = 0U; attempt <= instance->retry_limit; attempt++) {
		int err = send_frame(instance, MC_TYPE_DATA, sequence, data, length);

		if (err != 0) {
			return err;
		}
		while (instance->waiting_ack) {
			(void)mc_comm_update(instance);
			if (k_uptime_get() >= deadline) {
				if (attempt == instance->retry_limit) {
					instance->waiting_ack = false;
					return -ETIMEDOUT;
				}
				break;
			}
			k_usleep(MC_RETRY_US);
		}
		if (!instance->waiting_ack) {
			return 0;
		}
	}
	return -ETIMEDOUT;
}

int mc_comm_receive(struct mc_comm *instance, void *data, size_t capacity, size_t *length,
		    uint32_t timeout_us)
{
	if (instance == NULL || data == NULL || length == NULL) {
		return -EINVAL;
	}
	int64_t deadline = k_uptime_get() + DIV_ROUND_UP(timeout_us, 1000U);

	while (!instance->rx_pending) {
		(void)mc_comm_update(instance);
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
		k_usleep(MC_RETRY_US);
	}
	if (capacity < instance->rx_length) {
		return -ENOSPC;
	}
	memcpy(data, instance->rx, instance->rx_length);
	*length = instance->rx_length;
	instance->rx_length = 0U;
	instance->rx_pending = false;
	return 0;
}

int mc_comm_flush(struct mc_comm *instance, uint32_t timeout_us)
{
	if (instance == NULL) {
		return -EINVAL;
	}
	int64_t deadline = k_uptime_get() + DIV_ROUND_UP(timeout_us, 1000U);

	while (instance->waiting_ack) {
		(void)mc_comm_update(instance);
		if (k_uptime_get() >= deadline) {
			return -ETIMEDOUT;
		}
	}
	return 0;
}
