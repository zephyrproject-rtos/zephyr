/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


ZTEST(coap, test_hop_limit_constants)
{
	/* RFC 8768 Section 6.2: Hop-Limit option number is 16 */
	zassert_equal(COAP_OPTION_HOP_LIMIT, 16,
		      "COAP_OPTION_HOP_LIMIT must be 16 per RFC 8768");

	/* RFC 8768 Section 6.1: 5.08 Hop Limit Reached response code */
	zassert_equal(COAP_RESPONSE_CODE_HOP_LIMIT_REACHED,
		      COAP_MAKE_RESPONSE_CODE(5, 8),
		      "COAP_RESPONSE_CODE_HOP_LIMIT_REACHED must be 5.08 per RFC 8768");
}

ZTEST(coap, test_hop_limit_code_recognition)
{
	/* RFC 8768 Section 6.1: Verify coap_header_get_code() recognizes 5.08 */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t token[] = { 0x01, 0x02 };
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_NON_CON, sizeof(token), token,
			       COAP_RESPONSE_CODE_HOP_LIMIT_REACHED, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet with 5.08 code");

	uint8_t code = coap_header_get_code(&cpkt);
	zassert_equal(code, COAP_RESPONSE_CODE_HOP_LIMIT_REACHED,
		      "coap_header_get_code() should return 5.08, not 0.00");
}

ZTEST(coap, test_uint_encoding_boundary_255)
{
	/* RFC 7252 Section 3.2: uint encoding must use minimal bytes.
	 * Value 255 must encode as 1 byte (0xFF), not 2 bytes.
	 */
	uint8_t buf[128];
	struct coap_packet cpkt;
	struct coap_option option;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append option with value 255 */
	ret = coap_append_option_int(&cpkt, COAP_OPTION_HOP_LIMIT, 255);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=255");

	/* Parse and verify encoding */
	ret = coap_find_options(&cpkt, COAP_OPTION_HOP_LIMIT, &option, 1);
	zassert_equal(ret, 1, "Failed to find Hop-Limit option");
	zassert_equal(option.len, 1, "Hop-Limit=255 must encode as 1 byte");
	zassert_equal(option.value[0], 0xFF, "Hop-Limit=255 must encode as 0xFF");
}

ZTEST(coap, test_hop_limit_append_valid)
{
	/* RFC 8768 Section 3: Valid Hop-Limit values are 1-255 */
	uint8_t buf[128];
	struct coap_packet cpkt;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Test value 1 (minimum valid) */
	ret = coap_append_hop_limit(&cpkt, 1);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=1");

	/* Reset packet */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Test value 255 (maximum valid) */
	ret = coap_append_hop_limit(&cpkt, 255);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=255");

	/* Reset packet */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1236);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Test value 16 (default) */
	ret = coap_append_hop_limit(&cpkt, 16);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=16");
}

ZTEST(coap, test_hop_limit_append_invalid)
{
	/* RFC 8768 Section 3: Hop-Limit value 0 is invalid */
	uint8_t buf[128];
	struct coap_packet cpkt;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Test value 0 (invalid) */
	ret = coap_append_hop_limit(&cpkt, 0);
	zassert_equal(ret, -EINVAL, "Hop-Limit=0 must be rejected");
}

ZTEST(coap, test_hop_limit_get_valid)
{
	/* RFC 8768 Section 3: Get valid Hop-Limit values */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	int ret;

	/* Test with Hop-Limit=42 */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_hop_limit(&cpkt, 42);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=42");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 42, "Hop-Limit value mismatch");

	/* Test with Hop-Limit=1 (minimum) */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_hop_limit(&cpkt, 1);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=1");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 1, "Hop-Limit value mismatch");

	/* Test with Hop-Limit=255 (maximum) */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1236);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_hop_limit(&cpkt, 255);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=255");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 255, "Hop-Limit value mismatch");
}

ZTEST(coap, test_hop_limit_get_absent)
{
	/* RFC 8768 Section 3: Hop-Limit absent should return -ENOENT */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* No Hop-Limit option added */
	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, -ENOENT, "Absent Hop-Limit should return -ENOENT");
}

ZTEST(coap, test_hop_limit_get_invalid_length)
{
	/* RFC 8768 Section 3: Hop-Limit length must be exactly 1 byte */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	uint8_t invalid_value[2] = { 0x00, 0x10 };
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append Hop-Limit with 2 bytes (invalid) */
	ret = coap_packet_append_option(&cpkt, COAP_OPTION_HOP_LIMIT,
					invalid_value, 2);
	zassert_equal(ret, 0, "Failed to append option");

	/* Get should reject invalid length */
	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, -EINVAL, "Invalid length should return -EINVAL");

	/* Test with 0-byte length */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_packet_append_option(&cpkt, COAP_OPTION_HOP_LIMIT, NULL, 0);
	zassert_equal(ret, 0, "Failed to append option");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, -EINVAL, "Zero length should return -EINVAL");
}

ZTEST(coap, test_hop_limit_get_invalid_value)
{
	/* RFC 8768 Section 3: Hop-Limit value 0 is invalid */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	uint8_t zero_value = 0;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append Hop-Limit with value 0 (invalid) */
	ret = coap_packet_append_option(&cpkt, COAP_OPTION_HOP_LIMIT,
					&zero_value, 1);
	zassert_equal(ret, 0, "Failed to append option");

	/* Get should reject value 0 */
	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, -EINVAL, "Value 0 should return -EINVAL");
}

ZTEST(coap, test_hop_limit_proxy_update_decrement)
{
	/* RFC 8768 Section 3: Proxy must decrement Hop-Limit by 1 */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	int ret;

	/* Test decrement from 10 to 9 */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_hop_limit(&cpkt, 10);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=10");

	ret = coap_hop_limit_proxy_update(&cpkt, 0);
	zassert_equal(ret, 0, "Failed to decrement Hop-Limit");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 9, "Hop-Limit should be decremented to 9");

	/* Test decrement from 2 to 1 */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_hop_limit(&cpkt, 2);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=2");

	ret = coap_hop_limit_proxy_update(&cpkt, 0);
	zassert_equal(ret, 0, "Failed to decrement Hop-Limit");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 1, "Hop-Limit should be decremented to 1");
}

ZTEST(coap, test_hop_limit_proxy_update_exhaustion)
{
	/* RFC 8768 Section 3: Proxy must not forward if Hop-Limit becomes 0 */
	uint8_t buf[128];
	struct coap_packet cpkt;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_append_hop_limit(&cpkt, 1);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=1");

	/* Decrementing from 1 should signal exhaustion */
	ret = coap_hop_limit_proxy_update(&cpkt, 0);
	zassert_equal(ret, -EHOSTUNREACH,
		      "Hop-Limit 1->0 should return -EHOSTUNREACH");
}

ZTEST(coap, test_hop_limit_proxy_update_insert)
{
	/* RFC 8768 Section 3: Proxy may insert Hop-Limit if absent */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	int ret;

	/* Test insert with default 16 */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* No Hop-Limit present, insert with default */
	ret = coap_hop_limit_proxy_update(&cpkt, 0);
	zassert_equal(ret, 0, "Failed to insert Hop-Limit");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 16, "Default Hop-Limit should be 16");

	/* Test insert with custom default - use direct append first to verify encoding */
	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1235);
	zassert_equal(ret, 0, "Failed to init packet");

	/* First verify direct append works */
	ret = coap_append_hop_limit(&cpkt, 32);
	zassert_equal(ret, 0, "Failed to append Hop-Limit=32");

	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit after direct append");
	zassert_equal(hop_limit, 32, "Direct append should give 32");

	/* Now test via proxy_update with custom default */
	uint8_t buf2[128];
	struct coap_packet cpkt2;

	ret = coap_packet_init(&cpkt2, buf2, sizeof(buf2), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1236);
	zassert_equal(ret, 0, "Failed to init packet");

	ret = coap_hop_limit_proxy_update(&cpkt2, 32);
	zassert_equal(ret, 0, "Failed to insert Hop-Limit via proxy_update");

	ret = coap_get_hop_limit(&cpkt2, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit after proxy_update");
	zassert_equal(hop_limit, 32, "Proxy_update with custom default should give 32");
}

ZTEST(coap, test_hop_limit_multiple_options)
{
	/* RFC 7252 Section 5.4.5: Hop-Limit is not repeatable.
	 * Only the first occurrence should be processed.
	 */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t hop_limit;
	uint8_t value1 = 10;
	uint8_t value2 = 20;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append two Hop-Limit options */
	ret = coap_packet_append_option(&cpkt, COAP_OPTION_HOP_LIMIT, &value1, 1);
	zassert_equal(ret, 0, "Failed to append first Hop-Limit");

	ret = coap_packet_append_option(&cpkt, COAP_OPTION_HOP_LIMIT, &value2, 1);
	zassert_equal(ret, 0, "Failed to append second Hop-Limit");

	/* Get should return only the first value */
	ret = coap_get_hop_limit(&cpkt, &hop_limit);
	zassert_equal(ret, 0, "Failed to get Hop-Limit");
	zassert_equal(hop_limit, 10, "Should return first Hop-Limit value");
}

ZTEST(coap, test_hop_limit_proxy_update_with_invalid)
{
	/* RFC 8768 Section 3: Proxy should reject invalid Hop-Limit */
	uint8_t buf[128];
	struct coap_packet cpkt;
	uint8_t zero_value = 0;
	int ret;

	ret = coap_packet_init(&cpkt, buf, sizeof(buf), COAP_VERSION_1,
			       COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0x1234);
	zassert_equal(ret, 0, "Failed to init packet");

	/* Append invalid Hop-Limit=0 */
	ret = coap_packet_append_option(&cpkt, COAP_OPTION_HOP_LIMIT, &zero_value, 1);
	zassert_equal(ret, 0, "Failed to append option");

	/* Proxy update should detect invalid value */
	ret = coap_hop_limit_proxy_update(&cpkt, 0);
	zassert_equal(ret, -EINVAL, "Should reject invalid Hop-Limit=0");
}


