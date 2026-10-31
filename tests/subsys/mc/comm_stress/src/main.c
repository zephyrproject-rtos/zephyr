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
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#include "transport.h"

#define TEST_STORAGE_SIZE 20000U
#define TEST_MESSAGE_SIZE 4096U
#define TEST_SEND_FRAME_SIZE 1157U
#define TEST_RECEIVE_FRAME_SIZE 59U

static uint8_t sender_storage[TEST_STORAGE_SIZE] __aligned(8);
static uint8_t receiver_storage[TEST_STORAGE_SIZE] __aligned(8);
static struct mc_comm *sender_comm;
static struct mc_comm *receiver_comm;
static uint8_t received[TEST_MESSAGE_SIZE];
static uint32_t words[1024];

static struct mc_comm_config make_config(bool sender, uint8_t retry_limit)
{
	struct mc_comm_config config = {
		.receive = stress_transport_receive,
		.transmit = stress_transport_transmit,
		.context = stress_transport_context(sender),
		.frame_size = 64U,
		.receive_capacity = TEST_MESSAGE_SIZE,
		.retry_limit = retry_limit,
		.send_window_size = sender ? TEST_SEND_FRAME_SIZE : TEST_RECEIVE_FRAME_SIZE,
		.receive_window_size = sender ? TEST_RECEIVE_FRAME_SIZE : TEST_SEND_FRAME_SIZE,
		.send_window_count = sender ? 3U : 1U,
		.receive_window_count = sender ? 2U : 4U,
	};

	return config;
}

static void init_pair(uint8_t loss_rate, bool duplicate, bool corrupt, bool shorten,
		      bool disconnect, uint8_t retry_limit)
{
	struct mc_comm_config sender_config;
	struct mc_comm_config receiver_config;

	zassert_ok(stress_transport_init(), NULL);
	stress_transport_reset(loss_rate, duplicate, corrupt, shorten, disconnect);
	sender_config = make_config(true, retry_limit);
	receiver_config = make_config(false, retry_limit);
	zassert_true(mc_comm_required_size(&sender_config) <= sizeof(sender_storage), NULL);
	zassert_true(mc_comm_required_size(&receiver_config) <= sizeof(receiver_storage), NULL);
	zassert_ok(mc_comm_init(&sender_comm, sender_storage, sizeof(sender_storage),
				&sender_config), NULL);
	zassert_ok(mc_comm_init(&receiver_comm, receiver_storage, sizeof(receiver_storage),
				&receiver_config), NULL);
	stress_transport_set_peer(true, receiver_comm);
	stress_transport_set_peer(false, sender_comm);
	if (IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) &&
	    IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER)) {
		k_msleep(200U);
	}
}

static int exchange(const void *expected, size_t length, uint32_t timeout_us)
{
	size_t received_length = 0U;
	int err;

	if (!IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ||
	    IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER)) {
		err = mc_comm_send(sender_comm, expected, length, timeout_us);
		if (err != 0) {
			return err;
		}
	}
	if (!IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ||
	    !IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER)) {
		err = mc_comm_receive(receiver_comm, received, sizeof(received),
				      &received_length, timeout_us);
		if (err != 0) {
			return err;
		}
		if (received_length != length ||
		    (length > 0U && memcmp(received, expected, length) != 0)) {
			return -EBADMSG;
		}
	}
	return 0;
}

static size_t variadic_count(uint32_t seed)
{
	return (((seed * 1664525U) + 1013904223U) % 997U) + 27U;
}

static int transfer_iteration(uint32_t seed, uint32_t timeout_us)
{
	char string[9] = {0};
	bool tiny = (seed & 1U) != 0U;
	size_t count = variadic_count(seed);
	int err;

	(void)snprintk(string, sizeof(string), "!p%03u.?I", seed % 1000U);
	err = exchange(string, sizeof(string), timeout_us);
	if (err != 0) {
		return err;
	}
	for (size_t i = 0U; i < count; i++) {
		uint32_t coefficient = (i & 1U) != 0U ? 2654435761U : 40503U;

		words[i] = coefficient * (uint32_t)(i + 1U) * (seed + 1U) + (uint32_t)i;
	}
	err = exchange(words, count * sizeof(*words), timeout_us);
	if (err != 0) {
		return err;
	}
	return exchange(&tiny, sizeof(tiny), timeout_us);
}

static int run_transfer(uint32_t iterations, uint8_t loss_rate, bool duplicate,
			bool corrupt, bool shorten, bool disconnect, bool allow_timeout)
{
	uint32_t timeout_us = allow_timeout ? 3000000U : 1000000U;
	uint8_t retry_limit = allow_timeout ? 100U : 30U;

	init_pair(loss_rate, duplicate, corrupt, shorten, disconnect, retry_limit);
	if (loss_rate == 0U) {
		int err = exchange(NULL, 0U, timeout_us);

		if (err != 0) {
			return err;
		}
		for (size_t i = 0U; i < ARRAY_SIZE(words); i++) {
			words[i] = (uint32_t)i * 2654435761U;
		}
		err = exchange(words, sizeof(words), timeout_us);
		if (err != 0) {
			return err;
		}
	}
	for (uint32_t i = 0U; i < iterations; i++) {
		int err = transfer_iteration(i, timeout_us);

		if (err == -ETIMEDOUT && allow_timeout) {
			return 0;
		}
		if (err != 0) {
			return err;
		}
	}
	return mc_comm_flush(IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) &&
				     !IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER) ?
				     receiver_comm : sender_comm, timeout_us);
}

ZTEST(mc_comm_stress, test_01_invalid_creation)
{
	struct mc_comm_config config;

	zassert_ok(stress_transport_init(), NULL);
	config = make_config(true, 3U);
	config.receive = NULL;
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
	config = make_config(true, 3U);
	config.transmit = NULL;
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
	config = make_config(true, 3U);
	config.send_window_size = 12U;
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
	config = make_config(true, 3U);
	config.receive_window_count = 17U;
	zassert_equal(mc_comm_required_size(&config), 0U, NULL);
}

ZTEST(mc_comm_stress, test_02_invalid_argument)
{
	uint8_t data[10];
	size_t received_length;

	init_pair(0U, false, false, false, false, 3U);
	zassert_equal(mc_comm_send(NULL, data, sizeof(data), 100U), -EINVAL, NULL);
	zassert_equal(mc_comm_send(sender_comm, NULL, sizeof(data), 100U), -EINVAL, NULL);
	zassert_equal(mc_comm_receive(NULL, data, sizeof(data), &received_length, 100U),
		      -EINVAL, NULL);
	zassert_equal(mc_comm_receive(receiver_comm, NULL, sizeof(data), &received_length,
				      100U), -EINVAL, NULL);
	zassert_equal(mc_comm_update(NULL), -EINVAL, NULL);
	zassert_equal(mc_comm_flush(NULL, 100U), -EINVAL, NULL);
}

ZTEST(mc_comm_stress, test_03_valid_creation)
{
	struct mc_comm_config config;

	init_pair(0U, false, false, false, false, 3U);
	config = make_config(true, 3U);
	zassert_true(mc_comm_required_size(&config) > 0U, NULL);
	zassert_not_null(sender_comm, NULL);
	zassert_not_null(receiver_comm, NULL);
}

ZTEST(mc_comm_stress, test_04_singly_direction)
{
	uint32_t iterations = IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ? 200U : 20U;

	zassert_ok(run_transfer(iterations, 0U, false, false, false, false, false), NULL);
}

ZTEST(mc_comm_stress, test_05_singly_repetitive)
{
	uint32_t iterations = IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ? 200U : 20U;

	zassert_ok(run_transfer(iterations, 0U, true, false, false, false, false), NULL);
}

ZTEST(mc_comm_stress, test_06_singly_low_lossy)
{
	uint32_t iterations = IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ? 200U : 20U;

	zassert_ok(run_transfer(iterations, 20U, true, true, true, true, false), NULL);
}

ZTEST(mc_comm_stress, test_07_singly_high_lossy)
{
	uint32_t iterations = IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ? 100U : 3U;

	zassert_ok(run_transfer(iterations, 96U, true, true, true, true, true), NULL);
}

ZTEST(mc_comm_stress, test_08_singly_timed_out)
{
	uint8_t data[] = "timeout";
	uint8_t received[sizeof(data)];
	size_t length;

	init_pair(100U, false, false, false, false, 3U);
	if (!IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ||
	    IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER)) {
		zassert_equal(mc_comm_send(sender_comm, data, sizeof(data), 50000U),
			      -ETIMEDOUT, NULL);
	}
	if (!IS_ENABLED(CONFIG_TEST_MC_STRESS_UART) ||
	    !IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER)) {
		zassert_equal(mc_comm_receive(receiver_comm, received, sizeof(received),
					      &length, 50000U), -ETIMEDOUT, NULL);
	}
}

ZTEST_SUITE(mc_comm_stress, NULL, NULL, NULL, NULL, NULL);
