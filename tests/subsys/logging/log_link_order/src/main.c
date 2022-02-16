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

LOG_MODULE_REGISTER(test);

static log_timestamp_t timestamp;

static void process(const struct log_backend *const backend,
			union log_msg_generic *msg)
{

}

static void panic(const struct log_backend *const backend)
{

}

const struct log_backend_api log_backend_test_api = {
	.process = process,
	.panic = panic
};

LOG_BACKEND_DEFINE(backend1, log_backend_test_api, false);

static log_timestamp_t timestamp_get(void)
{
	return timestamp++;
}

static void log_setup(void)
{
	uint8_t offset = 0;
	int err;

	log_init();

	timestamp = 0;
	err = log_set_timestamp_func(timestamp_get, 0);
	zassert_equal(err, 0, NULL);

	(void)z_log_links_activate(0xFFFFFFFF, &offset);

	log_backend_enable(&backend1, NULL, LOG_LEVEL_DBG);
}

extern struct log_link_api mock_log_link_api;

LOG_LINK_DEF(mock_link1, mock_log_link_api, 0, NULL);
LOG_LINK_DEF(mock_link2, mock_log_link_api, 512, NULL);
LOG_LINK_DEF(mock_link3, mock_log_link_api, 1024, NULL);

static void check_msg(int exp_timestamp, int line)
{
	union log_msg_generic *msg;
	k_timeout_t t;

	msg = z_log_msg_claim(&t);

	if (exp_timestamp < 0) {
		zassert_equal(msg, NULL, "%d: Unexpected msg", line);
		return;
	}

	zassert_true(msg != NULL, "%d: Unexpected msg", line);
	zassert_equal(msg->log.hdr.timestamp, exp_timestamp, "%d: got:%d, exp:%d",
			line, (int)msg->log.hdr.timestamp, exp_timestamp);

	z_log_msg_free(msg);
}

#define CHECK_MSG(t) check_msg(t, __LINE__)

ZTEST(log_link_order, test_log_only_local)
{
	struct log_msg log2;
	struct log_msg *log1;

	log1 = z_log_msg_alloc(sizeof(struct log_msg) / sizeof(int));
	log1->hdr.desc.type = Z_LOG_MSG2_LOG;
	log1->hdr.desc.package_len = 0;
	log1->hdr.desc.data_len = 0;

	/* Commit local message. */
	z_log_msg_commit(log1);

	log2.hdr.desc.type = Z_LOG_MSG2_LOG;
	log2.hdr.desc.package_len = 0;
	log2.hdr.desc.data_len = 0;

	log2.hdr.timestamp = timestamp_get();

	z_log_msg_enqueue(&mock_link1, &log2, sizeof(log2));

	CHECK_MSG(0);
	CHECK_MSG(1);
	CHECK_MSG(-1);
}

ZTEST(log_link_order, test_log_local_unordered)
{
	struct log_msg *log1;
	struct log_msg log2;

	/* Get timestamp for second message before first. Simulate that it is
	 * taken by remote.
	 */
	log_timestamp_t t = timestamp_get();

	log1 = z_log_msg_alloc(sizeof(struct log_msg) / sizeof(int));

	log1->hdr.desc.package_len = 0;
	log1->hdr.desc.data_len = 0;

	/* Commit local message. */
	z_log_msg_commit(log1);

	/* Simulate receiving of remote message. It is enqueued later but with
	 * earlier timestamp.
	 */
	log2.hdr.desc.type = Z_LOG_MSG2_LOG;
	log2.hdr.timestamp = t;
	log2.hdr.desc.package_len = 0;
	log2.hdr.desc.data_len = 0;

	z_log_msg_enqueue(&mock_link1, &log2, sizeof(log2));

	CHECK_MSG(1);
	CHECK_MSG(0);
	CHECK_MSG(-1);
}

ZTEST(log_link_order, test_log_one_remote_ordering)
{
	struct log_msg *log1;
	struct log_msg log2;

	/* Get timestamp for second message before first. Simulate that it is
	 * taken by remote.
	 */
	log_timestamp_t t = timestamp_get();

	log1 = z_log_msg_alloc(sizeof(struct log_msg) / sizeof(int));

	log1->hdr.desc.package_len = 0;
	log1->hdr.desc.data_len = 0;

	/* Commit local message. */
	z_log_msg_commit(log1);

	/* Simulate receiving of remote message. It is enqueued later but with
	 * earlier timestamp. However, it is enqueued to link with dedicated buffer
	 * thus when processing, earliest from the buffers is taken
	 */

	log2.hdr.timestamp = t;
	log2.hdr.desc.type = Z_LOG_MSG2_LOG;
	log2.hdr.desc.package_len = 0;
	log2.hdr.desc.data_len = 0;

	/* link2 has dedicated buffer. */
	z_log_msg_enqueue(&mock_link2, &log2, sizeof(log2));

	log2.hdr.timestamp = timestamp_get();
	/* link2 has dedicated buffer. Log another message with the latest timestamp. */
	z_log_msg_enqueue(&mock_link2, &log2, sizeof(log2));

	CHECK_MSG(0);
	CHECK_MSG(1);
	CHECK_MSG(2);
	CHECK_MSG(-1);
}

static void before(void *data)
{
	ARG_UNUSED(data);

	log_setup();
}

ZTEST_SUITE(log_link_order, NULL, NULL, before, NULL, NULL);
