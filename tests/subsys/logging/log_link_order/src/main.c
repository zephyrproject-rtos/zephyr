/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log links
 *
 */

#include <zephyr/tc_util.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_link.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>
#include "mock_log_link.h"

LOG_MODULE_REGISTER(test);

static log_timestamp_t exp_timestamp[8];
static uint8_t exp_domain[8];
static uint32_t exp_msg_count;
static uint32_t msg_count;
static int timestamp_offset;

static void backend_reset(void)
{
	exp_msg_count = 0;
	msg_count = 0;
}

static void backend_validate(int exp_count)
{
	zassert_equal(msg_count, exp_count, "Expected message count mismatch got:%d (expected:%d)",
		msg_count, exp_count);
}

static void backend_add_message(log_timestamp_t t, uint8_t domain)
{
	exp_timestamp[exp_msg_count] = t;
	exp_domain[exp_msg_count] = domain;
	exp_msg_count++;
}

static void process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	uint32_t delta = k_us_to_cyc_floor32(100);

	zassert_equal(msg->log.hdr.desc.domain, exp_domain[msg_count],
		"Expected domain mismatch got:%d (expected:%d)",
		msg->log.hdr.desc.domain, exp_domain[msg_count]);
	zassert_within(msg->log.hdr.timestamp, exp_timestamp[msg_count], delta,
		"Expected timestamp mismatch got:%d (expected:%d) (now:%d)",
		msg->log.hdr.timestamp, exp_timestamp[msg_count], k_cycle_get_32());
	msg_count++;
	zassert_true(msg_count <= exp_msg_count,
		"Expected message count mismatch got:%d (expected:%d)",
		msg_count, exp_msg_count);
}

static void panic(const struct log_backend *const backend)
{

}

const struct log_backend_api log_backend_test_api = {
	.process = process,
	.panic = panic
};

LOG_BACKEND_DEFINE(backend1, log_backend_test_api, false);

static log_timestamp_t set_timestamp_offset(int offset)
{
	timestamp_offset = offset;
	return k_cycle_get_32() + offset;
}

static log_timestamp_t timestamp_get(void)
{
	log_timestamp_t timestamp = k_cycle_get_32() + timestamp_offset;

	timestamp_offset = 0;

	return timestamp;
}

static void log_setup(void)
{
	uint8_t offset = 0;

	log_init();

	(void)z_log_links_activate(0xFFFFFFFF, &offset);

	log_backend_enable(&backend1, NULL, LOG_LEVEL_DBG);
}

static void test_log_process(uint32_t exp_msg_count, bool drain, uint32_t timeout)
{
	uint32_t now = k_cycle_get_32();

	while (1) {
		(void)LOG_PROCESS();

		uint32_t diff = k_cycle_get_32() - now;

		if (k_cyc_to_us_floor32(diff) > timeout) {
			break;
		}

		k_msleep(1);
	}

	if (drain) {
		backend_reset();
		return;
	}

	backend_validate(exp_msg_count);
}

MOCK_LOG_LINK_DEFINE(mock_link1);
MOCK_LOG_LINK_DEFINE(mock_link2);
MOCK_LOG_LINK_DEFINE(mock_link3);

static void remote_message_prepare(const struct log_link *link)
{
	struct log_msg log1;

	log1.hdr.desc.type = Z_LOG_MSG_LOG;
	log1.hdr.desc.domain = 0;
	log1.hdr.desc.package_len = 0;
	log1.hdr.desc.data_len = 0;
	log1.hdr.timestamp = timestamp_get();

	mock_log_link_add_message(link, &log1);
}

ZTEST(log_link_order, test_log_local_remote_ordered)
{
	log_timestamp_t t0, t1;

	/* Create local message with current timestamp. */
	t0 = set_timestamp_offset(0);
	LOG_INF("log1");

	/* Create link message with timestamp now + 1.*/
	t1 = set_timestamp_offset(1);
	remote_message_prepare(&mock_link1);

	/* Expect ordered processing: first local then remote. */
	backend_add_message(t0, 0);
	backend_add_message(t1, 1);

	test_log_process(2, false, 100000);
}

ZTEST(log_link_order, test_log_unordered)
{
	log_timestamp_t t0, t1;

	int processing_latency = k_us_to_cyc_floor32(CONFIG_LOG_PROCESSING_LATENCY_US + 300);

	/* Create local message with current timestamp. */
	t0 = set_timestamp_offset(0);
	LOG_INF("log1");

	/* Message is not processed yet. System is waiting for potential remote messages. */
	test_log_process(0, false, 0);

	/* Simulate receiving of remote message. It is enqueued later but with
	 * earlier timestamp.
	 */
	t1 = set_timestamp_offset(-processing_latency);
	remote_message_prepare(&mock_link1);

	backend_add_message(t1, 1);
	backend_add_message(t0, 0);
	test_log_process(2, false, 100000);
}

ZTEST(log_link_order, test_log_multiple_links)
{
	log_timestamp_t t0, t1, t2, t3, t4;

	/* Create local message with current timestamp. */
	t0 = set_timestamp_offset(0);
	LOG_INF("log1");

	/* Simulate receiving of remote messages. It is enqueued later but with
	 * earlier timestamp.
	 */
	t1 = set_timestamp_offset(-k_us_to_cyc_floor32(700));
	remote_message_prepare(&mock_link1);

	t2 = set_timestamp_offset(-k_us_to_cyc_floor32(680));
	remote_message_prepare(&mock_link1);

	t3 = set_timestamp_offset(-k_us_to_cyc_floor32(800));
	remote_message_prepare(&mock_link2);

	t4 = set_timestamp_offset(-k_us_to_cyc_floor32(200));
	remote_message_prepare(&mock_link3);

	/* Messages are processed in the order of their timestamps and not order of arrival. */
	backend_add_message(t3, 2);
	backend_add_message(t1, 1);
	backend_add_message(t2, 1);
	backend_add_message(t4, 3);
	backend_add_message(t0, 0);

	test_log_process(5, false, 100000);
}

static void before(void *data)
{
	ARG_UNUSED(data);

	log_set_timestamp_func(timestamp_get, sys_clock_hw_cycles_per_sec());
	test_log_process(0, true, 100);
	mock_log_link_reset(&mock_link1);
	mock_log_link_reset(&mock_link2);
	mock_log_link_reset(&mock_link3);
	log_setup();
}

ZTEST_SUITE(log_link_order, NULL, NULL, before, NULL, NULL);
