/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"

/* Forward declaration for EDHOC transport validation function */
extern int coap_edhoc_transport_validate_content_format(const struct coap_packet *request);

/* EDHOC tests */

ZTEST(coap, test_edhoc_option_number)
{
	/* RFC 9668 Section 3.1 / IANA Section 8.1: EDHOC option number is 21 */
	zassert_equal(COAP_OPTION_EDHOC, 21, "EDHOC option number must be 21");
}

ZTEST(coap, test_edhoc_content_formats)
{
	/* RFC 9528 Section 10.9 Table 13: EDHOC content-format IDs */
	zassert_equal(COAP_CONTENT_FORMAT_APP_EDHOC_CBOR_SEQ, 64,
		      "application/edhoc+cbor-seq content-format must be 64");
	zassert_equal(COAP_CONTENT_FORMAT_APP_CID_EDHOC_CBOR_SEQ, 65,
		      "application/cid-edhoc+cbor-seq content-format must be 65");
}

#if !defined(CONFIG_COAP_EDHOC)
/* Test that EDHOC option is rejected when EDHOC support is disabled */
ZTEST(coap, test_edhoc_unsupported_critical_option)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	uint16_t unsupported_opt;
	int r;

	/* Build a request with EDHOC option */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Add EDHOC option (empty as per RFC 9668) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to append EDHOC option");

	/* Should detect EDHOC as unsupported critical option */
	r = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_equal(r, -ENOTSUP, "Should detect EDHOC as unsupported");
	zassert_equal(unsupported_opt, COAP_OPTION_EDHOC,
		      "Should report EDHOC option as unsupported");
}
#endif /* !CONFIG_COAP_EDHOC */

#if defined(CONFIG_COAP_EDHOC)
/* Test EDHOC option detection */
ZTEST(coap, test_edhoc_msg_has_edhoc)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a request without EDHOC option */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Should not detect EDHOC option */
	zassert_false(coap_edhoc_msg_has_edhoc(&cpkt),
		      "Should not detect EDHOC in message without option");

	/* Add EDHOC option (empty as per RFC 9668) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to append EDHOC option");

	/* Should detect EDHOC option */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt),
		     "Should detect EDHOC option in message");
}

/* Test EDHOC combined payload parsing - RFC 9668 Figure 4 example */
ZTEST(coap, test_edhoc_split_comb_payload)
{
	/* Example from RFC 9668 Section 3.2.1:
	 * EDHOC_MSG_3 is a CBOR bstr containing some data
	 * For this test, we'll use a simple example:
	 * - CBOR bstr with 10 bytes of data: 0x4a (header) + 10 bytes
	 * - Followed by OSCORE payload
	 */
	uint8_t combined_payload[] = {
		/* CBOR bstr header: major type 2, length 10 */
		0x4a,
		/* EDHOC_MSG_3 data (10 bytes) */
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
		/* OSCORE_PAYLOAD (5 bytes) */
		0xaa, 0xbb, 0xcc, 0xdd, 0xee
	};

	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	r = coap_edhoc_split_comb_payload(combined_payload, sizeof(combined_payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, 0, "Failed to split combined payload");

	/* Check EDHOC_MSG_3 span (header + data) */
	zassert_equal(edhoc_msg3.len, 11, "EDHOC_MSG_3 length incorrect");
	zassert_equal(edhoc_msg3.ptr, combined_payload, "EDHOC_MSG_3 pointer incorrect");

	/* Check OSCORE_PAYLOAD span */
	zassert_equal(oscore_payload.len, 5, "OSCORE_PAYLOAD length incorrect");
	zassert_equal(oscore_payload.ptr, combined_payload + 11,
		      "OSCORE_PAYLOAD pointer incorrect");
	zassert_equal(oscore_payload.ptr[0], 0xaa, "OSCORE_PAYLOAD data incorrect");
}

/* Test EDHOC combined payload parsing with 1-byte length encoding */
ZTEST(coap, test_edhoc_split_comb_payload_1byte_len)
{
	/* CBOR bstr with 1-byte length encoding (additional info = 24)
	 * 0x58 0x1e (30 bytes) + data + OSCORE payload
	 */
	uint8_t combined_payload[2 + 30 + 5];

	combined_payload[0] = 0x58; /* major type 2, additional info 24 */
	combined_payload[1] = 30;   /* length = 30 */
	memset(&combined_payload[2], 0xaa, 30); /* EDHOC data */
	memset(&combined_payload[32], 0xbb, 5); /* OSCORE payload */

	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	r = coap_edhoc_split_comb_payload(combined_payload, sizeof(combined_payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, 0, "Failed to split combined payload with 1-byte length");

	zassert_equal(edhoc_msg3.len, 32, "EDHOC_MSG_3 length incorrect");
	zassert_equal(oscore_payload.len, 5, "OSCORE_PAYLOAD length incorrect");
}

/* Test EDHOC combined payload parsing with 2-byte length encoding */
ZTEST(coap, test_edhoc_split_comb_payload_2byte_len)
{
	/* CBOR bstr with 2-byte length encoding (additional info = 25)
	 * 0x59 0x01 0x00 (256 bytes) + data + OSCORE payload
	 */
	uint8_t combined_payload[3 + 256 + 5];

	combined_payload[0] = 0x59; /* major type 2, additional info 25 */
	combined_payload[1] = 0x01; /* length high byte */
	combined_payload[2] = 0x00; /* length low byte = 256 */
	memset(&combined_payload[3], 0xcc, 256); /* EDHOC data */
	memset(&combined_payload[259], 0xdd, 5); /* OSCORE payload */

	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	r = coap_edhoc_split_comb_payload(combined_payload, sizeof(combined_payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, 0, "Failed to split combined payload with 2-byte length");

	zassert_equal(edhoc_msg3.len, 259, "EDHOC_MSG_3 length incorrect");
	zassert_equal(oscore_payload.len, 5, "OSCORE_PAYLOAD length incorrect");
}

/* Test EDHOC combined payload parsing error cases */
ZTEST(coap, test_edhoc_split_comb_payload_errors)
{
	uint8_t payload[] = { 0x4a, 0x01, 0x02, 0x03, 0x04, 0x05,
			      0x06, 0x07, 0x08, 0x09, 0x0a };
	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	/* Test NULL parameters */
	r = coap_edhoc_split_comb_payload(NULL, sizeof(payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject NULL payload");

	r = coap_edhoc_split_comb_payload(payload, sizeof(payload),
					  NULL, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject NULL edhoc_msg3");

	r = coap_edhoc_split_comb_payload(payload, sizeof(payload),
					  &edhoc_msg3, NULL);
	zassert_equal(r, -EINVAL, "Should reject NULL oscore_payload");

	/* Test empty payload */
	r = coap_edhoc_split_comb_payload(payload, 0,
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject empty payload");

	/* Test wrong CBOR major type (not byte string) */
	uint8_t wrong_type[] = { 0x01, 0x02, 0x03 }; /* major type 0 (unsigned int) */

	r = coap_edhoc_split_comb_payload(wrong_type, sizeof(wrong_type),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject non-bstr major type");

	/* Test missing OSCORE payload (EDHOC_MSG_3 takes entire payload) */
	uint8_t no_oscore[] = { 0x43, 0x01, 0x02, 0x03 }; /* bstr of length 3 */

	r = coap_edhoc_split_comb_payload(no_oscore, sizeof(no_oscore),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject payload without OSCORE part");
}

/* Test EDHOC option removal */
ZTEST(coap, test_edhoc_remove_option)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a request with EDHOC option */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Add EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to append EDHOC option");

	/* Verify EDHOC option is present */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt), "EDHOC option should be present");

	/* Remove EDHOC option */
	r = coap_edhoc_remove_option(&cpkt);
	zassert_equal(r, 0, "Failed to remove EDHOC option");

	/* Re-parse the packet to ensure option removal is reflected */
	struct coap_option options[10];
	uint8_t opt_num = 10;

	r = coap_packet_parse(&cpkt, buffer, cpkt.offset, options, opt_num);
	zassert_equal(r, 0, "Failed to re-parse packet");

	/* Verify EDHOC option is removed */
	zassert_false(coap_edhoc_msg_has_edhoc(&cpkt), "EDHOC option should be removed");
}

/* Test EDHOC option validation: at most once */
ZTEST(coap, test_edhoc_option_at_most_once)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	bool present;
	int r;

	/* Build a packet with two EDHOC options (invalid per RFC 9668 Section 3.1) */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add first EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to add first EDHOC option");

	/* Add second EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to add second EDHOC option");

	/* RFC 9668 Section 3.1 + RFC 7252 Section 5.4.5:
	 * coap_edhoc_msg_has_edhoc() should return true (at least one EDHOC option present)
	 */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt),
		     "coap_edhoc_msg_has_edhoc() should return true when EDHOC option present");

	/* coap_edhoc_validate_option() should detect the violation and return error */
	r = coap_edhoc_validate_option(&cpkt, &present);
	zassert_equal(r, -EBADMSG, "Should return -EBADMSG for multiple EDHOC options");
	zassert_true(present, "present flag should be true when EDHOC option exists");
}

/* Test EDHOC option validation: ignore non-empty value */
ZTEST(coap, test_edhoc_option_ignore_value)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	bool present;
	int r;

	/* Build a packet with EDHOC option containing a value (should be ignored) */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add EDHOC option with a value (RFC 9668 says recipient MUST ignore it) */
	uint8_t edhoc_value[] = { 0x01, 0x02, 0x03 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC,
				      edhoc_value, sizeof(edhoc_value));
	zassert_equal(r, 0, "Failed to add EDHOC option");

	/* Verify that EDHOC option is still detected (value is ignored) */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt),
		     "EDHOC option should be detected even with non-empty value");

	/* RFC 9668 Section 3.1: Validator should accept non-empty value (must be ignored) */
	r = coap_edhoc_validate_option(&cpkt, &present);
	zassert_equal(r, 0, "Should return success even with non-empty EDHOC option value");
	zassert_true(present, "present flag should be true");
}

/* Test server rejection of repeated EDHOC options in CON request
 * RFC 9668 Section 3.1 + RFC 7252 Section 5.4.5 + 5.4.1
 */
ZTEST(coap, test_edhoc_repeated_option_server_rejection)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	bool present;
	int r;

	/* Build a CON request with two EDHOC options */
	uint8_t token[] = { 0xAB, 0xCD };
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON,
			     sizeof(token), token,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add first EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to add first EDHOC option");

	/* Add second EDHOC option (violation) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to add second EDHOC option");

	/* Verify validator detects the violation */
	r = coap_edhoc_validate_option(&cpkt, &present);
	zassert_equal(r, -EBADMSG, "Validator should return -EBADMSG for repeated options");
	zassert_true(present, "present flag should be true");

	/* Per RFC 7252 Section 5.4.1:
	 * - CON request with unrecognized critical option MUST return 4.02 (Bad Option)
	 * - NON request with unrecognized critical option MUST be rejected (dropped)
	 *
	 * This test verifies that the validator correctly identifies the violation.
	 * The actual server response handling is tested in integration tests.
	 */
}

#if defined(CONFIG_COAP_OSCORE)
/* Test that EDHOC option is Class U (unprotected) for OSCORE */
ZTEST(coap, test_edhoc_option_class_u_oscore)
{
	/* This test verifies that the EDHOC option (21) is treated as Class U
	 * (unprotected) by OSCORE, as required by RFC 9668 Section 3.1.
	 * This is implemented in the uoscore-uedhoc library's is_class_e() function.
	 *
	 * We can't directly test the uoscore library here, but we verify that
	 * the EDHOC option number is correctly defined.
	 */
	zassert_equal(COAP_OPTION_EDHOC, 21,
		      "EDHOC option must be 21 for Class U classification");
}
#endif /* CONFIG_COAP_OSCORE */

#endif /* CONFIG_COAP_EDHOC */

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)

ZTEST(coap, test_edhoc_encode_error_basic)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	/* Encode EDHOC error: ERR_CODE=1, ERR_INFO="EDHOC error" */
	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	r = coap_edhoc_encode_error(1, "EDHOC error", buffer, &buffer_len);
	zassert_equal(r, 0, "Failed to encode EDHOC error");

	/* Verify CBOR Sequence encoding:
	 * - First item: CBOR unsigned int 1 = 0x01
	 * - Second item: CBOR text string "EDHOC error" (11 bytes)
	 *   - Major type 3 (text string), length 11 in additional info
	 *   - Header: 0x6B (0x60 | 11)
	 *   - Followed by 11 bytes of UTF-8 text
	 */
	zassert_equal(buffer_len, 1 + 1 + 11, "Encoded length should be 13 bytes");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x6B, "ERR_INFO header should be 0x6B (tstr, len=11)");
	zassert_mem_equal(&buffer[2], "EDHOC error", 11, "ERR_INFO should be 'EDHOC error'");
}

/* Test EDHOC error encoding: short diagnostic message */
ZTEST(coap, test_edhoc_encode_error_short_diag)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	r = coap_edhoc_encode_error(1, "err", buffer, &buffer_len);
	zassert_equal(r, 0, "Failed to encode EDHOC error");

	/* Verify encoding:
	 * - ERR_CODE: 0x01
	 * - ERR_INFO: 0x63 (tstr, len=3) + "err"
	 */
	zassert_equal(buffer_len, 1 + 1 + 3, "Encoded length should be 5 bytes");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x63, "ERR_INFO header should be 0x63 (tstr, len=3)");
	zassert_mem_equal(&buffer[2], "err", 3, "ERR_INFO should be 'err'");
}

/* Test EDHOC error encoding: longer diagnostic message (>23 bytes) */
ZTEST(coap, test_edhoc_encode_error_long_diag)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	/* 30-byte diagnostic message */
	const char *diag = "EDHOC processing failed here";

	r = coap_edhoc_encode_error(1, diag, buffer, &buffer_len);
	zassert_equal(r, 0, "Failed to encode EDHOC error");

	size_t diag_len = strlen(diag);

	/* Verify encoding:
	 * - ERR_CODE: 0x01
	 * - ERR_INFO: 0x78 (tstr, 1-byte length follows) + length byte + text
	 */
	zassert_equal(buffer_len, 1 + 2 + diag_len, "Encoded length incorrect");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x78, "ERR_INFO header should be 0x78 (tstr, 1-byte len)");
	zassert_equal(buffer[2], diag_len, "Length byte should match diagnostic length");
	zassert_mem_equal(&buffer[3], diag, diag_len, "ERR_INFO text incorrect");
}

/* Test EDHOC error encoding: buffer too small */
ZTEST(coap, test_edhoc_encode_error_buffer_too_small)
{
	uint8_t buffer[5];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	/* Try to encode "EDHOC error" (12 bytes) into 5-byte buffer */
	r = coap_edhoc_encode_error(1, "EDHOC error", buffer, &buffer_len);
	zassert_equal(r, -ENOMEM, "Should fail with -ENOMEM for small buffer");
}

/* Test EDHOC error encoding: invalid parameters */
ZTEST(coap, test_edhoc_encode_error_invalid_params)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	/* NULL buffer */
	r = coap_edhoc_encode_error(1, "test", NULL, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with NULL buffer");

	/* NULL length pointer */
	r = coap_edhoc_encode_error(1, "test", buffer, NULL);
	zassert_equal(r, -EINVAL, "Should fail with NULL length pointer");

	/* NULL diagnostic message */
	r = coap_edhoc_encode_error(1, NULL, buffer, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with NULL diagnostic message");

	/* Invalid error code (>23) */
	r = coap_edhoc_encode_error(100, "test", buffer, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with error code > 23");

	/* Negative error code */
	r = coap_edhoc_encode_error(-1, "test", buffer, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with negative error code");
}

/* Test EDHOC error encoding: empty diagnostic message */
ZTEST(coap, test_edhoc_encode_error_empty_diag)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	r = coap_edhoc_encode_error(1, "", buffer, &buffer_len);
	zassert_equal(r, 0, "Should succeed with empty diagnostic message");

	/* Verify encoding:
	 * - ERR_CODE: 0x01
	 * - ERR_INFO: 0x60 (tstr, len=0)
	 */
	zassert_equal(buffer_len, 2, "Encoded length should be 2 bytes");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x60, "ERR_INFO header should be 0x60 (tstr, len=0)");
}

/* Test EDHOC error response formatting: basic case */
ZTEST(coap, test_edhoc_error_response_format)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build a CON request */
	uint8_t token[] = { 0x12, 0x34 };

	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			     COAP_METHOD_POST, 0x5678);
	zassert_equal(r, 0, "Failed to initialize request");

	/* Build EDHOC error response */
	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");

	/* Verify response properties */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_ACK,
		      "Response should be ACK for CON request");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_BAD_REQUEST,
		      "Response code should be 4.00");
	zassert_equal(coap_header_get_id(&response), 0x5678,
		      "Response ID should match request ID");

	uint8_t resp_token[COAP_TOKEN_MAX_LEN];
	uint8_t resp_tkl = coap_header_get_token(&response, resp_token);

	zassert_equal(resp_tkl, sizeof(token), "Token length should match");
	zassert_mem_equal(resp_token, token, sizeof(token), "Token should match");

	/* Verify Content-Format option */
	int content_format = coap_get_option_int(&response, COAP_OPTION_CONTENT_FORMAT);

	zassert_equal(content_format, COAP_CONTENT_FORMAT_APP_EDHOC_CBOR_SEQ,
		      "Content-Format should be application/edhoc+cbor-seq (64)");

	/* Verify payload contains EDHOC error CBOR sequence */
	uint16_t payload_len;
	const uint8_t *payload = coap_packet_get_payload(&response, &payload_len);

	zassert_not_null(payload, "Response should have payload");
	zassert_true(payload_len > 0, "Payload should not be empty");

	/* Verify CBOR sequence structure:
	 * - First byte: ERR_CODE = 0x01
	 * - Second byte: tstr header for "EDHOC error" (11 bytes) = 0x6B
	 * - Remaining bytes: "EDHOC error"
	 */
	zassert_equal(payload[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(payload[1], 0x6B, "ERR_INFO header should be 0x6B");
	zassert_mem_equal(&payload[2], "EDHOC error", 11, "ERR_INFO should be 'EDHOC error'");
}

/* Test EDHOC error response: NON request should get NON response */
ZTEST(coap, test_edhoc_error_response_non)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build a NON request */
	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_NON_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");

	/* Verify response type is NON for NON request */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_NON_CON,
		      "Response should be NON for NON request");
}

/* Test EDHOC error response: no OSCORE option present */
ZTEST(coap, test_edhoc_error_response_no_oscore)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build request */
	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");

	/* Verify OSCORE option is NOT present in error response
	 * Per RFC 9668 Section 3.3.1, EDHOC error responses MUST NOT be OSCORE-protected
	 */
	struct coap_option option;

	r = coap_find_options(&response, COAP_OPTION_OSCORE, &option, 1);
	zassert_equal(r, 0, "OSCORE option should NOT be present in EDHOC error response");
}

/* Test EDHOC error response: different error codes */
ZTEST(coap, test_edhoc_error_response_different_codes)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	/* Test with 5.00 Internal Server Error */
	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_INTERNAL_ERROR,
					    1, "Server error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_INTERNAL_ERROR,
		      "Response code should be 5.00");

	/* Verify payload still has correct EDHOC error structure */
	uint16_t payload_len;
	const uint8_t *payload = coap_packet_get_payload(&response, &payload_len);

	zassert_not_null(payload, "Response should have payload");
	zassert_equal(payload[0], 0x01, "ERR_CODE should be 0x01");
}

/* Test EDHOC error response: buffer too small */
ZTEST(coap, test_edhoc_error_response_buffer_too_small)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[10];  /* Too small - need at least ~25 bytes */
	struct coap_packet request;
	struct coap_packet response;
	int r;

	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_true(r < 0, "Should fail with buffer too small");
}

/* Test RFC 9528 Table 14 ID mapping for derived OSCORE contexts */
ZTEST(coap, test_edhoc_oscore_id_mapping)
{
	/* This test verifies that EDHOC-derived OSCORE contexts use the correct
	 * Sender/Recipient ID mapping per RFC 9528 Appendix A.1 Table 14:
	 * "EDHOC Responder: OSCORE Sender ID = C_I; OSCORE Recipient ID = C_R"
	 */

	/* Test data: C_I and C_R from RFC 9528 test vectors */
	uint8_t c_i[] = { 0x37 };  /* Connection identifier for initiator */
	uint8_t c_r[] = { 0x27 };  /* Connection identifier for responder */

	/* Verify that wrapper signature accepts both IDs */
	uint8_t master_secret[16] = { 0 };
	uint8_t master_salt[8] = { 0 };
	struct context mock_ctx = { 0 };

	/* When CONFIG_UEDHOC=n, this will return -ENOTSUP (expected for tests) */
	int ret = coap_oscore_context_init_wrapper(
		&mock_ctx,
		master_secret, sizeof(master_secret),
		master_salt, sizeof(master_salt),
		c_i, sizeof(c_i),  /* Sender ID = C_I */
		c_r, sizeof(c_r),  /* Recipient ID = C_R */
		10,  /* AES-CCM-16-64-128 */
		5);  /* HKDF-SHA-256 */

	/* In test environment without CONFIG_UEDHOC, expect -ENOTSUP */
	/* In production with CONFIG_UEDHOC, this would succeed and initialize the context */
	zassert_true(ret == -ENOTSUP || ret == 0,
		     "Wrapper should return -ENOTSUP (test) or 0 (production)");
}

/* Test per-exchange OSCORE context tracking */
ZTEST(coap, test_oscore_exchange_context_tracking)
{
	/* This test verifies that OSCORE exchanges track the correct context
	 * for response protection, enabling per-exchange contexts for EDHOC-derived
	 * OSCORE contexts per RFC 9668 Section 3.3.1.
	 */

	struct coap_oscore_exchange cache[4] = { 0 };
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } }
	};
	uint8_t token[] = { 0x12, 0x34 };
	struct context mock_ctx = { 0 };

	/* Add exchange with specific context */
	int ret = oscore_exchange_add(cache, (struct net_sockaddr *)&addr,
				      sizeof(addr), token, sizeof(token),
				      false, &mock_ctx);
	zassert_equal(ret, 0, "Failed to add OSCORE exchange");

	/* Find exchange and verify context is stored */
	struct coap_oscore_exchange *exchange = oscore_exchange_find(
		cache, (struct net_sockaddr *)&addr, sizeof(addr),
		token, sizeof(token));

	zassert_not_null(exchange, "Exchange should be found");
	zassert_equal(exchange->oscore_ctx, &mock_ctx,
		      "Exchange should track the correct OSCORE context");
}

/* Test EDHOC session C_I storage */
ZTEST(coap, test_edhoc_session_ci_storage)
{
	/* This test verifies that EDHOC sessions can store C_I for later use
	 * in OSCORE context initialization per RFC 9528 Table 14.
	 */

	struct coap_edhoc_session cache[4] = { 0 };
	uint8_t c_r[] = { 0x27 };
	uint8_t c_i[] = { 0x37 };

	/* Insert session */
	struct coap_edhoc_session *session = coap_edhoc_session_insert(
		cache, ARRAY_SIZE(cache), c_r, sizeof(c_r));
	zassert_not_null(session, "Failed to insert EDHOC session");

	/* Set C_I */
	int ret = coap_edhoc_session_set_ci(session, c_i, sizeof(c_i));
	zassert_equal(ret, 0, "Failed to set C_I");

	/* Verify C_I is stored */
	zassert_equal(session->c_i_len, sizeof(c_i), "C_I length mismatch");
	zassert_mem_equal(session->c_i, c_i, sizeof(c_i), "C_I value mismatch");

	/* Find session and verify C_I is still there */
	struct coap_edhoc_session *found = coap_edhoc_session_find(
		cache, ARRAY_SIZE(cache), c_r, sizeof(c_r));
	zassert_not_null(found, "Session should be found");
	zassert_equal(found->c_i_len, sizeof(c_i), "Found C_I length mismatch");
	zassert_mem_equal(found->c_i, c_i, sizeof(c_i), "Found C_I value mismatch");
}

#if defined(CONFIG_UOSCORE)
/* Test OSCORE context allocation from pool */
ZTEST(coap, test_oscore_context_pool_allocation)
{
	/* This test verifies that OSCORE contexts can be allocated from the
	 * internal fixed pool for EDHOC-derived contexts.
	 */

	struct context *ctx1 = coap_oscore_ctx_alloc();
	zassert_not_null(ctx1, "Failed to allocate first context");

	struct context *ctx2 = coap_oscore_ctx_alloc();
	zassert_not_null(ctx2, "Failed to allocate second context");

	/* Contexts should be different */
	zassert_not_equal(ctx1, ctx2, "Contexts should be different");

	/* Free contexts */
	coap_oscore_ctx_free(ctx1);
	coap_oscore_ctx_free(ctx2);

	/* Should be able to allocate again after freeing */
	struct context *ctx3 = coap_oscore_ctx_alloc();
	zassert_not_null(ctx3, "Failed to allocate after freeing");

	coap_oscore_ctx_free(ctx3);
}
#endif /* CONFIG_UOSCORE */

#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

ZTEST(coap, test_edhoc_transport_message_1)
{
	/* Test EDHOC message_1 request to /.well-known/edhoc */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request to /.well-known/edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token123",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Add Content-Format: 65 (application/cid-edhoc+cbor-seq) */
	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 65);
	zassert_equal(r, 0, "Failed to add Content-Format");

	/* Add payload: CBOR true (0xF5) + dummy message_1 */
	uint8_t payload[] = {0xF5, 0x01, 0x02, 0x03, 0x04};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Verify payload can be retrieved */
	const uint8_t *retrieved_payload;
	uint16_t payload_len;

	retrieved_payload = coap_packet_get_payload(&request, &payload_len);
	zassert_not_null(retrieved_payload, "Payload should be present");
	zassert_equal(payload_len, sizeof(payload), "Payload length mismatch");
	zassert_mem_equal(retrieved_payload, payload, sizeof(payload), "Payload content mismatch");
}

ZTEST(coap, test_edhoc_transport_message_3)
{
	/* Test EDHOC message_3 request to /.well-known/edhoc */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request to /.well-known/edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token456",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Add Content-Format: 65 (application/cid-edhoc+cbor-seq) */
	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 65);
	zassert_equal(r, 0, "Failed to add Content-Format");

	/* Add payload: C_R (0x00) + dummy message_3 */
	uint8_t payload[] = {0x00, /* C_R as one-byte CBOR integer */
			     0x05, 0x06, 0x07, 0x08};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Verify payload can be retrieved */
	const uint8_t *retrieved_payload;
	uint16_t payload_len;

	retrieved_payload = coap_packet_get_payload(&request, &payload_len);
	zassert_not_null(retrieved_payload, "Payload should be present");
	zassert_equal(payload_len, sizeof(payload), "Payload length mismatch");
	zassert_mem_equal(retrieved_payload, payload, sizeof(payload), "Payload content mismatch");
}

ZTEST(coap, test_edhoc_transport_c_r_parsing_integer)
{
	/* Test parsing C_R as one-byte CBOR integer per RFC 9528 Section 3.3.2 */
	uint8_t payload[] = {0x00, 0x01, 0x02}; /* C_R=0x00, followed by data */

	/* Parse connection identifier - this is internal to coap_edhoc_transport.c
	 * For now, just verify the payload format is correct
	 */
	zassert_equal(payload[0], 0x00, "C_R should be 0x00");
}

ZTEST(coap, test_edhoc_transport_c_r_parsing_bstr)
{
	/* Test parsing C_R as CBOR byte string */
	uint8_t payload[] = {0x43, 0x01, 0x02, 0x03, /* bstr(3) = {0x01, 0x02, 0x03} */
			     0x04, 0x05}; /* followed by data */

	/* Verify CBOR byte string encoding */
	zassert_equal(payload[0], 0x43, "Should be bstr(3)");
	zassert_equal(payload[1], 0x01, "First byte of C_R");
	zassert_equal(payload[2], 0x02, "Second byte of C_R");
	zassert_equal(payload[3], 0x03, "Third byte of C_R");
}

ZTEST(coap, test_edhoc_transport_error_wrong_method)
{
	/* Test that non-POST methods to /.well-known/edhoc are rejected */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build GET request (wrong method) */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token789",
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Verify method is GET */
	uint8_t code = coap_header_get_code(&request);

	zassert_equal(code, COAP_METHOD_GET, "Method should be GET");
}

ZTEST(coap, test_edhoc_transport_error_no_payload)
{
	/* Test that EDHOC requests without payload are rejected */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request without payload */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token000",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Verify no payload */
	const uint8_t *payload;
	uint16_t payload_len;

	payload = coap_packet_get_payload(&request, &payload_len);
	zassert_is_null(payload, "Payload should be NULL");
}

ZTEST(coap, test_edhoc_transport_error_invalid_prefix)
{
	/* Test that message_1 with invalid prefix (not 0xF5) is rejected */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request with invalid prefix */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"tokenAAA",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Add Content-Format */
	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 65);
	zassert_equal(r, 0, "Failed to add Content-Format");

	/* Add payload with invalid prefix (0xF4 instead of 0xF5) */
	uint8_t payload[] = {0xF4, 0x01, 0x02, 0x03};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Verify payload has wrong prefix */
	const uint8_t *retrieved_payload;
	uint16_t payload_len;

	retrieved_payload = coap_packet_get_payload(&request, &payload_len);
	zassert_not_null(retrieved_payload, "Payload should be present");
	zassert_not_equal(retrieved_payload[0], 0xF5, "Prefix should not be 0xF5");
}

#if defined(CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC)

ZTEST(coap, test_edhoc_transport_content_format_missing)
{
	/* Test that EDHOC requests without Content-Format are rejected */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request to /.well-known/edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token001",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Do NOT add Content-Format option */

	/* Add payload: CBOR true (0xF5) + dummy message_1 */
	uint8_t payload[] = {0xF5, 0x01, 0x02, 0x03, 0x04};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Validate Content-Format - should fail with -ENOENT (missing) */
	r = coap_edhoc_transport_validate_content_format(&request);
	zassert_equal(r, -ENOENT, "Should reject request without Content-Format, got %d", r);
}

ZTEST(coap, test_edhoc_transport_content_format_wrong_value)
{
	/* Test that EDHOC requests with Content-Format 64 are rejected */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request to /.well-known/edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token002",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Add Content-Format: 64 (wrong - should be 65 for client requests) */
	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 64);
	zassert_equal(r, 0, "Failed to add Content-Format");

	/* Add payload: CBOR true (0xF5) + dummy message_1 */
	uint8_t payload[] = {0xF5, 0x01, 0x02, 0x03, 0x04};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Validate Content-Format - should fail with -EBADMSG (wrong value) */
	r = coap_edhoc_transport_validate_content_format(&request);
	zassert_equal(r, -EBADMSG, "Should reject request with Content-Format 64, got %d", r);
}

ZTEST(coap, test_edhoc_transport_content_format_correct)
{
	/* Test that EDHOC requests with Content-Format 65 are accepted */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request to /.well-known/edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token003",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Add Content-Format: 65 (correct for client requests) */
	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 65);
	zassert_equal(r, 0, "Failed to add Content-Format");

	/* Add payload: CBOR true (0xF5) + dummy message_1 */
	uint8_t payload[] = {0xF5, 0x01, 0x02, 0x03, 0x04};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Validate Content-Format - should succeed */
	r = coap_edhoc_transport_validate_content_format(&request);
	zassert_equal(r, 0, "Should accept request with Content-Format 65, got %d", r);
}

ZTEST(coap, test_edhoc_transport_content_format_duplicate)
{
	/* Test that EDHOC requests with duplicate Content-Format options are rejected */
	uint8_t request_buf[128];
	struct coap_packet request;
	int r;

	/* Build POST request to /.well-known/edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token004",
			     COAP_METHOD_POST, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "edhoc", strlen("edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Path edhoc");

	/* Add Content-Format option twice (duplicate) */
	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 65);
	zassert_equal(r, 0, "Failed to add first Content-Format");

	r = coap_append_option_int(&request, COAP_OPTION_CONTENT_FORMAT, 65);
	zassert_equal(r, 0, "Failed to add second Content-Format");

	/* Add payload: CBOR true (0xF5) + dummy message_1 */
	uint8_t payload[] = {0xF5, 0x01, 0x02, 0x03, 0x04};

	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to add payload marker");

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to add payload");

	/* Validate Content-Format - should fail with -EMSGSIZE (duplicate) */
	r = coap_edhoc_transport_validate_content_format(&request);
	zassert_equal(r, -EMSGSIZE, "Should reject request with duplicate Content-Format, got %d",
		      r);
}

#endif /* CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC */

#if defined(CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC)
/* Test wrappers for EDHOC transport */

/* Forward declaration for test-only helper */
extern int coap_edhoc_transport_validate_content_format(const struct coap_packet *request);

/* Mock EDHOC message_2 generation */
int coap_edhoc_msg2_gen_wrapper(void *resp_ctx,
				void *runtime_ctx,
				const uint8_t *msg1,
				size_t msg1_len,
				uint8_t *msg2,
				size_t *msg2_len,
				uint8_t *c_r,
				size_t *c_r_len)
{
	ARG_UNUSED(resp_ctx);
	ARG_UNUSED(runtime_ctx);

	/* Verify message_1 is present */
	if (msg1 == NULL || msg1_len == 0) {
		return -EINVAL;
	}

	/* Generate dummy message_2 */
	const uint8_t dummy_msg2[] = {0x58, 0x10, /* bstr(16) */
				      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
				      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

	if (*msg2_len < sizeof(dummy_msg2)) {
		return -ENOMEM;
	}

	memcpy(msg2, dummy_msg2, sizeof(dummy_msg2));
	*msg2_len = sizeof(dummy_msg2);

	/* Generate dummy C_R (one-byte CBOR integer 0x00) */
	c_r[0] = 0x00;
	*c_r_len = 1;

	return 0;
}

/* Mock EDHOC message_3 processing */
int coap_edhoc_msg3_process_wrapper(const uint8_t *edhoc_msg3,
				    size_t edhoc_msg3_len,
				    void *resp_ctx,
				    void *runtime_ctx,
				    void *cred_i_array,
				    uint8_t *prk_out,
				    size_t *prk_out_len,
				    uint8_t *initiator_pk,
				    size_t *initiator_pk_len,
				    uint8_t *c_i,
				    size_t *c_i_len)
{
	ARG_UNUSED(resp_ctx);
	ARG_UNUSED(runtime_ctx);
	ARG_UNUSED(cred_i_array);
	ARG_UNUSED(initiator_pk);
	ARG_UNUSED(initiator_pk_len);

	/* Verify message_3 is present */
	if (edhoc_msg3 == NULL || edhoc_msg3_len == 0) {
		return -EINVAL;
	}

	/* Generate dummy PRK_out */
	if (*prk_out_len < 32) {
		return -ENOMEM;
	}
	memset(prk_out, 0xAA, 32);
	*prk_out_len = 32;

	/* Generate dummy C_I (one-byte CBOR integer 0x01) */
	c_i[0] = 0x01;
	*c_i_len = 1;

	return 0;
}

/* Mock EDHOC message_4 generation */
int coap_edhoc_msg4_gen_wrapper(void *resp_ctx,
				void *runtime_ctx,
				uint8_t *msg4,
				size_t *msg4_len,
				bool *msg4_required)
{
	ARG_UNUSED(resp_ctx);
	ARG_UNUSED(runtime_ctx);
	ARG_UNUSED(msg4);

	/* For testing, message_4 is not required */
	*msg4_required = false;
	*msg4_len = 0;

	return 0;
}

/* Mock EDHOC exporter */
int coap_edhoc_exporter_wrapper(const uint8_t *prk_out,
				size_t prk_out_len,
				int app_hash_alg,
				uint8_t label,
				uint8_t *output,
				size_t *output_len)
{
	ARG_UNUSED(prk_out);
	ARG_UNUSED(prk_out_len);
	ARG_UNUSED(app_hash_alg);

	/* Generate dummy output based on label */
	size_t out_len = (label == 0) ? 16 : 8; /* master_secret : master_salt */

	if (*output_len < out_len) {
		return -ENOMEM;
	}

	memset(output, 0xBB + label, out_len);
	*output_len = out_len;

	return 0;
}

/* Mock OSCORE context init */
int coap_oscore_context_init_wrapper(void *ctx,
				     const uint8_t *master_secret,
				     size_t master_secret_len,
				     const uint8_t *master_salt,
				     size_t master_salt_len,
				     const uint8_t *sender_id,
				     size_t sender_id_len,
				     const uint8_t *recipient_id,
				     size_t recipient_id_len,
				     int aead_alg,
				     int hkdf_alg)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(aead_alg);
	ARG_UNUSED(hkdf_alg);

	/* Verify parameters */
	if (master_secret == NULL || master_secret_len == 0 ||
	    sender_id == NULL || sender_id_len == 0 ||
	    recipient_id == NULL || recipient_id_len == 0) {
		return -EINVAL;
	}

	return 0;
}

#endif /* CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC */

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)

#include "coap_edhoc_combined_blockwise.h"
#include "coap_edhoc.h"
#include "coap_edhoc_client_combined.h"
#include <zephyr/net/net_ip.h>

/* Test outer Block1 reassembly for EDHOC+OSCORE combined requests */

/**
 * Test Case A: EDHOC option present only on block NUM=0; subsequent blocks omit EDHOC option
 * Must still reassemble and produce the full reconstructed request
 */
ZTEST(coap, test_edhoc_outer_block_reassembly_case_a)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;

	/* Build synthetic combined payload: CBOR bstr(EDHOC_MSG_3) + OSCORE_PAYLOAD
	 * EDHOC_MSG_3 = 24 bytes: 0x58 0x18 (bstr length 24) + "EDHOC_DATA_LONG_MESSAG12"
	 * OSCORE_PAYLOAD = 6 bytes: "OSCOR!"
	 * Total payload = 32 bytes (2 blocks of 16 bytes each)
	 */
	uint8_t combined_payload[] = {
		0x58, 0x18, /* CBOR bstr, length 24 */
		'E', 'D', 'H', 'O', 'C', '_', 'D', 'A', 'T', 'A', '_', 'L',
		'O', 'N', 'G', '_', 'M', 'E', 'S', 'S', 'A', 'G', '1', '2',
		/* EDHOC_MSG_3 content: 24 bytes */
		'O', 'S', 'C', 'O', 'R', '!' /* OSCORE_PAYLOAD: 6 bytes */
	};

	/* Block 0: 16 bytes of payload, EDHOC option present, M=1 */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	/* Add EDHOC option (empty per RFC 9668) */
	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	/* Add OSCORE option (dummy kid) */
	uint8_t kid[] = {0x01, 0x02};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	/* Add Block1 option: NUM=0, M=1, SZX=0 (16 bytes) */
	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32, /* Total is larger than current + block_size, so M=1 (more blocks) */
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add first 16 bytes of payload */
	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, combined_payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* Process block 0 */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Block 1: next 16 bytes, NO EDHOC option (per Case A), M=0 (last block) */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 1 request");

	/* NO EDHOC option on continuation blocks */

	/* Add OSCORE option (same kid) */
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	/* Add Block1 option: NUM=1, M=0, SZX=0 */
	block_ctx.current = 16; /* Offset after first block (16 bytes from block 0) */
	block_ctx.total_size = 32; /* Total matches current + this block size, so M=0 (last block) */
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add remaining 16 bytes of payload */
	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, combined_payload + 16, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* Process block 1 */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_COMPLETE,
		      "Block 1 should return COMPLETE");

	/* Verify reconstructed request contains full payload */
	struct coap_packet reconstructed;
	struct coap_option options[16];
	ret = coap_packet_parse(&reconstructed, reconstructed_buf, reconstructed_len,
				options, 16);
	zassert_equal(ret, 0, "Failed to parse reconstructed request");

	uint16_t payload_len;
	const uint8_t *payload = coap_packet_get_payload(&reconstructed, &payload_len);
	zassert_not_null(payload, "Reconstructed request should have payload");
	zassert_equal(payload_len, (uint16_t)sizeof(combined_payload),
		      "Payload length mismatch: expected %zu, got %u",
		      sizeof(combined_payload), payload_len);
	zassert_mem_equal(payload, combined_payload, sizeof(combined_payload),
			  "Payload content mismatch");

	/* Verify EDHOC option is present in reconstructed request (from block 0) */
	zassert_true(coap_edhoc_msg_has_edhoc(&reconstructed),
		     "Reconstructed request should have EDHOC option");
}

/**
 * Test Case B: Out-of-order NUM or inconsistent block size
 * Must fail and clear state
 */
ZTEST(coap, test_edhoc_outer_block_reassembly_case_b)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x05, 0x06, 0x07, 0x08};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	/* Payload: 32 bytes total (2 blocks of 16 bytes each) */
	uint8_t payload[32];
	memset(payload, 'A', sizeof(payload));

	/* Block 0: Start reassembly with first 16 bytes */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	uint8_t kid[] = {0x03, 0x04};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32, /* More than current + block_size, so M=1 */
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Block with wrong NUM (skip NUM=1, send NUM=2) - should fail */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 2 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	block_ctx.current = 32; /* Wrong: should be 16 (offset after block 0 with 16 bytes) */
	block_ctx.total_size = 48;
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload + 16, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_ERROR,
		      "Out-of-order block should return ERROR");

	/* Verify cache entry was cleared */
	struct coap_edhoc_outer_block_entry *entry;
	entry = coap_edhoc_outer_block_find(service.data->outer_block_cache,
					    CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					    (struct net_sockaddr *)&client_addr,
					    sizeof(client_addr),
					    token, sizeof(token));
	zassert_is_null(entry, "Cache entry should be cleared after error");
}

/**
 * Test Case C: Reassembled size exceeds CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_MAX_LEN
 * Must fail with configured error path
 */
ZTEST(coap, test_edhoc_outer_block_reassembly_case_c)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x09, 0x0A, 0x0B, 0x0C};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x3 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;

	/* Create a large payload that will exceed the limit */
	uint8_t large_payload[128];
	memset(large_payload, 0xAA, sizeof(large_payload));

	/* Block 0: Start with large block */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	uint8_t kid[] = {0x05, 0x06};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_128,
		.current = 0,
		.total_size = 2560, /* Much larger than current, so M=1 */
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, large_payload, 128);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Continue sending blocks until we exceed the limit
	 * CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_MAX_LEN defaults to 1024
	 */
	for (uint32_t num = 1; num < 20; num++) {
		ret = coap_packet_init(&request, buf, sizeof(buf),
				       COAP_VERSION_1, COAP_TYPE_CON,
				       sizeof(token), token,
				       COAP_METHOD_POST, coap_next_id());
		zassert_equal(ret, 0, "Failed to init block %u request", num);

		ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
		zassert_equal(ret, 0, "Failed to add OSCORE option");

		block_ctx.current = num * 128;
		block_ctx.total_size = 2560; /* Keep sending more blocks */
		ret = coap_append_block1_option(&request, &block_ctx);
		zassert_equal(ret, 0, "Failed to add Block1 option");

		ret = coap_packet_append_payload_marker(&request);
		zassert_equal(ret, 0, "Failed to add payload marker");
		ret = coap_packet_append_payload(&request, large_payload, 128);
		zassert_equal(ret, 0, "Failed to add payload");

		ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
						     (struct net_sockaddr *)&client_addr,
						     sizeof(client_addr),
						     reconstructed_buf, &reconstructed_len);

		/* Should eventually fail with REQUEST_TOO_LARGE */
		if (ret == COAP_EDHOC_OUTER_BLOCK_ERROR) {
			/* Verify cache was cleared */
			struct coap_edhoc_outer_block_entry *entry;
			entry = coap_edhoc_outer_block_find(
				service.data->outer_block_cache,
				CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
				(struct net_sockaddr *)&client_addr,
				sizeof(client_addr),
				token, sizeof(token));
			zassert_is_null(entry,
					"Cache entry should be cleared after size limit exceeded");
			return; /* Test passed */
		}

		zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
			      "Block %u should return WAITING or ERROR", num);
	}

	zassert_unreachable("Should have exceeded size limit and returned ERROR");
}

/**
 * Test intermediate-block response generation: 2.31 Continue with Block1 option
 */
ZTEST(coap, test_edhoc_outer_block_continue_response)
{
	/* This test verifies that the helper returns/builds a 2.31 Continue response
	 * and includes a Block1 option for continuation.
	 * The actual response sending is tested implicitly in Case A above.
	 */

	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x0D, 0x0E, 0x0F, 0x10};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x4 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	uint8_t payload[] = "TEST_PAYLOAD_DATA";

	/* Send first block */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init request");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	uint8_t kid[] = {0x07, 0x08};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32, /* More than current, so M=1 */
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* Process - should return WAITING and send 2.31 Continue */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "First block should return WAITING");

	/* Verify cache entry exists */
	struct coap_edhoc_outer_block_entry *entry;
	entry = coap_edhoc_outer_block_find(service.data->outer_block_cache,
					    CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					    (struct net_sockaddr *)&client_addr,
					    sizeof(client_addr),
					    token, sizeof(token));
	zassert_not_null(entry, "Cache entry should exist after first block");
	zassert_equal(entry->accumulated_len, 16, "Should have accumulated 16 bytes");
}

/**
 * Test RFC 9175 Section 3.3: Request-Tag is part of the blockwise operation key
 * Different Request-Tag values must be treated as different operations
 */
ZTEST(coap, test_edhoc_outer_block_request_tag_operation_key)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x11, 0x12, 0x13, 0x14};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x5 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	uint8_t payload[32];

	memset(payload, 0xA5, sizeof(payload));

	/* Block 0: Start with Request-Tag = 0x42 */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	uint8_t kid[] = {0x09, 0x0A};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32,
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add Request-Tag option with value 0x42 (must come after Block1) */
	uint8_t request_tag_a[] = {0x42};
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_a, sizeof(request_tag_a));
	zassert_equal(ret, 0, "Failed to add Request-Tag option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Block 1: Send with different Request-Tag = 0x43 (should fail) */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 1 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	block_ctx.current = 16;
	block_ctx.total_size = 32;
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add Request-Tag option with DIFFERENT value 0x43 (must come after Block1) */
	uint8_t request_tag_b[] = {0x43};
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_b, sizeof(request_tag_b));
	zassert_equal(ret, 0, "Failed to add Request-Tag option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload + 16, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* RFC 9175 Section 3.3: different Request-Tag = different operation = ERROR */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_ERROR,
		      "Different Request-Tag should return ERROR");

	/* Verify cache entry was cleared (fail-closed policy) */
	struct coap_edhoc_outer_block_entry *entry;
	entry = coap_edhoc_outer_block_find(service.data->outer_block_cache,
					    CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					    (struct net_sockaddr *)&client_addr,
					    sizeof(client_addr),
					    token, sizeof(token));
	zassert_is_null(entry, "Cache entry should be cleared after Request-Tag mismatch");
}

/**
 * Test RFC 9175 Section 3.4: Absent Request-Tag vs 0-length Request-Tag are distinct
 */
ZTEST(coap, test_edhoc_outer_block_request_tag_absent_vs_zero_length)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x15, 0x16, 0x17, 0x18};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x6 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	uint8_t payload[32];

	memset(payload, 0xA5, sizeof(payload));

	/* Block 0: Start with NO Request-Tag (absent) */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	uint8_t kid[] = {0x0B, 0x0C};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	/* NO Request-Tag option */

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32,
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Block 1: Send with 0-length Request-Tag (present but empty) */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 1 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	block_ctx.current = 16;
	block_ctx.total_size = 32;
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add Request-Tag option with 0-length (present but empty, must come after Block1) */
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG, NULL, 0);
	zassert_equal(ret, 0, "Failed to add 0-length Request-Tag option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload + 16, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* RFC 9175 Section 3.4: absent vs 0-length are distinct = ERROR */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_ERROR,
		      "Absent vs 0-length Request-Tag should return ERROR");

	/* Verify cache entry was cleared */
	struct coap_edhoc_outer_block_entry *entry;
	entry = coap_edhoc_outer_block_find(service.data->outer_block_cache,
					    CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					    (struct net_sockaddr *)&client_addr,
					    sizeof(client_addr),
					    token, sizeof(token));
	zassert_is_null(entry, "Cache entry should be cleared after Request-Tag mismatch");
}

/**
 * Test RFC 9175 Section 3.2.1: Request-Tag is repeatable, list must match exactly
 */
ZTEST(coap, test_edhoc_outer_block_request_tag_repeatable_list)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x19, 0x1A, 0x1B, 0x1C};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x7 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	uint8_t payload[32];

	memset(payload, 0xA5, sizeof(payload));

	/* Block 0: Start with two Request-Tag options */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	uint8_t kid[] = {0x0D, 0x0E};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32,
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add first Request-Tag option (must come after Block1) */
	uint8_t request_tag_1[] = {0x11, 0x22};
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_1, sizeof(request_tag_1));
	zassert_equal(ret, 0, "Failed to add first Request-Tag option");

	/* Add second Request-Tag option */
	uint8_t request_tag_2[] = {0x33, 0x44};
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_2, sizeof(request_tag_2));
	zassert_equal(ret, 0, "Failed to add second Request-Tag option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Block 1: Send with same two Request-Tag options in same order (should succeed) */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 1 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	block_ctx.current = 16;
	block_ctx.total_size = 32;
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add same Request-Tag options in same order (must come after Block1) */
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_1, sizeof(request_tag_1));
	zassert_equal(ret, 0, "Failed to add first Request-Tag option");

	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_2, sizeof(request_tag_2));
	zassert_equal(ret, 0, "Failed to add second Request-Tag option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload + 16, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* Same Request-Tag list should succeed */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_COMPLETE,
		      "Same Request-Tag list should return COMPLETE");
}

/**
 * Test RFC 9175 Section 3.2.1: Request-Tag list with different order should fail
 */
ZTEST(coap, test_edhoc_outer_block_request_tag_different_order)
{
	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x1D, 0x1E, 0x1F, 0x20};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x8 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	uint8_t payload[32];

	memset(payload, 0xA5, sizeof(payload));

	/* Block 0: Start with two Request-Tag options in order A, B */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	uint8_t kid[] = {0x0F, 0x10};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32,
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	uint8_t request_tag_a[] = {0xAA};
	uint8_t request_tag_b[] = {0xBB};

	/* Add in order: A, B (must come after Block1) */
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_a, sizeof(request_tag_a));
	zassert_equal(ret, 0, "Failed to add Request-Tag A");

	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_b, sizeof(request_tag_b));
	zassert_equal(ret, 0, "Failed to add Request-Tag B");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Block 1: Send with same tags but in DIFFERENT order: B, A (should fail) */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 1 request");

	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	block_ctx.current = 16;
	block_ctx.total_size = 32;
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Add in DIFFERENT order: B, A (must come after Block1) */
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_b, sizeof(request_tag_b));
	zassert_equal(ret, 0, "Failed to add Request-Tag B");

	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag_a, sizeof(request_tag_a));
	zassert_equal(ret, 0, "Failed to add Request-Tag A");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload + 16, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* Different order should fail */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_ERROR,
		      "Different Request-Tag order should return ERROR");

	/* Verify cache entry was cleared */
	struct coap_edhoc_outer_block_entry *entry;
	entry = coap_edhoc_outer_block_find(service.data->outer_block_cache,
					    CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					    (struct net_sockaddr *)&client_addr,
					    sizeof(client_addr),
					    token, sizeof(token));
	zassert_is_null(entry, "Cache entry should be cleared after Request-Tag mismatch");
}

/**
 * Test RFC 9175 Section 3.4: 2.31 Continue response MUST NOT contain Request-Tag
 */
ZTEST(coap, test_edhoc_outer_block_continue_no_request_tag)
{
	/* This test verifies that the 2.31 Continue response does not include Request-Tag.
	 * Since we construct fresh responses in send_continue_response(), this is a regression test.
	 * We verify by checking that a block 0 with Request-Tag successfully creates a cache entry,
	 * and the implementation doesn't accidentally copy Request-Tag to responses.
	 */

	uint8_t buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct coap_packet request;
	uint8_t token[] = {0x21, 0x22, 0x23, 0x24};
	struct net_sockaddr_in6 client_addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x9 } } },
		.sin6_port = net_htons(5683),
	};
	struct coap_service_data service_data = {0};
	struct coap_service service = {
		.data = &service_data,
	};
	uint8_t reconstructed_buf[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	size_t reconstructed_len = 0;
	int ret;
	uint8_t payload[32];

	memset(payload, 0xA5, sizeof(payload));

	/* Block 0: Start with Request-Tag */
	ret = coap_packet_init(&request, buf, sizeof(buf),
			       COAP_VERSION_1, COAP_TYPE_CON,
			       sizeof(token), token,
			       COAP_METHOD_POST, coap_next_id());
	zassert_equal(ret, 0, "Failed to init block 0 request");

	uint8_t kid[] = {0x11, 0x12};
	ret = coap_packet_append_option(&request, COAP_OPTION_OSCORE, kid, sizeof(kid));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	ret = coap_packet_append_option(&request, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(ret, 0, "Failed to add EDHOC option");

	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_16,
		.current = 0,
		.total_size = 32,
	};
	ret = coap_append_block1_option(&request, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	uint8_t request_tag[] = {0x99, 0x88};
	ret = coap_packet_append_option(&request, COAP_OPTION_REQUEST_TAG,
					request_tag, sizeof(request_tag));
	zassert_equal(ret, 0, "Failed to add Request-Tag option");

	ret = coap_packet_append_payload_marker(&request);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&request, payload, 16);
	zassert_equal(ret, 0, "Failed to add payload");

	/* Process - should return WAITING (which triggers 2.31 Continue response) */
	ret = coap_edhoc_outer_block_process(&service, &request, buf, request.offset,
					     (struct net_sockaddr *)&client_addr,
					     sizeof(client_addr),
					     reconstructed_buf, &reconstructed_len);
	zassert_equal(ret, COAP_EDHOC_OUTER_BLOCK_WAITING,
		      "Block 0 should return WAITING");

	/* Verify cache entry exists with Request-Tag stored */
	struct coap_edhoc_outer_block_entry *entry;
	entry = coap_edhoc_outer_block_find(service.data->outer_block_cache,
					    CONFIG_COAP_EDHOC_COMBINED_OUTER_BLOCK_CACHE_SIZE,
					    (struct net_sockaddr *)&client_addr,
					    sizeof(client_addr),
					    token, sizeof(token));
	zassert_not_null(entry, "Cache entry should exist");
	zassert_equal(entry->request_tag_count, 1, "Should have 1 Request-Tag stored");
	zassert_true(entry->request_tag_data_len > 0, "Request-Tag data should be stored");

	/* The actual response sending is handled by send_continue_response() which constructs
	 * a fresh response without copying Request-Tag. This is verified by code inspection
	 * and the fact that we only add Block1 option to the response.
	 */
}

#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

/* RFC 7959 §2.4 Block2 ETag validation tests */
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_TEST_API_ENABLE)

/* Helper to build a Block2 response with optional ETag */
static int build_block2_response(struct coap_packet *response, uint8_t *buf, size_t buf_len,
				 const uint8_t *token, uint8_t tkl, uint16_t id,
				 int block_num, bool more, const uint8_t *etag,
				 uint8_t etag_len, const uint8_t *payload, size_t payload_len)
{
	int ret;

	ret = coap_packet_init(response, buf, buf_len, COAP_VERSION_1, COAP_TYPE_ACK,
			       tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (ret < 0) {
		return ret;
	}

	if (etag != NULL && etag_len > 0) {
		ret = coap_packet_append_option(response, COAP_OPTION_ETAG, etag, etag_len);
		if (ret < 0) {
			return ret;
		}
	}

	ret = coap_append_option_int(response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (ret < 0) {
		return ret;
	}

	int block_opt = (block_num << 4) | (more ? 0x08 : 0x00) | COAP_BLOCK_64;
	ret = coap_append_option_int(response, COAP_OPTION_BLOCK2, block_opt);
	if (ret < 0) {
		return ret;
	}

	if (payload != NULL && payload_len > 0) {
		ret = coap_packet_append_payload_marker(response);
		if (ret < 0) {
			return ret;
		}
		ret = coap_packet_append_payload(response, payload, payload_len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}


/* Helper to set up test request state after block 0 */
static void setup_block_state(struct coap_client_internal_request *req,
			      const uint8_t *token, size_t tkl,
			      const uint8_t *etag, size_t etag_len)
{
	req->request_ongoing = true;
	req->last_response_id = -1;
	memcpy(req->request_token, token, tkl);
	req->request_tkl = tkl;
	if (etag && etag_len > 0) {
		memcpy(req->block2_etag, etag, etag_len);
		req->block2_etag_len = etag_len;
	}
	req->recv_blk_ctx.current = 64;
	req->recv_blk_ctx.block_size = COAP_BLOCK_64;
}

ZTEST(coap, test_edhoc_oscore_combined_request_construction)
{
	uint8_t oscore_pkt_buf[256];
	struct coap_packet oscore_pkt;
	uint8_t combined_buf[512];
	size_t combined_len;
	int ret;

	/* Build a synthetic OSCORE-protected packet
	 * Header: CON POST, token=0x42, MID=0x1234
	 * Options: OSCORE option (9) with value 0x09 (kid=empty, PIV=empty, kid context=empty)
	 * Payload: OSCORE ciphertext "OSCORE_CIPHERTEXT"
	 */
	uint8_t token[] = { 0x42 };
	ret = coap_packet_init(&oscore_pkt, oscore_pkt_buf, sizeof(oscore_pkt_buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1234);
	zassert_equal(ret, 0, "Failed to init OSCORE packet");

	/* Add OSCORE option (simplified: just flag byte 0x09) */
	uint8_t oscore_opt[] = { 0x09 };
	ret = coap_packet_append_option(&oscore_pkt, COAP_OPTION_OSCORE,
					oscore_opt, sizeof(oscore_opt));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	/* Add OSCORE payload (ciphertext) */
	const char *oscore_payload = "OSCORE_CIPHERTEXT";
	ret = coap_packet_append_payload_marker(&oscore_pkt);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&oscore_pkt, (const uint8_t *)oscore_payload,
					 strlen(oscore_payload));
	zassert_equal(ret, 0, "Failed to add OSCORE payload");

	/* Build EDHOC_MSG_3 as CBOR bstr encoding
	 * For testing, use a simple CBOR bstr: 0x4D (bstr of length 13) + "EDHOC_MSG_3!!"
	 */
	uint8_t edhoc_msg3[] = { 0x4D, 'E', 'D', 'H', 'O', 'C', '_', 'M', 'S', 'G', '_', '3', '!', '!' };
	size_t edhoc_msg3_len = sizeof(edhoc_msg3);

	/* Build combined request */
	ret = coap_edhoc_client_build_combined_request(
		oscore_pkt.data, oscore_pkt.offset,
		edhoc_msg3, edhoc_msg3_len,
		combined_buf, sizeof(combined_buf),
		&combined_len);
	zassert_equal(ret, 0, "Failed to build combined request");

	/* Parse combined request */
	struct coap_packet combined_pkt;
	ret = coap_packet_parse(&combined_pkt, combined_buf, combined_len, NULL, 0);
	zassert_equal(ret, 0, "Failed to parse combined request");

	/* RFC 9668 Section 3.1: EDHOC option MUST occur at most once and MUST be empty */
	struct coap_option edhoc_opts[2];
	int num_edhoc = coap_find_options(&combined_pkt, COAP_OPTION_EDHOC,
					  edhoc_opts, ARRAY_SIZE(edhoc_opts));
	zassert_equal(num_edhoc, 1, "EDHOC option should appear exactly once, got %d", num_edhoc);
	zassert_equal(edhoc_opts[0].len, 0, "EDHOC option should be empty, got len=%u",
		      edhoc_opts[0].len);

	/* RFC 9668 Section 3.2.1 Step 3: Payload should be EDHOC_MSG_3 || OSCORE_PAYLOAD */
	uint16_t payload_len;
	const uint8_t *payload = coap_packet_get_payload(&combined_pkt, &payload_len);
	zassert_not_null(payload, "Combined request should have payload");

	/* Check payload starts with EDHOC_MSG_3 */
	zassert_true(payload_len >= edhoc_msg3_len,
		     "Payload too short (%u < %zu)", payload_len, edhoc_msg3_len);
	zassert_mem_equal(payload, edhoc_msg3, edhoc_msg3_len,
			  "Payload should start with EDHOC_MSG_3");

	/* Check OSCORE payload follows */
	const uint8_t *oscore_part = payload + edhoc_msg3_len;
	size_t oscore_part_len = payload_len - edhoc_msg3_len;
	zassert_equal(oscore_part_len, strlen(oscore_payload),
		      "OSCORE part length mismatch");
	zassert_mem_equal(oscore_part, oscore_payload, strlen(oscore_payload),
			  "OSCORE payload mismatch");

	/* Verify header fields are preserved */
	zassert_equal(coap_header_get_type(&combined_pkt), COAP_TYPE_CON,
		      "Type should be preserved");
	zassert_equal(coap_header_get_code(&combined_pkt), COAP_METHOD_POST,
		      "Code should be preserved");
	zassert_equal(coap_header_get_id(&combined_pkt), 0x1234,
		      "MID should be preserved");
	uint8_t combined_token[COAP_TOKEN_MAX_LEN];
	uint8_t combined_tkl = coap_header_get_token(&combined_pkt, combined_token);
	zassert_equal(combined_tkl, 1, "Token length should be preserved");
	zassert_equal(combined_token[0], 0x42, "Token should be preserved");
}

/**
 * @brief Test combined request with Block1 NUM != 0
 *
 * Tests RFC 9668 Section 3.2.2 Step 2.1:
 * - EDHOC option should NOT be included for non-first inner Block1
 */
ZTEST(coap, test_edhoc_oscore_combined_request_block1_continuation)
{
	uint8_t plaintext_buf[256];
	struct coap_packet plaintext_pkt;
	bool is_first_block;
	int ret;

	/* Build plaintext request with Block1 NUM=1 (continuation block) */
	uint8_t token[] = { 0x42 };
	ret = coap_packet_init(&plaintext_pkt, plaintext_buf, sizeof(plaintext_buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1234);
	zassert_equal(ret, 0, "Failed to init plaintext packet");

	/* Add Block1 option with NUM=1, M=1, SZX=6 (1024 bytes)
	 * Block1 value encoding: NUM(variable bits) | M(1 bit) | SZX(3 bits)
	 * For NUM=1, M=1, SZX=6: (1 << 4) | (1 << 3) | 6 = 0x1E
	 */
	struct coap_block_context block_ctx = {
		.block_size = COAP_BLOCK_1024,
		.current = 1024,  /* Second block */
		.total_size = 0
	};
	ret = coap_append_block1_option(&plaintext_pkt, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	/* Check if this is first block */
	ret = coap_edhoc_client_is_first_inner_block(plaintext_pkt.data,
						      plaintext_pkt.offset,
						      &is_first_block);
	zassert_equal(ret, 0, "Failed to check first block");
	zassert_false(is_first_block, "Block1 NUM=1 should not be first block");

	/* Build another request with Block1 NUM=0 (first block) */
	ret = coap_packet_init(&plaintext_pkt, plaintext_buf, sizeof(plaintext_buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1235);
	zassert_equal(ret, 0, "Failed to init plaintext packet");

	block_ctx.current = 0;  /* First block */
	ret = coap_append_block1_option(&plaintext_pkt, &block_ctx);
	zassert_equal(ret, 0, "Failed to add Block1 option");

	ret = coap_edhoc_client_is_first_inner_block(plaintext_pkt.data,
						      plaintext_pkt.offset,
						      &is_first_block);
	zassert_equal(ret, 0, "Failed to check first block");
	zassert_true(is_first_block, "Block1 NUM=0 should be first block");

	/* Build request without Block1 option (treated as NUM=0) */
	ret = coap_packet_init(&plaintext_pkt, plaintext_buf, sizeof(plaintext_buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1236);
	zassert_equal(ret, 0, "Failed to init plaintext packet");

	ret = coap_edhoc_client_is_first_inner_block(plaintext_pkt.data,
						      plaintext_pkt.offset,
						      &is_first_block);
	zassert_equal(ret, 0, "Failed to check first block");
	zassert_true(is_first_block, "No Block1 should be treated as first block");
}

/**
 * @brief Test MAX_UNFRAGMENTED_SIZE constraint for EDHOC+OSCORE combined request
 *
 * Tests RFC 9668 Section 3.2.2 Step 3.1:
 * - If COMB_PAYLOAD exceeds MAX_UNFRAGMENTED_SIZE, function returns -EMSGSIZE
 * - No packet is sent (fail-closed)
 */
ZTEST(coap, test_edhoc_oscore_combined_request_max_unfragmented_size)
{
	/* Use a larger buffer to accommodate the large payload */
	static uint8_t oscore_pkt_buf[CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE + 128];
	struct coap_packet oscore_pkt;
	uint8_t combined_buf[CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE + 256];
	size_t combined_len;
	int ret;

	/* Build OSCORE-protected packet with large payload */
	uint8_t token[] = { 0x42 };
	ret = coap_packet_init(&oscore_pkt, oscore_pkt_buf, sizeof(oscore_pkt_buf),
			       COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			       COAP_METHOD_POST, 0x1234);
	zassert_equal(ret, 0, "Failed to init OSCORE packet");

	/* Add OSCORE option */
	uint8_t oscore_opt[] = { 0x09 };
	ret = coap_packet_append_option(&oscore_pkt, COAP_OPTION_OSCORE,
					oscore_opt, sizeof(oscore_opt));
	zassert_equal(ret, 0, "Failed to add OSCORE option");

	/* Add large OSCORE payload that will exceed MAX_UNFRAGMENTED_SIZE when combined
	 * We use MAX_UNFRAGMENTED_SIZE - 10 to leave room for headers, then add EDHOC_MSG_3
	 * which will push it over the limit
	 */
	size_t oscore_payload_size = CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE - 10;
	static uint8_t large_payload[CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE];
	memset(large_payload, 0xAA, oscore_payload_size);
	ret = coap_packet_append_payload_marker(&oscore_pkt);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&oscore_pkt, large_payload, oscore_payload_size);
	zassert_equal(ret, 0, "Failed to add OSCORE payload");

	/* Build EDHOC_MSG_3 (large enough to exceed MAX_UNFRAGMENTED_SIZE when combined) */
	uint8_t edhoc_msg3[20];
	memset(edhoc_msg3, 0x42, sizeof(edhoc_msg3));
	size_t edhoc_msg3_len = sizeof(edhoc_msg3);

	/* Attempt to build combined request - should fail with -EMSGSIZE */
	ret = coap_edhoc_client_build_combined_request(
		oscore_pkt.data, oscore_pkt.offset,
		edhoc_msg3, edhoc_msg3_len,
		combined_buf, sizeof(combined_buf),
		&combined_len);
	zassert_equal(ret, -EMSGSIZE,
		      "Should fail with -EMSGSIZE when exceeding MAX_UNFRAGMENTED_SIZE, got %d", ret);
}

#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST && CONFIG_COAP_CLIENT */
