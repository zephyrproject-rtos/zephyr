/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

struct version_msg {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
};

struct external_data_msg {
	void *reference;
	size_t size;
};

ZBUS_CHAN_DEFINE(dyn_chan_no_subs,	   /* Name */
		 struct external_data_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(0)      /* Initial value {0} */
);

ZBUS_CHAN_DEFINE(dyn_chan,		   /* Name */
		 struct external_data_msg, /* Message type */

		 NULL,		     /* Validator */
		 NULL,		     /* User data */
		 ZBUS_OBSERVERS(s1), /* observers */
		 ZBUS_MSG_INIT(0)    /* Initial value {0} */
);

static struct {
	uint8_t a;
	uint64_t b;
} my_random_data_expected;

static void s1_cb(const struct zbus_channel *chan)
{
	LOG_DBG("Callback called");

	const struct external_data_msg *chan_message;

	chan_message = zbus_chan_const_msg(chan);
	memcpy(&my_random_data_expected, chan_message->reference, sizeof(my_random_data_expected));

	zbus_chan_finish(chan);
}
ZBUS_LISTENER_DEFINE(s1, s1_cb);

/**
 * @brief Test Asserts
 *
 * This test verifies various assert macros provided by ztest.
 *
 */
ZTEST(dynamic, test_static)
{
	uint8_t static_memory[256] = {0};
	struct external_data_msg static_external_data = {.reference = static_memory, .size = 256};

	struct {
		uint8_t a;
		uint64_t b;
	} my_random_data = {.a = 10, .b = 200000};

	memcpy(static_memory, &my_random_data, sizeof(my_random_data));

	int err = zbus_chan_pub(&dyn_chan, &static_external_data, K_MSEC(500));

	zassert_equal(err, 0, "Allocation could not be performed");

	k_msleep(1000);

	zassert_equal(my_random_data.a, my_random_data_expected.a,
		      "It must be 10, random data is %d", my_random_data_expected.a);
	zassert_equal(my_random_data.b, my_random_data_expected.b, "It must be 200000");

	struct external_data_msg edm = {0};

	zbus_chan_read(&dyn_chan, &edm, K_NO_WAIT);
	zassert_equal(edm.reference, static_memory, "The pointer must be equal here");
}

ZTEST(dynamic, test_malloc)
{
	uint8_t *dynamic_memory = k_malloc(128);

	zassert_not_equal(dynamic_memory, NULL, "Memory could not be allocated");

	struct external_data_msg static_external_data = {.reference = dynamic_memory, .size = 128};

	struct {
		uint8_t a;
		uint64_t b;
	} my_random_data = {.a = 20, .b = 300000};

	memcpy(dynamic_memory, &my_random_data, sizeof(my_random_data));

	int err = zbus_chan_pub(&dyn_chan, &static_external_data, K_NO_WAIT);

	zassert_equal(err, 0, "Allocation could not be performed");

	k_msleep(1000);

	zassert_equal(my_random_data.a, my_random_data_expected.a, "It must be 20");
	zassert_equal(my_random_data.b, my_random_data_expected.b, "It must be 300000");

	struct external_data_msg *actual_message_data = NULL;

	err = zbus_chan_claim(&dyn_chan, K_NO_WAIT);
	zassert_equal(err, 0, "Could not claim the channel");

	actual_message_data = (struct external_data_msg *)dyn_chan.message;
	zassert_equal(actual_message_data->reference, dynamic_memory,
		      "The pointer must be equal here");

	k_free(actual_message_data->reference);
	actual_message_data->reference = NULL;
	actual_message_data->size = 0;

	zbus_chan_finish(&dyn_chan);

	struct external_data_msg expected_to_be_clean = {0};

	zbus_chan_read(&dyn_chan, &expected_to_be_clean, K_NO_WAIT);
	zassert_equal(expected_to_be_clean.reference, NULL,
		      "The current message reference should be NULL");
	zassert_equal(expected_to_be_clean.size, 0, "The current message size should be zero");
}

ZTEST_SUITE(dynamic, NULL, NULL, NULL, NULL, NULL);
