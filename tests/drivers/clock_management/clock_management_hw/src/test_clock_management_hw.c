/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/clock_management.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define CONSUMER_NODE DT_NODELABEL(emul_dev)

CLOCK_MANAGEMENT_DT_DEFINE(CONSUMER_NODE);

/* Get references to each clock management state and output */
static const struct clock_management_data *data =
	CLOCK_MANAGEMENT_DT_GET(CONSUMER_NODE);
static clock_output_t consumer_out =
	CLOCK_MANAGEMENT_DT_GET_OUTPUT(CONSUMER_NODE);

static clock_request_t default_request =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(CONSUMER_NODE, default);
static clock_request_t sleep_request =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(CONSUMER_NODE, sleep);
static clock_request_t test1_request =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(CONSUMER_NODE, test1);
static clock_request_t test2_request =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(CONSUMER_NODE, test2);
static clock_request_t test3_request =
	CLOCK_MANAGEMENT_DT_GET_REQUEST(CONSUMER_NODE, test3);

void request_clock_state(clock_request_t request, const char *req_name,
		       int expected_rate)
{
	int ret;

	/* Apply clock request, verify frequencies */
	TC_PRINT("Try to apply %s clock request\n", req_name);

	ret = clock_management_request_state(data, request);
	zassert_equal(ret, 0,
		      "Failed to apply %s clock management state", req_name);

	/* Check rate */
	ret = clock_management_get_rate(data, consumer_out);
	TC_PRINT("Consumer %s clock rate: %d\n", req_name, ret);
	zassert_equal(ret, expected_rate,
		      "Consumer has invalid %s clock rate", req_name);
}

void request_clock_frequency(clock_freq_t freq, const char *req_name)
{
	int ret;

	TC_PRINT("Requesting frequency %d for %s\n", freq, req_name);

	ret = clock_management_req_rate(data, consumer_out, freq);
	zassert_equal(ret, freq, "Consumer did not realize requested frequency");
}

ZTEST(clock_management_hw, test_apply_states)
{
	request_clock_state(default_request, "default",
			  DT_PROP(CONSUMER_NODE, default_freq));
	request_clock_state(sleep_request, "sleep",
			  DT_PROP(CONSUMER_NODE, sleep_freq));
	request_clock_state(test1_request, "test1",
			  DT_PROP(CONSUMER_NODE, test1_freq));
	request_clock_state(test2_request, "test2",
			  DT_PROP(CONSUMER_NODE, test2_freq));
	request_clock_state(test3_request, "test3",
			  DT_PROP(CONSUMER_NODE, test3_freq));
}

ZTEST(clock_management_hw, test_freq_req)
{
	clock_freq_t req1 = DT_PROP(CONSUMER_NODE, freq_req_1);
	clock_freq_t req2 = DT_PROP(CONSUMER_NODE, freq_req_2);
	clock_freq_t req3 = DT_PROP(CONSUMER_NODE, freq_req_3);

	request_clock_frequency(req1, "req1");
	request_clock_frequency(req2, "req2");
	request_clock_frequency(req3, "req3");
}

ZTEST_SUITE(clock_management_hw, NULL, NULL, NULL, NULL, NULL);
