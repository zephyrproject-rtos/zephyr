/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "helpers.h"
#include "test_objects.h"

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_c_reset();
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_READ, "public"));
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_WRITE, "private"));
}

ZTEST(snmp_pdu, test_get_v2c)
{
	const struct req_vb vbs[] = {
		VB_NULL(TEST_OID_A, 0),       VB_NULL(TEST_OID_B, 0),
		VB_NULL(TEST_OID_T_COL2, 10), VB_NULL(TEST_OID_U_COL1, 2, 0),
		VB_NULL(TEST_BASE, 9, 0),     VB_NULL(TEST_OID_A),
		VB_NULL(TEST_OID_T_COL1, 6),
	};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
			      &rsp) > 0);
	expect_ok(&rsp, ARRAY_SIZE(vbs));

	expect_oid(&rsp.vb[0], OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0));
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_INTEGER);
	zassert_equal(rsp.vb[0].i32, 1);

	zassert_equal(rsp.vb[1].tag, SNMP_BER_TAG_OCTET_STRING);
	zassert_equal(rsp.vb[1].str_len, 1);
	zassert_equal(rsp.vb[1].str[0], 'b');

	zassert_equal(rsp.vb[2].tag, SNMP_BER_TAG_GAUGE32);
	zassert_equal(rsp.vb[2].u32, 10);

	zassert_equal(rsp.vb[3].tag, SNMP_BER_TAG_COUNTER64);
	zassert_equal(rsp.vb[3].u64, 200);

	expect_oid(&rsp.vb[4], OID(TEST_BASE, 9, 0), OID_LEN(TEST_BASE, 9, 0));
	zassert_equal(rsp.vb[4].tag, SNMP_BER_TAG_NO_SUCH_OBJECT);
	zassert_equal(rsp.vb[5].tag, SNMP_BER_TAG_NO_SUCH_INSTANCE);
	zassert_equal(rsp.vb[6].tag, SNMP_BER_TAG_NO_SUCH_INSTANCE);
}

ZTEST(snmp_pdu, test_get_v1)
{
	const struct req_vb ok[] = {
		VB_NULL(TEST_OID_A, 0),
		VB_NULL(TEST_OID_T_COL1, 5),
	};
	const struct req_vb bad[] = {
		VB_NULL(TEST_OID_A, 0),
		VB_NULL(TEST_OID_T_COL1, 6),
		VB_NULL(TEST_OID_B, 0),
	};
	struct response rsp;

	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, ok, ARRAY_SIZE(ok),
			      &rsp) > 0);
	expect_ok(&rsp, 2);
	zassert_equal(rsp.vb[1].i32, 50);

	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, bad, ARRAY_SIZE(bad),
			      &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 2);
	expect_echo(&rsp, bad, ARRAY_SIZE(bad));
}

ZTEST(snmp_pdu, test_getnext)
{
	const struct req_vb vbs[] = {
		VB_NULL(TEST_BASE),
		VB_NULL(TEST_OID_T_COL1, 5),
		VB_NULL(TEST_OID_C, 0),
	};
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, vbs,
			      ARRAY_SIZE(vbs), &rsp) > 0);
	expect_ok(&rsp, 3);
	expect_oid(&rsp.vb[0], OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0));
	zassert_equal(rsp.vb[0].i32, 1);
	expect_oid(&rsp.vb[1], OID(TEST_OID_T_COL1, 10), OID_LEN(TEST_OID_T_COL1, 10));
	zassert_equal(rsp.vb[1].i32, 100);
	expect_oid(&rsp.vb[2], OID(TEST_OID_C, 0), OID_LEN(TEST_OID_C, 0));
	zassert_equal(rsp.vb[2].tag, SNMP_BER_TAG_END_OF_MIB_VIEW);

	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, vbs,
			      ARRAY_SIZE(vbs), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 3);
	expect_echo(&rsp, vbs, ARRAY_SIZE(vbs));
}

ZTEST(snmp_pdu, test_getbulk)
{
	const struct req_vb one_each[] = {
		VB_NULL(TEST_OID_A),
		VB_NULL(TEST_OID_T_COL1),
	};
	const struct req_vb two_repeaters[] = {
		VB_NULL(TEST_OID_T_COL1),
		VB_NULL(TEST_OID_U_COL1),
	};
	const struct req_vb at_end[] = {
		VB_NULL(TEST_OID_C),
	};
	struct response rsp;

	/* One non-repeater, one repeater, three repetitions */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 1, 3, one_each,
			      ARRAY_SIZE(one_each), &rsp) > 0);
	expect_ok(&rsp, 4);
	expect_oid(&rsp.vb[0], OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0));
	expect_oid(&rsp.vb[1], OID(TEST_OID_T_COL1, 2), OID_LEN(TEST_OID_T_COL1, 2));
	expect_oid(&rsp.vb[2], OID(TEST_OID_T_COL1, 5), OID_LEN(TEST_OID_T_COL1, 5));
	expect_oid(&rsp.vb[3], OID(TEST_OID_T_COL1, 10), OID_LEN(TEST_OID_T_COL1, 10));
	zassert_equal(rsp.vb[3].i32, 100);

	/* Two repeaters interleave row by row */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 4, two_repeaters,
			      ARRAY_SIZE(two_repeaters), &rsp) > 0);
	expect_ok(&rsp, 8);
	expect_oid(&rsp.vb[0], OID(TEST_OID_T_COL1, 2), OID_LEN(TEST_OID_T_COL1, 2));
	expect_oid(&rsp.vb[1], OID(TEST_OID_U_COL1, 1, 9), OID_LEN(TEST_OID_U_COL1, 1, 9));
	expect_oid(&rsp.vb[2], OID(TEST_OID_T_COL1, 5), OID_LEN(TEST_OID_T_COL1, 5));
	expect_oid(&rsp.vb[3], OID(TEST_OID_U_COL1, 1, 10), OID_LEN(TEST_OID_U_COL1, 1, 10));
	expect_oid(&rsp.vb[4], OID(TEST_OID_T_COL1, 10), OID_LEN(TEST_OID_T_COL1, 10));
	expect_oid(&rsp.vb[5], OID(TEST_OID_U_COL1, 2, 0), OID_LEN(TEST_OID_U_COL1, 2, 0));
	expect_oid(&rsp.vb[6], OID(TEST_OID_T_COL2, 2), OID_LEN(TEST_OID_T_COL2, 2));
	/* Past the last row of u comes the next object in the registry */
	expect_oid(&rsp.vb[7], OID(TEST_OID_D, 0), OID_LEN(TEST_OID_D, 0));
	zassert_equal(rsp.vb[7].tag, SNMP_BER_TAG_INTEGER);
	zassert_equal(rsp.vb[7].i32, 42);

	/* Repetitions stop once every repeater reached the end of the view */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 10, at_end,
			      ARRAY_SIZE(at_end), &rsp) > 0);
	expect_ok(&rsp, 2);
	expect_oid(&rsp.vb[0], OID(TEST_OID_C, 0), OID_LEN(TEST_OID_C, 0));
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_INTEGER);
	expect_oid(&rsp.vb[1], OID(TEST_OID_C, 0), OID_LEN(TEST_OID_C, 0));
	zassert_equal(rsp.vb[1].tag, SNMP_BER_TAG_END_OF_MIB_VIEW);

	/* Negative counts are treated as zero */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, -1, -5, at_end,
			      ARRAY_SIZE(at_end), &rsp) > 0);
	expect_ok(&rsp, 0);

	/* GETBULK does not exist in SNMPv1 */
	zassert_equal(exchange(V1, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 1, at_end,
			       ARRAY_SIZE(at_end), &rsp),
		      0);
}

ZTEST(snmp_pdu, test_getbulk_truncated)
{
	const struct req_vb vbs[] = {
		VB_NULL(TEST_BASE),
	};
	size_t req_start, req_len, rsp_start, rsp_len;
	struct response rsp;

	req_len = build_request(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 100, vbs,
				ARRAY_SIZE(vbs), &req_start);

	/* A buffer for about four bindings yields a shorter, valid answer */
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, 120, &rsp_start);
	zassert_true(rsp_len > 0);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	zassert_equal(rsp.error_status, SNMP_ERR_NO_ERROR);
	zassert_true(rsp.vb_count > 0 && rsp.vb_count < 12, "%zu bindings", rsp.vb_count);
	expect_oid(&rsp.vb[0], OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0));
}

/* The values answered for these bindings take one byte more each than the
 * NULL placeholders in the request, so a buffer of the request's size holds
 * the echoed bindings of an error response but not the answered ones.
 */
ZTEST(snmp_pdu, test_too_big)
{
	const struct req_vb vbs[] = {
		VB_NULL(TEST_OID_A, 0),
		VB_NULL(TEST_OID_B, 0),
		VB_NULL(TEST_OID_C, 0),
	};
	size_t req_start, req_len, rsp_start, rsp_len;
	struct response rsp;

	req_len = build_request(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
				&req_start);
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len, &rsp_start);
	zassert_true(rsp_len > 0);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	expect_error(&rsp, SNMP_ERR_TOO_BIG, 0);
	zassert_equal(rsp.vb_count, 0, "SNMPv2c tooBig carries no bindings");

	req_len = build_request(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
				&req_start);
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len, &rsp_start);
	zassert_true(rsp_len > 0);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	expect_error(&rsp, SNMP_ERR_TOO_BIG, 0);
	expect_echo(&rsp, vbs, ARRAY_SIZE(vbs));

	/* Not even the echo fits: nothing is sent */
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len - 1,
				       &rsp_start);
	zassert_equal(rsp_len, 0);
}

/* An error response is framed exactly like its request, so it fits a buffer
 * of the request's size; the reserved head must not waste that margin.
 */
ZTEST(snmp_pdu, test_error_response_exact_fit)
{
	const struct req_vb vbs[] = {
		VB_NULL(TEST_BASE, 9, 0),
		VB_NULL(TEST_OID_A, 0),
		VB_NULL(TEST_OID_B, 0),
	};
	size_t req_start, req_len, rsp_start, rsp_len;
	struct response rsp;

	req_len = build_request(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, vbs, ARRAY_SIZE(vbs),
				&req_start);
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len, &rsp_start);
	zassert_equal(rsp_len, req_len, "Response of %zu bytes for a %zu byte request", rsp_len,
		      req_len);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 1);
	expect_echo(&rsp, vbs, ARRAY_SIZE(vbs));

	/* The same holds for a successful SET, which also echoes its bindings */
	{
		const struct req_vb set_c[] = {VB_INT(9, TEST_OID_C, 0)};

		req_len = build_request(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
					ARRAY_SIZE(set_c), &req_start);
		rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len,
					       &rsp_start);
		zassert_equal(rsp_len, req_len);
		zassert_equal(test_c_value(), 9);
	}
}

ZTEST(snmp_pdu, test_set)
{
	const struct req_vb set_c[] = {VB_INT(7, TEST_OID_C, 0)};
	const struct req_vb wrong_type[] = {VB_STR("x", TEST_OID_C, 0)};
	const struct req_vb wrong_value[] = {VB_INT(200, TEST_OID_C, 0)};
	const struct req_vb read_only[] = {VB_INT(1, TEST_OID_A, 0)};
	const struct req_vb missing[] = {VB_INT(1, TEST_BASE, 9, 0)};
	const struct req_vb two_phase[] = {VB_INT(5, TEST_OID_C, 0), VB_INT(1, TEST_OID_A, 0)};
	const struct req_vb unsupported[] = {VB_TAG(SNMP_BER_TAG_OPAQUE, TEST_OID_C, 0)};
	struct response rsp;

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			      ARRAY_SIZE(set_c), &rsp) > 0);
	expect_ok(&rsp, 1);
	expect_echo(&rsp, set_c, ARRAY_SIZE(set_c));
	zassert_equal(rsp.vb[0].i32, 7);
	zassert_equal(test_c_value(), 7);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, wrong_type,
			      ARRAY_SIZE(wrong_type), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_TYPE, 1);
	expect_echo(&rsp, wrong_type, ARRAY_SIZE(wrong_type));
	zassert_equal(test_c_value(), 7);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, wrong_value,
			      ARRAY_SIZE(wrong_value), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_VALUE, 1);
	zassert_equal(test_c_value(), 7);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, read_only,
			      ARRAY_SIZE(read_only), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NOT_WRITABLE, 1);

	/* RFC 3416 4.2.5 (2): nothing under that prefix can be written */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, missing,
			      ARRAY_SIZE(missing), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NOT_WRITABLE, 1);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, unsupported,
			      ARRAY_SIZE(unsupported), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_TYPE, 1);

	/* The second binding fails validation, so the first is not applied */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, two_phase,
			      ARRAY_SIZE(two_phase), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NOT_WRITABLE, 2);
	expect_echo(&rsp, two_phase, ARRAY_SIZE(two_phase));
	zassert_equal(test_c_value(), 7, "Value applied although the request failed");

	/* SNMPv1 reports the same failures with its own codes */
	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, wrong_type,
			      ARRAY_SIZE(wrong_type), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_BAD_VALUE, 1);
	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, read_only,
			      ARRAY_SIZE(read_only), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 1);

	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			      ARRAY_SIZE(set_c), &rsp) > 0);
	expect_ok(&rsp, 1);
}

/* RFC 3416 4.2.5 (4): a value that does not decode as its tag demands. */
ZTEST(snmp_pdu, test_set_wrong_encoding)
{
	/* SET of 1.3.6.1.7.1.5.0 to an INTEGER of five content bytes, 2^31 */
	static const uint8_t request[] = {0x30, 0x2B, 0x02, 0x01, 0x01, 0x04, 0x07, 'p',  'r',
					  'i',  'v',  'a',  't',  'e',  0xA3, 0x1D, 0x02, 0x01,
					  0x01, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00, 0x30, 0x12,
					  0x30, 0x10, 0x06, 0x07, 0x2B, 0x06, 0x01, 0x07, 0x01,
					  0x05, 0x00, 0x02, 0x05, 0x00, 0x80, 0x00, 0x00, 0x00};
	/* Offset and length of the binding list content in request[] */
	const size_t vbs_off = 27, vbs_len = 18;
	int32_t before = test_c_value();
	size_t rsp_start, rsp_len;
	struct response rsp;

	zassert_equal(request[vbs_off], SNMP_BER_TAG_SEQUENCE);
	zassert_equal(vbs_off + vbs_len, sizeof(request));

	rsp_len = snmp_process_message(request, sizeof(request), rsp_buf, sizeof(rsp_buf),
				       &rsp_start);
	zassert_true(rsp_len > 0);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	expect_error(&rsp, SNMP_ERR_WRONG_ENCODING, 1);
	zassert_equal(rsp.raw_vbs_len, vbs_len);
	zassert_mem_equal(rsp.raw_vbs, &request[vbs_off], vbs_len);
	zassert_equal(test_c_value(), before);

	/* SNMPv1 reports it as badValue */
	memcpy(req_buf, request, sizeof(request));
	req_buf[4] = V1;
	rsp_len = snmp_process_message(req_buf, sizeof(request), rsp_buf, sizeof(rsp_buf),
				       &rsp_start);
	zassert_true(rsp_len > 0);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	expect_error(&rsp, SNMP_ERR_BAD_VALUE, 1);
	zassert_equal(test_c_value(), before);
}

/* RFC 3584 4.2.2.1: Counter64 instances are not in view for SNMPv1. */
ZTEST(snmp_pdu, test_v1_counter64)
{
	const struct req_vb get[] = {
		VB_NULL(TEST_OID_A, 0),
		VB_NULL(TEST_OID_U_COL1, 1, 9),
	};
	const struct req_vb next_into[] = {VB_NULL(TEST_OID_T_COL2, 10)};
	const struct req_vb next_within[] = {VB_NULL(TEST_OID_U_COL1, 1, 9)};
	const struct req_vb set[] = {VB_INT(1, TEST_OID_U_COL1, 1, 9)};
	const struct req_vb carries[] = {VB_TAG(SNMP_BER_TAG_COUNTER64, TEST_OID_A, 0)};
	struct response rsp;

	/* GET answers noSuchName for the Counter64 binding */
	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get, ARRAY_SIZE(get),
			      &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 2);
	expect_echo(&rsp, get, ARRAY_SIZE(get));

	/* GETNEXT skips the whole Counter64 column and lands on the next scalar */
	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next_into,
			      ARRAY_SIZE(next_into), &rsp) > 0);
	expect_ok(&rsp, 1);
	expect_oid(&rsp.vb[0], OID(TEST_OID_D, 0), OID_LEN(TEST_OID_D, 0));

	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next_within,
			      ARRAY_SIZE(next_within), &rsp) > 0);
	expect_ok(&rsp, 1);
	expect_oid(&rsp.vb[0], OID(TEST_OID_D, 0), OID_LEN(TEST_OID_D, 0));

	/* SNMPv2c still sees the column */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next_into,
			      ARRAY_SIZE(next_into), &rsp) > 0);
	expect_ok(&rsp, 1);
	expect_oid(&rsp.vb[0], OID(TEST_OID_U_COL1, 1, 9), OID_LEN(TEST_OID_U_COL1, 1, 9));
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_COUNTER64);

	/* SET on a Counter64 instance is noSuchName, not badValue */
	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set, ARRAY_SIZE(set),
			      &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 1);

	/* A version 1 request carrying a Counter64 value is ill-formed and dropped */
	zassert_equal(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, carries,
			       ARRAY_SIZE(carries), &rsp),
		      0);
	zassert_equal(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, carries,
			       ARRAY_SIZE(carries), &rsp),
		      0);
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, carries,
			      ARRAY_SIZE(carries), &rsp) > 0,
		     "The same request is fine in version 2c");
	expect_ok(&rsp, 1);
}

/* RFC 3416 4.2.5: every binding is validated before any is applied. */
ZTEST(snmp_pdu, test_set_two_phase)
{
	const struct req_vb bad_second[] = {VB_INT(5, TEST_OID_C, 0), VB_INT(200, TEST_OID_D, 0)};
	const struct req_vb commit_fails_second[] = {VB_INT(5, TEST_OID_C, 0),
						     VB_INT(TEST_E_COMMIT_FAILS, TEST_OID_E, 0)};
	const struct req_vb commit_fails_first[] = {VB_INT(TEST_E_COMMIT_FAILS, TEST_OID_E, 0),
						    VB_INT(5, TEST_OID_C, 0)};
	const struct req_vb all_good[] = {VB_INT(5, TEST_OID_C, 0), VB_INT(6, TEST_OID_D, 0),
					  VB_INT(7, TEST_OID_E, 0)};
	const struct req_vb set_c[] = {VB_INT(7, TEST_OID_C, 0)};
	size_t req_start, req_len, rsp_start, rsp_len;
	struct response rsp;

	/* The range error of the second binding prevents the first from being applied */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, bad_second,
			      ARRAY_SIZE(bad_second), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_WRONG_VALUE, 2);
	expect_echo(&rsp, bad_second, ARRAY_SIZE(bad_second));
	zassert_equal(test_c_value(), 42, "Value applied although a later check failed");
	zassert_equal(test_d_value(), 42);

	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, bad_second,
			      ARRAY_SIZE(bad_second), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_BAD_VALUE, 2);
	zassert_equal(test_c_value(), 42);

	/* A commit failure after another commit: undoFailed with index 0, and no rollback */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, commit_fails_second,
			      ARRAY_SIZE(commit_fails_second), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_UNDO_FAILED, 0);
	expect_echo(&rsp, commit_fails_second, ARRAY_SIZE(commit_fails_second));
	zassert_equal(test_c_value(), 5, "The committed binding stays applied");
	zassert_equal(test_e_value(), 42);
	test_c_reset();

	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, commit_fails_second,
			      ARRAY_SIZE(commit_fails_second), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 0);
	test_c_reset();

	/* A commit failure of the first binding: commitFailed with its index */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, commit_fails_first,
			      ARRAY_SIZE(commit_fails_first), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_COMMIT_FAILED, 1);
	zassert_equal(test_c_value(), 42, "Later bindings are not committed after a failure");
	zassert_equal(test_e_value(), 42);

	/* All checks pass: everything is applied in order */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, all_good,
			      ARRAY_SIZE(all_good), &rsp) > 0);
	expect_ok(&rsp, 3);
	zassert_equal(test_c_value(), 5);
	zassert_equal(test_d_value(), 6);
	zassert_equal(test_e_value(), 7);
	test_c_reset();

	/* The response must be able to repeat the bindings; that is checked
	 * before anything is changed.
	 */
	req_len = build_request(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
				ARRAY_SIZE(set_c), &req_start);
	rsp_len = snmp_process_message(&req_buf[req_start], req_len, rsp_buf, req_len - 1,
				       &rsp_start);
	zassert_true(rsp_len > 0);
	parse_response(&rsp_buf[rsp_start], rsp_len, &rsp);
	expect_error(&rsp, SNMP_ERR_TOO_BIG, 0);
	zassert_equal(rsp.vb_count, 0);
	zassert_equal(test_c_value(), 42, "Value applied although the response cannot be sent");
}

/* RFC 1157 4.1: a request that does not parse is discarded, not answered. */
ZTEST(snmp_pdu, test_malformed_bindings)
{
	const struct req_vb get_a[] = {VB_NULL(TEST_OID_A, 0)};
	const struct req_vb two[] = {VB_NULL(TEST_OID_A, 0), VB_NULL(TEST_OID_B, 0)};
	/* GET for 1.3.6.1.7.1.1.0 whose binding carries one byte after the NULL value */
	static const uint8_t trailing[] = {
		0x30, 0x26, 0x02, 0x01, 0x01, 0x04, 0x06, 'p',  'u',  'b',  'l',  'i',  'c',  0xA0,
		0x19, 0x02, 0x01, 0x01, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00, 0x30, 0x0E, 0x30, 0x0C,
		0x06, 0x07, 0x2B, 0x06, 0x01, 0x07, 0x01, 0x01, 0x00, 0x05, 0x00, 0x00};
	uint32_t long_name[SNMP_OID_MAX_LEN + 1];
	size_t req_start, req_len, rsp_start;
	struct response rsp;

	/* The second binding's name is not an OID: nothing is answered, not
	 * even the first binding.
	 */
	req_len = build_request(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, two, ARRAY_SIZE(two),
				&req_start);
	zassert_equal(req_buf[req_start + req_len - 11], SNMP_BER_TAG_OID,
		      "Test assumes the last binding's name starts 11 bytes from the end");
	req_buf[req_start + req_len - 11] = SNMP_BER_TAG_OCTET_STRING;
	zassert_equal(snmp_process_message(&req_buf[req_start], req_len, rsp_buf, sizeof(rsp_buf),
					   &rsp_start),
		      0);

	/* A name longer than the agent can represent */
	for (size_t i = 0; i < ARRAY_SIZE(long_name); i++) {
		long_name[i] = (i < 2) ? 1 : (uint32_t)i;
	}
	{
		const struct req_vb overlong[] = {{long_name, ARRAY_SIZE(long_name), {0}}};

		zassert_equal(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, overlong,
				       ARRAY_SIZE(overlong), &rsp),
			      0);
	}

	/* Trailing bytes inside a binding */
	zassert_equal(snmp_process_message(trailing, sizeof(trailing), rsp_buf, sizeof(rsp_buf),
					   &rsp_start),
		      0);

	/* The same request without the extra byte is answered */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			      ARRAY_SIZE(get_a), &rsp) > 0);
	expect_ok(&rsp, 1);
}

/* RFC 3416 4.2.5 (2) and (7): notWritable versus noCreation for rows. */
ZTEST(snmp_pdu, test_set_rows)
{
	const struct req_vb no_row_writable[] = {VB_INT(1, TEST_OID_T_COL1, 6)};
	const struct req_vb no_row_read_only[] = {VB_INT(1, TEST_OID_T_COL2, 6)};
	const struct req_vb set_row[] = {VB_INT(55, TEST_OID_T_COL1, 5)};
	const struct req_vb restore_row[] = {VB_INT(50, TEST_OID_T_COL1, 5)};
	const struct req_vb get_row[] = {VB_NULL(TEST_OID_T_COL1, 5)};
	struct response rsp;

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, no_row_writable,
			      ARRAY_SIZE(no_row_writable), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_CREATION, 1);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, no_row_read_only,
			      ARRAY_SIZE(no_row_read_only), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NOT_WRITABLE, 1);

	/* Both map to noSuchName in version 1 */
	zassert_true(exchange(V1, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, no_row_writable,
			      ARRAY_SIZE(no_row_writable), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 1);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_row,
			      ARRAY_SIZE(set_row), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_row,
			      ARRAY_SIZE(get_row), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].i32, 55);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, restore_row,
			      ARRAY_SIZE(restore_row), &rsp) > 0);
	expect_ok(&rsp, 1);
}

/* Getter failures: -ENOENT hides the instance, anything else is genErr. */
ZTEST(snmp_pdu, test_getter_errors)
{
	const struct req_vb get_e[] = {VB_NULL(TEST_OID_E, 0)};
	const struct req_vb next_d[] = {VB_NULL(TEST_OID_D, 0)};
	const struct req_vb set_e[] = {VB_INT(1, TEST_OID_E, 0)};
	struct response rsp;

	test_set_get_mode(TEST_GET_EIO);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_e,
			      ARRAY_SIZE(get_e), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 1);
	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_e,
			      ARRAY_SIZE(get_e), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 1);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next_d,
			      ARRAY_SIZE(next_d), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 1);

	/* GETBULK reports the index of the binding in the request */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 2, next_d,
			      ARRAY_SIZE(next_d), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 1);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_e,
			      ARRAY_SIZE(set_e), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_GEN_ERR, 1);

	test_set_get_mode(TEST_GET_ENOENT);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_e,
			      ARRAY_SIZE(get_e), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_NO_SUCH_INSTANCE);
	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_e,
			      ARRAY_SIZE(get_e), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 1);

	/* Both e and c vanish; the end of view names the request, not the
	 * last instance that was tried.
	 */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next_d,
			      ARRAY_SIZE(next_d), &rsp) > 0);
	expect_ok(&rsp, 1);
	expect_oid(&rsp.vb[0], OID(TEST_OID_D, 0), OID_LEN(TEST_OID_D, 0));
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_END_OF_MIB_VIEW);

	zassert_true(exchange(V1, "public", SNMP_BER_TAG_GET_NEXT_REQUEST, 0, 0, next_d,
			      ARRAY_SIZE(next_d), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 1);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 0, 3, next_d,
			      ARRAY_SIZE(next_d), &rsp) > 0);
	expect_ok(&rsp, 1);
	expect_oid(&rsp.vb[0], OID(TEST_OID_D, 0), OID_LEN(TEST_OID_D, 0));
	zassert_equal(rsp.vb[0].tag, SNMP_BER_TAG_END_OF_MIB_VIEW);

	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_e,
			      ARRAY_SIZE(set_e), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_CREATION, 1);
}

/* RFC 3416 4.2.3 allows fewer repetitions than requested; a request with
 * more repeaters than the agent keeps state for gets exactly one.
 */
ZTEST(snmp_pdu, test_getbulk_many_repeaters)
{
	static const uint32_t a_oid[] = {TEST_OID_A};
	struct req_vb many[CONFIG_SNMP_MAX_VARBINDS + 2];
	struct response rsp;

	for (size_t i = 0; i < ARRAY_SIZE(many); i++) {
		many[i].oid = a_oid;
		many[i].oid_len = ARRAY_SIZE(a_oid);
		many[i].value.tag = 0;
	}

	/* One non-repeater and one too many repeaters, five repetitions asked */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 1, 5, many,
			      ARRAY_SIZE(many), &rsp) > 0);
	expect_ok(&rsp, ARRAY_SIZE(many));
	for (size_t i = 0; i < ARRAY_SIZE(many); i++) {
		expect_oid(&rsp.vb[i], OID(TEST_OID_A, 0), OID_LEN(TEST_OID_A, 0));
	}

	/* No repetitions asked: only the non-repeater is answered */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 1, 0, many,
			      ARRAY_SIZE(many), &rsp) > 0);
	expect_ok(&rsp, 1);

	/* Exactly the supported number of repeaters takes the stateful path */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_BULK_REQUEST, 2, 1, many,
			      ARRAY_SIZE(many), &rsp) > 0);
	expect_ok(&rsp, 2 + CONFIG_SNMP_MAX_VARBINDS);
}

ZTEST(snmp_pdu, test_set_access)
{
	const struct req_vb set_c[] = {VB_INT(9, TEST_OID_C, 0)};
	const struct req_vb get_c[] = {VB_NULL(TEST_OID_C, 0)};
	struct response rsp;

	/* The read community cannot write */
	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			      ARRAY_SIZE(set_c), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_AUTHORIZATION_ERROR, 0);
	expect_echo(&rsp, set_c, ARRAY_SIZE(set_c));
	zassert_equal(test_c_value(), 42);

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_SET_REQUEST, 0, 0, NULL, 0, &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_AUTHORIZATION_ERROR, 0);

	zassert_true(exchange(V1, "public", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			      ARRAY_SIZE(set_c), &rsp) > 0);
	expect_error(&rsp, SNMP_ERR_NO_SUCH_NAME, 0);

	/* The write community can read */
	zassert_true(exchange(V2C, "private", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_c,
			      ARRAY_SIZE(get_c), &rsp) > 0);
	expect_ok(&rsp, 1);
	zassert_equal(rsp.vb[0].i32, 42);

	/* An empty write community disables SET */
	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_WRITE, ""));
	zassert_equal(
		exchange(V2C, "", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c, ARRAY_SIZE(set_c), &rsp),
		0, "Empty community must not match");
	zassert_equal(exchange(V2C, "private", SNMP_BER_TAG_SET_REQUEST, 0, 0, set_c,
			       ARRAY_SIZE(set_c), &rsp),
		      0);
}

ZTEST(snmp_pdu, test_community)
{
	const struct req_vb get_a[] = {VB_NULL(TEST_OID_A, 0)};
	char too_long[CONFIG_SNMP_COMMUNITY_MAX_LEN + 2];
	struct response rsp;

	zassert_equal(exchange(V2C, "wrong", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);
	zassert_equal(exchange(V2C, "publi", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0, "Prefix of the community must not match");
	zassert_equal(exchange(V2C, "publicx", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);

	zassert_ok(snmp_agent_set_community(SNMP_COMMUNITY_READ, "zephyr"));
	zassert_equal(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);
	zassert_true(exchange(V2C, "zephyr", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			      ARRAY_SIZE(get_a), &rsp) > 0);
	expect_ok(&rsp, 1);

	memset(too_long, 'a', sizeof(too_long) - 1);
	too_long[sizeof(too_long) - 1] = '\0';
	zassert_equal(snmp_agent_set_community(SNMP_COMMUNITY_READ, too_long), -EMSGSIZE);
	zassert_equal(snmp_agent_set_community(SNMP_COMMUNITY_READ, NULL), -EINVAL);
	zassert_equal(snmp_agent_set_community((enum snmp_community)7, "x"), -EINVAL);
}

ZTEST(snmp_pdu, test_dropped_messages)
{
	const struct req_vb get_a[] = {VB_NULL(TEST_OID_A, 0)};
	static const uint8_t garbage[] = {0x30, 0x03, 0x02, 0x01};
	static const uint8_t not_a_sequence[] = {0x04, 0x01, 0x00};
	size_t req_start, req_len, rsp_start;
	struct response rsp;

	zassert_equal(snmp_process_message(garbage, sizeof(garbage), rsp_buf, sizeof(rsp_buf),
					   &rsp_start),
		      0);
	zassert_equal(snmp_process_message(not_a_sequence, sizeof(not_a_sequence), rsp_buf,
					   sizeof(rsp_buf), &rsp_start),
		      0);
	zassert_equal(snmp_process_message(garbage, 0, rsp_buf, sizeof(rsp_buf), &rsp_start), 0);

	/* Unsupported version */
	zassert_equal(exchange(3, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);

	/* PDU types an agent does not answer */
	zassert_equal(exchange(V2C, "public", SNMP_BER_TAG_RESPONSE, 0, 0, get_a, ARRAY_SIZE(get_a),
			       &rsp),
		      0);
	zassert_equal(
		exchange(V2C, "public", SNMP_BER_TAG_TRAP_V2, 0, 0, get_a, ARRAY_SIZE(get_a), &rsp),
		0);
	zassert_equal(exchange(V2C, "public", SNMP_BER_TAG_INFORM_REQUEST, 0, 0, get_a,
			       ARRAY_SIZE(get_a), &rsp),
		      0);

	/* A truncated request is dropped rather than answered partially */
	req_len = build_request(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, get_a,
				ARRAY_SIZE(get_a), &req_start);
	zassert_equal(snmp_process_message(&req_buf[req_start], req_len - 1, rsp_buf,
					   sizeof(rsp_buf), &rsp_start),
		      0);
}

ZTEST(snmp_pdu, test_empty_request)
{
	struct response rsp;

	zassert_true(exchange(V2C, "public", SNMP_BER_TAG_GET_REQUEST, 0, 0, NULL, 0, &rsp) > 0);
	expect_ok(&rsp, 0);
}

ZTEST_SUITE(snmp_pdu, NULL, NULL, before, NULL, NULL);
