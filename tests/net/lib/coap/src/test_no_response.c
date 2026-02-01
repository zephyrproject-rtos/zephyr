/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


ZTEST(coap, test_no_response_option_absent)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE] = {0};
	bool suppress;
	int r;

	/* Build a request without No-Response option */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, -ENOENT, "Expected -ENOENT when option is absent, got %d", r);

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, -ENOENT, "Expected -ENOENT when option is absent, got %d", r);

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, -ENOENT, "Expected -ENOENT when option is absent, got %d", r);
}

ZTEST(coap, test_no_response_option_empty)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	bool suppress;
	int r;

	/* Build a request with empty No-Response option (interested in all responses) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	/* Add empty No-Response option */
	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE, NULL, 0);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Empty option should not suppress 2.xx");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Empty option should not suppress 4.xx");

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Empty option should not suppress 5.xx");
}

ZTEST(coap, test_no_response_option_suppress_2xx)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value = COAP_NO_RESPONSE_SUPPRESS_2_XX;
	bool suppress;
	int r;

	/* Build a request with No-Response option set to suppress 2.xx */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx responses - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_OK, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.00 OK");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.05 Content");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CHANGED, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.04 Changed");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 4.04 Not Found");

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 5.00 Internal Error");
}

ZTEST(coap, test_no_response_option_suppress_4xx)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value = COAP_NO_RESPONSE_SUPPRESS_4_XX;
	bool suppress;
	int r;

	/* Build a request with No-Response option set to suppress 4.xx */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 2.05 Content");

	/* Check 4.xx responses - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_BAD_REQUEST, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.00 Bad Request");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.04 Not Found");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_BAD_OPTION, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.02 Bad Option");

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 5.00 Internal Error");
}

ZTEST(coap, test_no_response_option_suppress_5xx)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value = COAP_NO_RESPONSE_SUPPRESS_5_XX;
	bool suppress;
	int r;

	/* Build a request with No-Response option set to suppress 5.xx */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 2.05 Content");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 4.04 Not Found");

	/* Check 5.xx responses - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.00 Internal Error");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_IMPLEMENTED, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.01 Not Implemented");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_BAD_GATEWAY, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.02 Bad Gateway");
}

ZTEST(coap, test_no_response_option_suppress_combinations)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value;
	bool suppress;
	int r;

	/* Test suppressing 2.xx and 5.xx (0x12 = 0x02 | 0x10) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	no_response_value = COAP_NO_RESPONSE_SUPPRESS_2_XX | COAP_NO_RESPONSE_SUPPRESS_5_XX;
	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.05 Content");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 4.04 Not Found");

	/* Check 5.xx response - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.00 Internal Error");

	/* Test suppressing all (0x1A = 0x02 | 0x08 | 0x10) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	no_response_value = COAP_NO_RESPONSE_SUPPRESS_ALL;
	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* All response classes should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.05 Content");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.04 Not Found");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.00 Internal Error");
}

ZTEST(coap, test_no_response_option_invalid_length)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value[2] = {0x02, 0x08};
	bool suppress;
	int r;

	/* Build a request with invalid No-Response option (length > 1) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      no_response_value, 2);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check that invalid length is detected */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, -EINVAL, "Should return -EINVAL for invalid option length");
}
