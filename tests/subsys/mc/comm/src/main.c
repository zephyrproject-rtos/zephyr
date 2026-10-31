/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/mc/comm.h>
#include <zephyr/ztest.h>

struct test_link {
	uint8_t data[128];
	size_t length;
	bool full;
	bool corrupt;
	bool drop;
	uint8_t drop_count;
	uint32_t transmit_count;
	uint32_t dropped_count;
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

static const struct mc_comm_config valid_config = {
	.receive = receive,
	.transmit = transmit,
	.frame_size = 32U,
	.receive_capacity = 32U,
};

static void init_pair(struct mc_comm **a, struct mc_comm **b, struct test_link *a_to_b,
		      struct test_link *b_to_a, struct test_endpoint *a_endpoint,
		      struct test_endpoint *b_endpoint, uint8_t *a_storage, uint8_t *b_storage)
{
	struct mc_comm_config config = valid_config;

	a_endpoint->receive_link = b_to_a;
	a_endpoint->transmit_link = a_to_b;
	b_endpoint->receive_link = a_to_b;
	b_endpoint->transmit_link = b_to_a;

	config.context = a_endpoint;
	zassert_ok(mc_comm_init(a, a_storage, 512U, &config), NULL);
	config.context = b_endpoint;
	zassert_ok(mc_comm_init(b, b_storage, 512U, &config), NULL);
	a_to_b->peer = *b;
	b_to_a->peer = *a;
}

ZTEST(mc_comm, test_invalid_configuration)
{
	struct mc_comm_config config = {0};
	struct mc_comm *instance;
	uint8_t storage[512] __aligned(8);

	zassert_equal(mc_comm_required_size(NULL), 0U, NULL);
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
	config = valid_config;
	config.frame_size = 8U;
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
	config = valid_config;
	config.receive_capacity = 31U;
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
	config = valid_config;
	zassert_equal(mc_comm_init(NULL, storage, sizeof(storage), &config), -EINVAL, NULL);
	zassert_equal(mc_comm_init(&instance, NULL, sizeof(storage), &config), -EINVAL, NULL);
	zassert_equal(mc_comm_init(&instance, storage, 1U, &config), -ENOMEM, NULL);
}

ZTEST(mc_comm, test_invalid_arguments_and_timeout)
{
	struct mc_comm *instance;
	struct test_link link = {0};
	struct test_endpoint endpoint = {
		.receive_link = &link,
		.transmit_link = &link,
	};
	struct mc_comm_config config = valid_config;
	uint8_t storage[512] __aligned(8);
	uint8_t data[8];
	size_t length;

	config.context = &endpoint;
	zassert_ok(mc_comm_init(&instance, storage, sizeof(storage), &config), NULL);
	zassert_equal(mc_comm_update(NULL), -EINVAL, NULL);
	zassert_equal(mc_comm_send(NULL, data, sizeof(data), 0U), -EINVAL, NULL);
	zassert_equal(mc_comm_send(instance, NULL, sizeof(data), 0U), -EINVAL, NULL);
	zassert_equal(mc_comm_receive(instance, data, sizeof(data), NULL, 0U), -EINVAL, NULL);
	zassert_equal(mc_comm_receive(instance, data, sizeof(data), &length, 0U), -ETIMEDOUT, NULL);
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
	err = mc_comm_receive(b, received, sizeof(received), &received_length, 100000U);
	zassert_equal(err, 0, NULL);
	zassert_equal(received_length, sizeof(payload), NULL);
	zassert_mem_equal(received, payload, sizeof(payload), NULL);
	zassert_ok(mc_comm_send(a, NULL, 0U, 100000U), NULL);
	received_length = 1U;
	zassert_ok(mc_comm_receive(b, received, sizeof(received), &received_length, 100000U), NULL);
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
	zassert_ok(mc_comm_receive(b, received, sizeof(received), &received_length, 100000U), NULL);
	zassert_equal(received_length, sizeof(payload), NULL);
	zassert_mem_equal(received, payload, sizeof(payload), NULL);
}

ZTEST_SUITE(mc_comm, NULL, NULL, NULL, NULL, NULL);
