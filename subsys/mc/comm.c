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
#define MC_DEFAULT_RETRIES   3U
#define MC_MAX_WINDOW_COUNT  16U
#define MC_MAX_FRAME_SIZE    4096U
#define MC_RETRY_INTERVAL_US 10000U
#define MC_POLL_INTERVAL_US  1000U

struct mc_tx_window {
	uint8_t *frame;
	uint32_t sequence;
	uint64_t sent_at_us;
	size_t length;
	uint16_t attempts;
	bool used;
	bool acked;
};

struct mc_rx_window {
	uint8_t *data;
	uint32_t sequence;
	size_t length;
	bool used;
};

struct mc_comm {
	struct mc_comm_config config;
	struct mc_tx_window *tx_windows;
	struct mc_rx_window *rx_windows;
	uint8_t *rx_frame;
	uint8_t *message;
	size_t send_window_size;
	size_t receive_window_size;
	uint8_t send_window_count;
	uint8_t receive_window_count;
	uint32_t tx_sequence;
	uint32_t rx_sequence;
	uint32_t expected_length;
	size_t message_length;
	uint8_t prefix[sizeof(uint32_t)];
	uint8_t prefix_length;
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

static size_t send_window_size(const struct mc_comm_config *config)
{
	return config->send_window_size != 0U ? config->send_window_size : config->frame_size;
}

static size_t receive_window_size(const struct mc_comm_config *config)
{
	return config->receive_window_size != 0U ? config->receive_window_size :
						     config->frame_size;
}

static uint8_t send_window_count(const struct mc_comm_config *config)
{
	return config->send_window_count != 0U ? config->send_window_count : 1U;
}

static uint8_t receive_window_count(const struct mc_comm_config *config)
{
	return config->receive_window_count != 0U ? config->receive_window_count : 1U;
}

static bool valid_config(const struct mc_comm_config *config)
{
	if (config == NULL || config->receive == NULL || config->transmit == NULL) {
		return false;
	}

	size_t send_size = send_window_size(config);
	size_t receive_size = receive_window_size(config);

	return config->frame_size >= MC_HEADER_SIZE + 1U &&
	       config->frame_size <= MC_MAX_FRAME_SIZE &&
	       send_size >= MC_HEADER_SIZE + 1U && send_size <= MC_MAX_FRAME_SIZE &&
	       receive_size >= MC_HEADER_SIZE + 1U && receive_size <= MC_MAX_FRAME_SIZE &&
	       send_window_count(config) <= MC_MAX_WINDOW_COUNT &&
	       receive_window_count(config) <= MC_MAX_WINDOW_COUNT &&
	       config->receive_capacity >= receive_size &&
	       config->receive_capacity <= UINT16_MAX;
}

size_t mc_comm_required_size(const struct mc_comm_config *config)
{
	if (!valid_config(config)) {
		return 0U;
	}

	size_t tx_count = send_window_count(config);
	size_t rx_count = receive_window_count(config);
	size_t tx_size = send_window_size(config);
	size_t rx_size = receive_window_size(config);

	return align_up(sizeof(struct mc_comm)) +
	       align_up(tx_count * sizeof(struct mc_tx_window)) +
	       align_up(rx_count * sizeof(struct mc_rx_window)) +
	       align_up(config->receive_capacity) + align_up(rx_size) +
	       tx_count * align_up(tx_size) +
	       rx_count * align_up(rx_size - MC_HEADER_SIZE);
}

int mc_comm_init(struct mc_comm **instance, void *storage, size_t storage_size,
		 const struct mc_comm_config *config)
{
	if (instance == NULL || storage == NULL || !valid_config(config)) {
		return -EINVAL;
	}
	if (((uintptr_t)storage % sizeof(uint64_t)) != 0U ||
	    storage_size < mc_comm_required_size(config)) {
		return -ENOMEM;
	}

	struct mc_comm *comm = storage;
	uint8_t *cursor = (uint8_t *)storage + align_up(sizeof(*comm));

	memset(comm, 0, sizeof(*comm));
	comm->config = *config;
	comm->send_window_size = send_window_size(config);
	comm->receive_window_size = receive_window_size(config);
	comm->send_window_count = send_window_count(config);
	comm->receive_window_count = receive_window_count(config);
	comm->expected_length = UINT32_MAX;
	comm->tx_windows = (struct mc_tx_window *)cursor;
	cursor += align_up(comm->send_window_count * sizeof(*comm->tx_windows));
	comm->rx_windows = (struct mc_rx_window *)cursor;
	cursor += align_up(comm->receive_window_count * sizeof(*comm->rx_windows));
	comm->message = cursor;
	cursor += align_up(config->receive_capacity);
	comm->rx_frame = cursor;
	cursor += align_up(comm->receive_window_size);

	memset(comm->tx_windows, 0, comm->send_window_count * sizeof(*comm->tx_windows));
	memset(comm->rx_windows, 0, comm->receive_window_count * sizeof(*comm->rx_windows));
	for (uint8_t i = 0U; i < comm->send_window_count; i++) {
		comm->tx_windows[i].frame = cursor;
		cursor += align_up(comm->send_window_size);
	}
	for (uint8_t i = 0U; i < comm->receive_window_count; i++) {
		comm->rx_windows[i].data = cursor;
		cursor += align_up(comm->receive_window_size - MC_HEADER_SIZE);
	}
	*instance = comm;
	return 0;
}

static size_t encode_frame(uint8_t *frame, uint8_t type, uint32_t sequence,
			   const uint8_t *payload, size_t length)
{
	sys_put_be16(MC_MAGIC, &frame[0]);
	frame[2] = type;
	frame[3] = 0U;
	sys_put_be32(sequence, &frame[4]);
	sys_put_be16((uint16_t)length, &frame[8]);
	sys_put_be16(0U, &frame[10]);
	if (length > 0U && payload != &frame[MC_HEADER_SIZE]) {
		memcpy(&frame[MC_HEADER_SIZE], payload, length);
	}
	sys_put_be16(crc16_itu_t(0xffffU, frame, MC_HEADER_SIZE + length), &frame[10]);
	return MC_HEADER_SIZE + length;
}

static int transmit_frame(struct mc_comm *comm, const uint8_t *frame, size_t length)
{
	int sent = comm->config.transmit(comm->config.context, frame, length);

	if (sent < 0) {
		return sent;
	}
	return (size_t)sent == length ? 0 : -EIO;
}

static int send_ack(struct mc_comm *comm, uint32_t sequence)
{
	uint8_t frame[MC_HEADER_SIZE];
	size_t length = encode_frame(frame, MC_TYPE_ACK, sequence, NULL, 0U);

	return transmit_frame(comm, frame, length);
}

static int append_payload(struct mc_comm *comm, const uint8_t *data, size_t length)
{
	for (size_t i = 0U; i < length; i++) {
		if (comm->prefix_length < sizeof(comm->prefix)) {
			comm->prefix[comm->prefix_length++] = data[i];
			if (comm->prefix_length == sizeof(comm->prefix)) {
				comm->expected_length = sys_get_be32(comm->prefix);
				if (comm->expected_length > comm->config.receive_capacity) {
					return -EMSGSIZE;
				}
				if (comm->expected_length == 0U) {
					comm->message_pending = true;
				}
			}
		} else {
			if (comm->message_length >= comm->expected_length) {
				return -EBADMSG;
			}
			comm->message[comm->message_length++] = data[i];
			if (comm->message_length == comm->expected_length) {
				comm->message_pending = true;
			}
		}
		if (comm->message_pending && i + 1U < length) {
			return -EBADMSG;
		}
	}
	return 0;
}

static int drain_received(struct mc_comm *comm)
{
	while (!comm->message_pending) {
		struct mc_rx_window *window =
			&comm->rx_windows[comm->rx_sequence % comm->receive_window_count];

		if (!window->used || window->sequence != comm->rx_sequence) {
			return 0;
		}
		int err = append_payload(comm, window->data, window->length);

		if (err != 0) {
			return err;
		}
		window->used = false;
		comm->rx_sequence++;
	}
	return 0;
}

static int process_data(struct mc_comm *comm, uint32_t sequence,
			const uint8_t *payload, size_t length)
{
	int32_t distance = (int32_t)(sequence - comm->rx_sequence);

	if (distance < 0) {
		return send_ack(comm, sequence);
	}
	if (distance >= comm->receive_window_count) {
		return 0;
	}

	struct mc_rx_window *window =
		&comm->rx_windows[sequence % comm->receive_window_count];

	if (!window->used) {
		memcpy(window->data, payload, length);
		window->sequence = sequence;
		window->length = length;
		window->used = true;
	}
	int err = drain_received(comm);

	if (err != 0) {
		return err;
	}
	err = send_ack(comm, sequence);

	if (err != 0 && err != -EAGAIN) {
		return err;
	}
	return 0;
}

static int process_frame(struct mc_comm *comm, uint8_t *frame, size_t length)
{
	if (length < MC_HEADER_SIZE || length > comm->receive_window_size ||
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
		struct mc_tx_window *window =
			&comm->tx_windows[sequence % comm->send_window_count];

		if (window->used && window->sequence == sequence) {
			window->acked = true;
		}
		return 0;
	}
	if (frame[2] != MC_TYPE_DATA || length == MC_HEADER_SIZE) {
		return -EBADMSG;
	}
	return process_data(comm, sequence, &frame[MC_HEADER_SIZE],
			    length - MC_HEADER_SIZE);
}

int mc_comm_update(struct mc_comm *instance)
{
	if (instance == NULL) {
		return -EINVAL;
	}
	int received = instance->config.receive(instance->config.context, instance->rx_frame,
						instance->receive_window_size);

	if (received == -EAGAIN || received == 0) {
		return 0;
	}
	if (received < 0) {
		return received;
	}
	if ((size_t)received > instance->receive_window_size) {
		return -EMSGSIZE;
	}
	return process_frame(instance, instance->rx_frame, (size_t)received);
}

static size_t copy_stream(uint8_t *destination, size_t capacity, size_t offset,
			  const uint8_t *payload, size_t payload_length)
{
	uint8_t prefix[sizeof(uint32_t)];
	size_t copied = 0U;

	sys_put_be32((uint32_t)payload_length, prefix);
	while (copied < capacity && offset + copied < sizeof(prefix) + payload_length) {
		size_t index = offset + copied;

		destination[copied] = index < sizeof(prefix) ? prefix[index] :
							 payload[index - sizeof(prefix)];
		copied++;
	}
	return copied;
}

static int send_window(struct mc_comm *comm, struct mc_tx_window *window, uint64_t now)
{
	int err = transmit_frame(comm, window->frame, window->length);

	if (err == 0) {
		window->attempts++;
		window->sent_at_us = now;
	}
	return err;
}

static void clear_tx_windows(struct mc_comm *comm)
{
	for (uint8_t i = 0U; i < comm->send_window_count; i++) {
		comm->tx_windows[i].used = false;
		comm->tx_windows[i].acked = false;
	}
}

int mc_comm_send(struct mc_comm *instance, const void *data, size_t length, uint32_t timeout_us)
{
	if (instance == NULL || (data == NULL && length != 0U)) {
		return -EINVAL;
	}
	if (length > UINT32_MAX) {
		return -EMSGSIZE;
	}

	uint64_t deadline = now_us() + timeout_us;
	uint32_t start_sequence = instance->tx_sequence;
	size_t stream_length = sizeof(uint32_t) + length;
	size_t offset = 0U;
	size_t in_flight = 0U;
	int err = 0;

	while (offset < stream_length || in_flight > 0U) {
		while (offset < stream_length && in_flight < instance->send_window_count) {
			struct mc_tx_window *window =
				&instance->tx_windows[instance->tx_sequence %
						      instance->send_window_count];

			if (window->used) {
				break;
			}
			size_t part = copy_stream(&window->frame[MC_HEADER_SIZE],
						  instance->send_window_size - MC_HEADER_SIZE,
						  offset, data, length);

			window->sequence = instance->tx_sequence++;
			window->length = encode_frame(window->frame, MC_TYPE_DATA,
						      window->sequence,
						      &window->frame[MC_HEADER_SIZE], part);
			window->attempts = 0U;
			window->sent_at_us = 0U;
			window->acked = false;
			window->used = true;
			offset += part;
			in_flight++;
			err = send_window(instance, window, now_us());
			if (err != 0 && err != -EAGAIN) {
				goto failed;
			}
		}

		err = mc_comm_update(instance);
		if (err != 0 && err != -EBADMSG && err != -EAGAIN) {
			goto failed;
		}
		for (uint8_t i = 0U; i < instance->send_window_count; i++) {
			struct mc_tx_window *window = &instance->tx_windows[i];

			if (window->used && window->acked) {
				window->used = false;
				in_flight--;
			}
		}
		if (offset == stream_length && in_flight == 0U) {
			return 0;
		}
		uint64_t now = now_us();

		if (now >= deadline) {
			err = -ETIMEDOUT;
			goto failed;
		}
		for (uint8_t i = 0U; i < instance->send_window_count; i++) {
			struct mc_tx_window *window = &instance->tx_windows[i];

			if (!window->used || window->acked ||
			    (window->attempts > 0U &&
			     now - window->sent_at_us < MC_RETRY_INTERVAL_US)) {
				continue;
			}
			if (window->attempts > instance->config.retry_limit &&
			    instance->config.retry_limit != 0U) {
				err = -ETIMEDOUT;
				goto failed;
			}
			if (window->attempts > MC_DEFAULT_RETRIES &&
			    instance->config.retry_limit == 0U) {
				err = -ETIMEDOUT;
				goto failed;
			}
			err = send_window(instance, window, now);
			if (err != 0 && err != -EAGAIN) {
				goto failed;
			}
		}
		k_usleep(MC_POLL_INTERVAL_US);
	}
	return 0;

failed:
	clear_tx_windows(instance);
	instance->tx_sequence = start_sequence;
	return err;
}

int mc_comm_receive(struct mc_comm *instance, void *data, size_t capacity, size_t *length,
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
	memcpy(data, instance->message, instance->message_length);
	*length = instance->message_length;
	instance->message_pending = false;
	instance->message_length = 0U;
	instance->expected_length = UINT32_MAX;
	instance->prefix_length = 0U;
	(void)drain_received(instance);
	return 0;
}

int mc_comm_flush(struct mc_comm *instance, uint32_t timeout_us)
{
	ARG_UNUSED(timeout_us);

	return instance == NULL ? -EINVAL : 0;
}
