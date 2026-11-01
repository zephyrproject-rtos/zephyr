/*
 * Copyright (c) 2026 Mahdi Sahebi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/mc/msg.h>
#include <zephyr/ztest.h>

struct test_link {
	uint8_t data[128];
	size_t length;
	bool full;
	bool duplicate;
	bool disconnected;
	uint8_t drop_count;
	uint8_t corrupt_count;
	int receive_error;
	uint32_t receive_calls;
	uint32_t fail_on_receive_call;
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

	link->receive_calls++;
	if (link->receive_calls == link->fail_on_receive_call) {
		return -EIO;
	}
	if (link->receive_error != 0) {
		return link->receive_error;
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

	if (link->full || length > sizeof(link->data)) {
		return -EAGAIN;
	}
	if (link->disconnected) {
		return -EAGAIN;
	}
	if (link->drop_count > 0U) {
		link->drop_count--;
		return (int)length;
	}
	memcpy(link->data, data, length);
	if (link->corrupt_count > 0U) {
		link->corrupt_count--;
		link->data[length - 1U] ^= 1U;
	}
	link->length = length;
	link->full = true;
	if (link->peer != NULL) {
		(void)mc_comm_update(link->peer);
		if (link->duplicate && !link->full) {
			memcpy(link->data, data, length);
			link->length = length;
			link->full = true;
			(void)mc_comm_update(link->peer);
		}
	}
	return (int)length;
}

static const struct mc_msg_cfg valid_config = {
	.comm = {
		.io.receive = receive,
		.io.transmit = transmit,
		.recv.size = 64U,
		.send.size = 64U,
		.receive_capacity = 64U,
	},
	.pool_size = 32U,
	.ids_capacity = 2U,
};

static void init_pair(struct mc_msg **a, struct mc_msg **b,
			      struct test_link *a_to_b, struct test_link *b_to_a,
			      struct test_endpoint *a_endpoint,
			      struct test_endpoint *b_endpoint,
			      uint8_t *a_storage, uint8_t *b_storage)
{
	struct mc_msg_cfg config = valid_config;

	a_endpoint->receive_link = b_to_a;
	a_endpoint->transmit_link = a_to_b;
	b_endpoint->receive_link = a_to_b;
	b_endpoint->transmit_link = b_to_a;
	config.comm.io.context = a_endpoint;
	zassert_ok(mc_msg_init(a, a_storage, 1024U, &config), NULL);
	config.comm.io.context = b_endpoint;
	zassert_ok(mc_msg_init(b, b_storage, 1024U, &config), NULL);
	a_to_b->peer = mc_msg_comm(*b);
	b_to_a->peer = mc_msg_comm(*a);
}

static bool received_message;
static bool received_signal;
static uint32_t callback_count;
static uint8_t expected_payload[32];
static size_t expected_length;
static uint32_t validated_count;

static void callback(void *context, uint16_t id, const uint8_t *data,
			    size_t length)
{
	ARG_UNUSED(context);

	if (id == 7U && length == 5U && memcmp(data, "hello", 5U) == 0) {
		received_message = true;
		callback_count++;
	}
	if (id == 8U && length == 0U) {
		received_signal = true;
	}
}

static void validate_payload(void *context, uint16_t id, const uint8_t *data, size_t length)
{
	ARG_UNUSED(context);

	zassert_equal(id, 42U, NULL);
	zassert_equal(length, expected_length, NULL);
	zassert_mem_equal(data, expected_payload, expected_length, NULL);
	validated_count++;
}

ZTEST(mc_msg, test_invalid_configuration)
{
	struct mc_msg_cfg config = {0};

	zassert_equal(mc_msg_req_size(NULL), 0U, NULL);
	zassert_equal(mc_msg_req_size(&config), 0U, NULL);
	config = valid_config;
	config.pool_size = 3U;
	zassert_equal(mc_msg_req_size(&config), 0U, NULL);
	config = valid_config;
	config.ids_capacity = 0U;
	zassert_equal(mc_msg_req_size(&config), 0U, NULL);
	config.ids_capacity = SIZE_MAX;
	zassert_equal(mc_msg_req_size(&config), 0U, NULL);
}

ZTEST(mc_msg, test_caller_owned_storage_bounds)
{
	struct mc_msg_cfg config = valid_config;
	struct mc_msg *instance;
	uint8_t storage[1024] __aligned(8);
	size_t required = mc_msg_req_size(&config);

	zassert_true(required > 0U && required < sizeof(storage), NULL);
	memset(storage, 0xa5, sizeof(storage));
	zassert_equal(mc_msg_init(&instance, storage, required - 1U, &config), -ENOMEM, NULL);
	zassert_equal(mc_msg_init(&instance, &storage[1], required, &config), -ENOMEM, NULL);
	zassert_ok(mc_msg_init(&instance, storage, required, &config), NULL);
	zassert_equal((void *)instance, (void *)storage, NULL);
	for (size_t i = required; i < sizeof(storage); i++) {
		zassert_equal(storage[i], 0xa5U, NULL);
	}
}

ZTEST(mc_msg, test_subscriptions)
{
	struct mc_msg *instance;
	struct test_link link = {0};
	struct test_endpoint endpoint = {
		.receive_link = &link,
		.transmit_link = &link,
	};
	struct mc_msg_cfg config = valid_config;
	uint8_t storage[1024] __aligned(8);

	config.comm.io.context = &endpoint;
	zassert_ok(mc_msg_init(&instance, storage, sizeof(storage), &config), NULL);
	zassert_equal(mc_msg_subscribe(NULL, 1U, callback, NULL), -EINVAL, NULL);
	zassert_equal(mc_msg_subscribe(instance, 1U, NULL, NULL), -EINVAL, NULL);
	zassert_ok(mc_msg_subscribe(instance, 1U, callback, NULL), NULL);
	zassert_equal(mc_msg_subscribe(instance, 1U, callback, NULL), -EEXIST, NULL);
	zassert_ok(mc_msg_subscribe(instance, 2U, callback, NULL), NULL);
	zassert_equal(mc_msg_subscribe(instance, 3U, callback, NULL), -ENOSPC, NULL);
	zassert_ok(mc_msg_unsubscribe(instance, 1U), NULL);
	zassert_equal(mc_msg_unsubscribe(instance, 1U), -ENOENT, NULL);
	zassert_equal(mc_msg_unsubscribe(NULL, 1U), -EINVAL, NULL);
}

ZTEST(mc_msg, test_message_signal_and_unknown_id)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_msg *a;
	struct mc_msg *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[1024] __aligned(8);
	uint8_t b_storage[1024] __aligned(8);

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint,
		  a_storage, b_storage);
	received_message = false;
	received_signal = false;
	zassert_ok(mc_msg_subscribe(b, 7U, callback, NULL), NULL);
	zassert_ok(mc_msg_subscribe(b, 8U, callback, NULL), NULL);
	zassert_ok(mc_msg_send(a, 7U, "hello", 5U, 100000U), NULL);
	zassert_ok(mc_msg_update(b), NULL);
	zassert_true(received_message, NULL);
	zassert_ok(mc_msg_signal(a, 8U, 100000U), NULL);
	zassert_ok(mc_msg_update(b), NULL);
	zassert_true(received_signal, NULL);
	zassert_ok(mc_msg_send(a, 9U, "ignored", 7U, 100000U), NULL);
	zassert_ok(mc_msg_update(b), NULL);
	zassert_true(received_message, NULL);
}

ZTEST(mc_msg, test_invalid_arguments_and_transport_error)
{
	struct test_link link = {0};
	struct test_endpoint endpoint = {
		.receive_link = &link,
		.transmit_link = &link,
	};
	struct mc_msg_cfg config = valid_config;
	struct mc_msg *instance;
	uint8_t storage[1025] __aligned(8);
	uint8_t data[33] = {0};

	config.comm.io.context = &endpoint;
	zassert_equal(mc_msg_init(NULL, storage, sizeof(storage), &config), -EINVAL, NULL);
	zassert_equal(mc_msg_init(&instance, NULL, sizeof(storage), &config), -EINVAL, NULL);
	zassert_equal(mc_msg_init(&instance, storage, 1U, &config), -ENOMEM, NULL);
	zassert_equal(mc_msg_init(&instance, &storage[1], sizeof(storage) - 1U, &config),
		      -ENOMEM, NULL);
	zassert_ok(mc_msg_init(&instance, storage, sizeof(storage), &config), NULL);
	zassert_equal(mc_msg_update(NULL), -EINVAL, NULL);
	zassert_ok(mc_msg_update(instance), NULL);
	link.receive_error = -EIO;
	zassert_equal(mc_msg_update(instance), -EIO, NULL);
	link.receive_error = 0;
	link.receive_calls = 0U;
	link.fail_on_receive_call = 2U;
	zassert_equal(mc_msg_update(instance), -EIO, NULL);
	link.fail_on_receive_call = 0U;
	zassert_equal(mc_msg_send(NULL, 1U, data, 1U, 0U), -EINVAL, NULL);
	zassert_equal(mc_msg_send(instance, 1U, NULL, 1U, 0U), -EINVAL, NULL);
	zassert_equal(mc_msg_send(instance, 1U, data, sizeof(data), 0U), -EMSGSIZE, NULL);
	zassert_equal(mc_msg_flush(NULL, 0U), -EINVAL, NULL);
	zassert_ok(mc_msg_flush(instance, 0U), NULL);
}

ZTEST(mc_msg, test_malformed_message)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_msg *a;
	struct mc_msg *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[1024] __aligned(8);
	uint8_t b_storage[1024] __aligned(8);
	const uint8_t short_message[] = {0U, 7U, 0U};
	const uint8_t wrong_length[] = {0U, 7U, 0U, 2U, 1U};

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint,
		  a_storage, b_storage);
	zassert_ok(mc_comm_send(mc_msg_comm(a), short_message, sizeof(short_message), 100000U),
		   NULL);
	zassert_equal(mc_msg_update(b), -EBADMSG, NULL);
	zassert_ok(mc_comm_send(mc_msg_comm(a), wrong_length, sizeof(wrong_length), 100000U),
		   NULL);
	zassert_equal(mc_msg_update(b), -EBADMSG, NULL);
}

ZTEST(mc_msg, test_loss_corruption_duplicates_and_recovery)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_msg *a;
	struct mc_msg *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[1024] __aligned(8);
	uint8_t b_storage[1024] __aligned(8);

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint,
		  a_storage, b_storage);
	callback_count = 0U;
	zassert_ok(mc_msg_subscribe(b, 7U, callback, NULL), NULL);
	a_to_b.drop_count = 1U;
	a_to_b.corrupt_count = 1U;
	a_to_b.duplicate = true;
	zassert_ok(mc_msg_send(a, 7U, "hello", 5U, 100000U), NULL);
	zassert_ok(mc_msg_update(b), NULL);
	zassert_equal(callback_count, 1U, NULL);
	a_to_b.disconnected = true;
	zassert_equal(mc_msg_send(a, 7U, "hello", 5U, 30000U), -ETIMEDOUT, NULL);
	a_to_b.disconnected = false;
	zassert_ok(mc_msg_send(a, 7U, "hello", 5U, 100000U), NULL);
	zassert_ok(mc_msg_update(b), NULL);
	zassert_equal(callback_count, 2U, NULL);
}

ZTEST(mc_msg, test_repeated_messages_on_noisy_link)
{
	struct test_link a_to_b = {0};
	struct test_link b_to_a = {0};
	struct mc_msg *a;
	struct mc_msg *b;
	struct test_endpoint a_endpoint;
	struct test_endpoint b_endpoint;
	uint8_t a_storage[1024] __aligned(8);
	uint8_t b_storage[1024] __aligned(8);

	init_pair(&a, &b, &a_to_b, &b_to_a, &a_endpoint, &b_endpoint,
		  a_storage, b_storage);
	zassert_ok(mc_msg_subscribe(b, 42U, validate_payload, NULL), NULL);
	validated_count = 0U;
	a_to_b.duplicate = true;
	for (uint32_t iteration = 0U; iteration < 20U; iteration++) {
		expected_length = (iteration % sizeof(expected_payload)) + 1U;
		for (size_t i = 0U; i < expected_length; i++) {
			expected_payload[i] = (uint8_t)(iteration + i);
		}
		a_to_b.drop_count = iteration % 5U == 0U ? 1U : 0U;
		a_to_b.corrupt_count = iteration % 7U == 0U ? 1U : 0U;
		zassert_ok(mc_msg_send(a, 42U, expected_payload, expected_length, 100000U), NULL);
		zassert_ok(mc_msg_update(b), NULL);
		zassert_equal(validated_count, iteration + 1U, NULL);
	}
}

ZTEST_SUITE(mc_msg, NULL, NULL, NULL, NULL, NULL);
