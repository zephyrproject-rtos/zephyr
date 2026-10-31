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
	memcpy(link->data, data, length);
	link->length = length;
	link->full = true;
	if (link->peer != NULL) {
		(void)mc_comm_update(link->peer);
	}
	return (int)length;
}

static const struct mc_msg_config valid_config = {
	.comm = {
		.receive = receive,
		.transmit = transmit,
		.frame_size = 64U,
		.receive_capacity = 64U,
	},
	.message_capacity = 32U,
	.subscription_capacity = 2U,
};

static void init_pair(struct mc_msg **a, struct mc_msg **b,
			      struct test_link *a_to_b, struct test_link *b_to_a,
			      struct test_endpoint *a_endpoint,
			      struct test_endpoint *b_endpoint,
			      uint8_t *a_storage, uint8_t *b_storage)
{
	struct mc_msg_config config = valid_config;

	a_endpoint->receive_link = b_to_a;
	a_endpoint->transmit_link = a_to_b;
	b_endpoint->receive_link = a_to_b;
	b_endpoint->transmit_link = b_to_a;
	config.comm.context = a_endpoint;
	zassert_ok(mc_msg_init(a, a_storage, 1024U, &config), NULL);
	config.comm.context = b_endpoint;
	zassert_ok(mc_msg_init(b, b_storage, 1024U, &config), NULL);
	a_to_b->peer = mc_msg_comm(*b);
	b_to_a->peer = mc_msg_comm(*a);
}

static bool received_message;
static bool received_signal;

static void callback(void *context, uint16_t id, const uint8_t *data,
			    size_t length)
{
	ARG_UNUSED(context);

	if (id == 7U && length == 5U && memcmp(data, "hello", 5U) == 0) {
		received_message = true;
	}
	if (id == 8U && length == 0U) {
		received_signal = true;
	}
}

ZTEST(mc_msg, test_invalid_configuration)
{
	struct mc_msg_config config = {0};

	zassert_equal(mc_msg_required_size(NULL), 0U, NULL);
	zassert_equal(mc_msg_required_size(&config), 0U, NULL);
	config = valid_config;
	config.message_capacity = 3U;
	zassert_equal(mc_msg_required_size(&config), 0U, NULL);
	config = valid_config;
	config.subscription_capacity = 0U;
	zassert_equal(mc_msg_required_size(&config), 0U, NULL);
}

ZTEST(mc_msg, test_subscriptions)
{
	struct mc_msg *instance;
	struct test_link link = {0};
	struct test_endpoint endpoint = {
		.receive_link = &link,
		.transmit_link = &link,
	};
	struct mc_msg_config config = valid_config;
	uint8_t storage[1024] __aligned(8);

	config.comm.context = &endpoint;
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

ZTEST_SUITE(mc_msg, NULL, NULL, NULL, NULL, NULL);
