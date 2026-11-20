/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/mc/comm.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

#define TEST_HEADER_SIZE 12U
#define TEST_MAGIC 0xc7e9U

struct test_link {
	uint8_t data[128];
	size_t length;
	bool full;
	bool corrupt;
	bool drop;
	uint8_t drop_count;
	uint32_t transmit_count;
	uint32_t dropped_count;
	int receive_error;
	int transmit_error;
	bool fail_retransmit;
	bool short_transmit;
	bool oversize_receive;
	struct mc_comm *peer;
};

struct test_endpoint {
	struct test_link *receive_link;
	struct test_link *transmit_link;
};

static int receive(void *context, uint8_t *data, size_t capacity)
{
	struct test_endpoint *endpoint = context;
	struct test_link *link = endpoint->receive_link;

	if (link->receive_error != 0) {
		return link->receive_error;
	}
	if (link->oversize_receive) {
		return (int)capacity + 1;
	}
	if (!link->full) {
		return -EAGAIN;
	}
	if (capacity < link->length) {
		return -ENOSPC;
	}
	memcpy(data, link->data, link->length);
	link->full = false;
	return (int)link->length;
}

static int transmit(void *context, const uint8_t *data, size_t length)
{
	struct test_endpoint *endpoint = context;
	struct test_link *link = endpoint->transmit_link;

	if (link->transmit_error != 0) {
		return link->transmit_error;
	}
	if (link->fail_retransmit && link->transmit_count > 0U) {
		return -EIO;
	}
	if (link->short_transmit) {
		return (int)length - 1;
	}
	if (link->full || length > sizeof(link->data)) {
		return -EAGAIN;
	}
	link->transmit_count++;
	memcpy(link->data, data, length);
	if (link->corrupt && length > 0U) {
		link->data[length - 1U] ^= 0x01U;
	}
	if (link->drop || link->drop_count > 0U) {
		if (link->drop_count > 0U) {
			link->drop_count--;
		}
		link->dropped_count++;
		return (int)length;
	}
	link->length = length;
	link->full = true;
	if (link->peer != NULL) {
		(void)mc_comm_update(link->peer);
	}
	return (int)length;
}

static const struct mc_comm_cfg valid_config = {
	.io.receive = receive,
	.io.transmit = transmit,
	.recv.size = 32U,
	.send.size = 32U,
	.receive_capacity = 32U,
};

static void set_frame(struct test_link *link, uint8_t type, uint32_t sequence,
		      const uint8_t *payload, size_t length)
{
	zassert_true(TEST_HEADER_SIZE + length <= sizeof(link->data), NULL);
	sys_put_be16(TEST_MAGIC, &link->data[0]);
	link->data[2] = type;
	link->data[3] = 0U;
	sys_put_be32(sequence, &link->data[4]);
	sys_put_be16((uint16_t)length, &link->data[8]);
	sys_put_be16(0U, &link->data[10]);
	if (length > 0U) {
		memcpy(&link->data[TEST_HEADER_SIZE], payload, length);
	}
	link->length = TEST_HEADER_SIZE + length;
	sys_put_be16(crc16_itu_t(0xffffU, link->data, link->length), &link->data[10]);
	link->full = true;
}

static void init_pair(struct mc_comm **a, struct mc_comm **b, struct test_link *a_to_b,
		      struct test_link *b_to_a, struct test_endpoint *a_endpoint,
		      struct test_endpoint *b_endpoint, uint8_t *a_storage, uint8_t *b_storage)
{
	struct mc_comm_cfg config = valid_config;

	a_endpoint->receive_link = b_to_a;
	a_endpoint->transmit_link = a_to_b;
	b_endpoint->receive_link = a_to_b;
	b_endpoint->transmit_link = b_to_a;

	config.io.context = a_endpoint;
	zassert_ok(mc_comm_init(a, a_storage, 512U, &config), NULL);
	config.io.context = b_endpoint;
	zassert_ok(mc_comm_init(b, b_storage, 512U, &config), NULL);
	a_to_b->peer = *b;
	b_to_a->peer = *a;
}

ZTEST(mc_comm, test_invalid_configuration)
{
	struct mc_comm_cfg config = {0};
	struct mc_comm *instance;
	uint8_t storage[512] __aligned(8);

	zassert_equal(mc_comm_req_size(NULL), 0U, NULL);
	zassert_equal(mc_comm_req_size(&config), 0U, NULL);
	config = valid_config;
	config.recv.size = 8U;
	zassert_equal(mc_comm_req_size(&config), 0U, NULL);
	config = valid_config;
	config.receive_capacity = 31U;
	zassert_equal(mc_comm_req_size(&config), 0U, NULL);
	config = valid_config;
	zassert_equal(mc_comm_init(NULL, storage, sizeof(storage), &config), -EINVAL, NULL);
	zassert_equal(mc_comm_init(&instance, NULL, sizeof(storage), &config), -EINVAL, NULL);
	zassert_equal(mc_comm_init(&instance, storage, 1U, &config), -ENOMEM, NULL);
}

ZTEST(mc_comm, test_caller_owned_storage_bounds)
{
	struct mc_comm_cfg config = valid_config;
	struct mc_comm *instance;
	uint8_t storage[512] __aligned(8);
	size_t required = mc_comm_req_size(&config);

	zassert_true(required > 0U && required < sizeof(storage), NULL);
	memset(storage, 0xa5, sizeof(storage));
	zassert_equal(mc_comm_init(&instance, storage, required - 1U, &config), -ENOMEM, NULL);
	zassert_equal(mc_comm_init(&instance, &storage[1], required, &config), -ENOMEM, NULL);
	zassert_ok(mc_comm_init(&instance, storage, required, &config), NULL);
	zassert_equal((void *)instance, (void *)storage, NULL);
	for (size_t i = required; i < sizeof(storage); i++) {
		zassert_equal(storage[i], 0xa5U, NULL);
	}
}

ZTEST(mc_comm, test_invalid_arguments_and_timeout)
{
	struct mc_comm *instance;
	struct test_link link = {0};
	struct test_endpoint endpoint = {
		.receive_link = &link,
		.transmit_link = &link,
	};
	struct mc_comm_cfg config = valid_config;
	uint8_t storage[512] __aligned(8);
	uint8_t data[8];
	size_t length;

	config.io.context = &endpoint;
	zassert_ok(mc_comm_init(&instance, storage, sizeof(storage), &config), NULL);
	zassert_equal(mc_comm_update(NULL), -EINVAL, NULL);
	zassert_equal(mc_comm_send(NULL, data, sizeof(data), 0U), -EINVAL, NULL);
	zassert_equal(mc_comm_send(instance, NULL, sizeof(data), 0U), -EINVAL, NULL);
	if (sizeof(size_t) > sizeof(uint32_t)) {
		zassert_equal(mc_comm_send(instance, data, (size_t)UINT32_MAX + 1U, 0U),
			      -EMSGSIZE, NULL);
	}
	zassert_equal(mc_comm_recv(instance, data, sizeof(data), NULL, 0U), -EINVAL, NULL);
	zassert_equal(mc_comm_recv(instance, data, sizeof(data), &length, 0U), -ETIMEDOUT, NULL);
	zassert_equal(mc_comm_flush(NULL, 0U), -EINVAL, NULL);
}

ZTEST(mc_comm, test_transfer_and_zero_length_payload)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_comm *a;
	struct mc_comm *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	int err;
	uint8_t a_storage[512] __aligned(8);
	uint8_t b_storage[512] __aligned(8);
	const uint8_t payload[] = "payload";
	uint8_t received[sizeof(payload)];
	size_t received_length = 0U;

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint, a_storage, b_storage);
	zassert_ok(mc_comm_send(a, payload, sizeof(payload), 100000U), NULL);
	err = mc_comm_recv(b, received, sizeof(received), &received_length, 100000U);
	zassert_equal(err, 0, NULL);
	zassert_equal(received_length, sizeof(payload), NULL);
	zassert_mem_equal(received, payload, sizeof(payload), NULL);
	zassert_ok(mc_comm_send(a, NULL, 0U, 100000U), NULL);
	received_length = 1U;
	zassert_ok(mc_comm_recv(b, received, sizeof(received), &received_length, 100000U), NULL);
	zassert_equal(received_length, 0U, NULL);
}

ZTEST(mc_comm, test_corrupted_frame_and_bounded_retry)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_comm *a;
	struct mc_comm *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[512] __aligned(8);
	uint8_t b_storage[512] __aligned(8);
	const uint8_t payload[] = "payload";

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint, a_storage, b_storage);
	a_to_b.corrupt = true;
	zassert_equal(mc_comm_send(a, payload, sizeof(payload), 50000U), -ETIMEDOUT, NULL);

	a_to_b.corrupt = false;
	a_to_b.drop = true;
	zassert_equal(mc_comm_send(a, payload, sizeof(payload), 50000U), -ETIMEDOUT, NULL);
	zassert_equal(mc_comm_send(a, payload, sizeof(payload), 200000U), -ETIMEDOUT, NULL);
}

ZTEST(mc_comm, test_retries_after_deterministic_frame_loss)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_comm *a;
	struct mc_comm *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[512] __aligned(8);
	uint8_t b_storage[512] __aligned(8);
	const uint8_t payload[] = "loss-recovery";
	uint8_t received[sizeof(payload)];
	size_t received_length = 0U;

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint, a_storage, b_storage);
	/* Lose two DATA frames; the third attempt must complete normally. */
	a_to_b.drop_count = 2U;
	zassert_ok(mc_comm_send(a, payload, sizeof(payload), 100000U), NULL);
	zassert_equal(a_to_b.dropped_count, 2U, NULL);
	zassert_equal(a_to_b.transmit_count, 3U, NULL);
	zassert_ok(mc_comm_recv(b, received, sizeof(received), &received_length, 100000U), NULL);
	zassert_equal(received_length, sizeof(payload), NULL);
	zassert_mem_equal(received, payload, sizeof(payload), NULL);
}

ZTEST(mc_comm, test_transport_failures)
{
	struct test_link link = {0};
	struct test_endpoint endpoint = {
		.receive_link = &link,
		.transmit_link = &link,
	};
	struct mc_comm_cfg config = valid_config;
	struct mc_comm *instance;
	uint8_t storage[512] __aligned(8);
	uint8_t data[8];
	size_t length;

	config.io.context = &endpoint;
	zassert_ok(mc_comm_init(&instance, storage, sizeof(storage), &config), NULL);
	link.receive_error = -EIO;
	zassert_equal(mc_comm_update(instance), -EIO, NULL);
	zassert_equal(mc_comm_recv(instance, data, sizeof(data), &length, 0U), -EIO, NULL);
	zassert_equal(mc_comm_send(instance, data, sizeof(data), 1000U), -EIO, NULL);
	link.receive_error = 0;
	link.oversize_receive = true;
	zassert_equal(mc_comm_update(instance), -EMSGSIZE, NULL);
	link.oversize_receive = false;
	link.transmit_error = -EIO;
	zassert_equal(mc_comm_send(instance, data, sizeof(data), 1000U), -EIO, NULL);
	link.transmit_error = 0;
	link.short_transmit = true;
	zassert_equal(mc_comm_send(instance, data, sizeof(data), 1000U), -EIO, NULL);
	link.short_transmit = false;
	link.full = false;
	link.transmit_count = 0U;
	link.drop = true;
	link.fail_retransmit = true;
	zassert_equal(mc_comm_send(instance, data, sizeof(data), 100000U), -EIO, NULL);
}

ZTEST(mc_comm, test_malformed_frames_and_receive_capacity)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_comm *a;
	struct mc_comm *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[512] __aligned(8);
	uint8_t b_storage[512] __aligned(8);
	uint8_t data[8];
	size_t length = 0U;
	const uint8_t too_large[] = {0U, 0U, 0U, 33U};
	const uint8_t extra[] = {0U, 0U, 0U, 0U, 1U};
	const uint8_t valid[] = {0U, 0U, 0U, 1U, 42U};

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint, a_storage, b_storage);
	set_frame(&a_to_b, 1U, 1U, valid, sizeof(valid));
	zassert_ok(mc_comm_update(b), NULL);
	set_frame(&a_to_b, 2U, 0U, valid, sizeof(valid));
	zassert_equal(mc_comm_update(b), -EBADMSG, NULL);
	set_frame(&a_to_b, 3U, 0U, NULL, 0U);
	zassert_equal(mc_comm_update(b), -EBADMSG, NULL);
	set_frame(&a_to_b, 1U, 0U, too_large, sizeof(too_large));
	zassert_equal(mc_comm_update(b), -EMSGSIZE, NULL);
	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint, a_storage, b_storage);
	set_frame(&a_to_b, 1U, 0U, extra, sizeof(extra));
	zassert_equal(mc_comm_update(b), -EBADMSG, NULL);
	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint, a_storage, b_storage);
	b_to_a.transmit_error = -EIO;
	set_frame(&a_to_b, 1U, 0U, valid, sizeof(valid));
	zassert_equal(mc_comm_update(b), -EIO, NULL);
	b_to_a.transmit_error = 0;
	zassert_equal(mc_comm_recv(b, data, 0U, &length, 0U), -ENOSPC, NULL);
	zassert_ok(mc_comm_recv(b, data, sizeof(data), &length, 0U), NULL);
	zassert_equal(length, 1U, NULL);
}

ZTEST(mc_comm, test_reordered_and_duplicate_frames)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_comm *a;
	struct mc_comm *b;
	struct mc_comm_cfg config = valid_config;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[512] __aligned(8);
	uint8_t b_storage[512] __aligned(8);
	uint8_t received[5];
	size_t length = 0U;
	const uint8_t first[] = {0U, 0U, 0U, 5U, 'a', 'b'};
	const uint8_t second[] = {'c', 'd', 'e'};

	a_endpoint.receive_link = &b_to_a;
	a_endpoint.transmit_link = &a_to_b;
	b_endpoint.receive_link = &a_to_b;
	b_endpoint.transmit_link = &b_to_a;
	config.recv.capacity = 2U;
	config.io.context = &a_endpoint;
	zassert_ok(mc_comm_init(&a, a_storage, sizeof(a_storage), &config), NULL);
	config.io.context = &b_endpoint;
	zassert_ok(mc_comm_init(&b, b_storage, sizeof(b_storage), &config), NULL);
	a_to_b.peer = b;
	b_to_a.peer = a;
	set_frame(&a_to_b, 1U, 1U, second, sizeof(second));
	zassert_ok(mc_comm_update(b), NULL);
	set_frame(&a_to_b, 1U, 0U, first, sizeof(first));
	zassert_ok(mc_comm_update(b), NULL);
	set_frame(&a_to_b, 1U, 0U, first, sizeof(first));
	zassert_ok(mc_comm_update(b), NULL);
	zassert_ok(mc_comm_recv(b, received, sizeof(received), &length, 0U), NULL);
	zassert_equal(length, sizeof(received), NULL);
	zassert_mem_equal(received, "abcde", sizeof(received), NULL);
	zassert_equal(mc_comm_recv(b, received, sizeof(received), &length, 0U),
		      -ETIMEDOUT, NULL);
}

ZTEST_SUITE(mc_comm, NULL, NULL, NULL, NULL, NULL);
