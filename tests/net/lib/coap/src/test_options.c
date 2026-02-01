/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


ZTEST(coap, test_handle_invalid_coap_req)
{
	struct coap_packet pkt;
	uint8_t *data = data_buf[0];
	struct coap_option options[4] = {};
	uint8_t opt_num = 4;
	int r;
	const char *const *p;

	r = coap_packet_init(&pkt, data, COAP_BUF_SIZE, COAP_VERSION_1,
					COAP_TYPE_CON, 0, NULL, 0xFF, coap_next_id());

	zassert_equal(r, 0, "Unable to init req");

	for (p = server_resource_1_path; p && *p; p++) {
		r = coap_packet_append_option(&pkt, COAP_OPTION_URI_PATH,
					*p, strlen(*p));
		zassert_equal(r, 0, "Unable to append option");
	}

	r = coap_packet_parse(&pkt, data, pkt.offset, options, opt_num);
	zassert_equal(r, 0, "Could not parse req packet");

	r = coap_handle_request(&pkt, server_resources, options, opt_num,
					(struct net_sockaddr *) &dummy_addr, sizeof(dummy_addr));
	zassert_equal(r, -ENOTSUP, "Request handling should fail with -ENOTSUP");
}

ZTEST(coap, test_build_options_out_of_order_0)
{
	uint8_t result[] = {0x45, 0x02, 0x12, 0x34, 't', 'o', 'k',  'e', 'n', 0xC0, 0xB1, 0x19,
			    0xC5, 'p',	'r',  'o',  'x', 'y', 0x44, 'c', 'o', 'a',  'p'};
	struct coap_packet cpkt;
	static const char token[] = "token";
	uint8_t *data = data_buf[0];
	int r;

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_options_0 = 0xc0; /* content format */

	zassert_mem_equal(&expected_options_0, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	const char *proxy_uri = "proxy";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_PROXY_URI, proxy_uri, strlen(proxy_uri));
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_options_1[] = {
		0xc0,				    /* content format */
		0xd5, 0x0a, 'p', 'r', 'o', 'x', 'y' /* proxy url */
	};
	zassert_mem_equal(expected_options_1, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	const char *proxy_scheme = "coap";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_PROXY_SCHEME, proxy_scheme,
				      strlen(proxy_scheme));
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_options_2[] = {
		0xc0,				     /*  content format */
		0xd5, 0x0a, 'p', 'r', 'o', 'x', 'y', /*  proxy url */
		0x44, 'c',  'o', 'a', 'p'	     /*  proxy scheme */
	};
	zassert_mem_equal(expected_options_2, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	/*  option out of order */
	const uint8_t block_option = 0b11001;

	r = coap_append_option_int(&cpkt, COAP_OPTION_BLOCK2, block_option);
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_options_3[] = {
		0xc0,				/*  content format */
		0xb1, 0x19,			/*  block2 */
		0xc5, 'p',  'r', 'o', 'x', 'y', /*  proxy url */
		0x44, 'c',  'o', 'a', 'p'	/*  proxy scheme */
	};
	zassert_mem_equal(expected_options_3, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	/*  look for options */
	struct coap_option opt;

	r = coap_find_options(&cpkt, COAP_OPTION_CONTENT_FORMAT, &opt, 1);
	zassert_equal(r, 1, "Could not find option");

	r = coap_find_options(&cpkt, COAP_OPTION_PROXY_URI, &opt, 1);
	zassert_equal(r, 1, "Could not find option");
	zassert_equal(opt.len, strlen(proxy_uri), "Wrong option len");
	zassert_mem_equal(opt.value, proxy_uri, strlen(proxy_uri), "Wrong option content");

	r = coap_find_options(&cpkt, COAP_OPTION_PROXY_SCHEME, &opt, 1);
	zassert_equal(r, 1, "Could not find option");
	zassert_equal(opt.len, strlen(proxy_scheme), "Wrong option len");
	zassert_mem_equal(opt.value, proxy_scheme, strlen(proxy_scheme), "Wrong option content");

	r = coap_find_options(&cpkt, COAP_OPTION_BLOCK2, &opt, 1);
	zassert_equal(r, 1, "Could not find option");
	zassert_equal(opt.len, 1, "Wrong option len");
	zassert_equal(*opt.value, block_option, "Wrong option content");

	zassert_equal(cpkt.hdr_len, 9, "Wrong header len");
	zassert_equal(cpkt.opt_len, 14, "Wrong options size");
	zassert_equal(cpkt.delta, 39, "Wrong delta");

	zassert_equal(cpkt.offset, 23, "Wrong data size");

	zassert_mem_equal(result, cpkt.data, cpkt.offset,
			  "Built packet doesn't match reference packet");
}

#define ASSERT_OPTIONS(cpkt, expected_opt_len, expected_data, expected_data_len)                   \
	do {                                                                                       \
		static const uint8_t expected_hdr_len = 9;                                         \
		zassert_equal(expected_hdr_len, cpkt.hdr_len, "Wrong header length");              \
		zassert_equal(expected_opt_len, cpkt.opt_len, "Wrong option length");              \
		zassert_equal(expected_hdr_len + expected_opt_len, cpkt.offset, "Wrong offset");   \
		zassert_equal(expected_data_len, cpkt.offset, "Wrong offset");                     \
		zassert_mem_equal(expected_data, cpkt.data, expected_data_len, "Wrong data");      \
	} while (0)

ZTEST(coap, test_build_options_out_of_order_1)
{
	struct coap_packet cpkt;

	static const char token[] = "token";

	uint8_t *data = data_buf[0];

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	int r;

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE2,
				   coap_block_size_to_bytes(COAP_BLOCK_128));
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 't',  'o',
					     'k',  'e',	 'n',  0xd1, 0x0f, 0x80};
	ASSERT_OPTIONS(cpkt, 3, expected_0, 12);

	const char *uri_path = "path";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_1[] = {
		0x45, 0x02, 0x12, 0x34, 't', 'o',  'k',	 'e',  'n',
		0xb4, 'p',  'a',  't',	'h', 0xd1, 0x04, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 8, expected_1, 17);

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_2[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0xb4,
		'p',  'a',  't',  'h',	0x11, 0x32, 0xd1, 0x03, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 10, expected_2, 19);

	const char *uri_host = "hostname";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_HOST, uri_host, strlen(uri_host));
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_3[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o', 'k', 'e', 'n', 0x38, 'h',  'o',  's',  't',
		'n',  'a',  'm',  'e',	0x84, 'p', 'a', 't', 'h', 0x11, 0x32, 0xd1, 0x03, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 19, expected_3, 28);

	r = coap_append_option_int(&cpkt, COAP_OPTION_URI_PORT, 5638);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_4[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',
		'o',  's',  't',  'n',	'a',  'm',  'e',  'B',	0x16, 0x06, 'D',
		'p',  'a',  't',  'h',	0x11, 0x32, 0xd1, 0x03, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 22, expected_4, 31);

	const char *uri_query0 = "query0";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_QUERY, uri_query0, strlen(uri_query0));
	zassert_equal(r, 0, "Could not append option");

	const char *uri_query1 = "query1";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_QUERY, uri_query1, strlen(uri_query1));
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_5[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',
		's',  't',  'n',  'a',	'm',  'e',  'B',  0x16, 0x06, 'D',  'p',  'a',
		't',  'h',  0x11, 0x32, 0x36, 'q',  'u',  'e',	'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0xd1, 0x00, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 36, expected_5, 45);

	r = coap_append_option_int(&cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_CBOR);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_6[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',
		's',  't',  'n',  'a',	'm',  'e',  'B',  0x16, 0x06, 'D',  'p',  'a',
		't',  'h',  0x11, 0x32, 0x36, 'q',  'u',  'e',	'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 37, expected_6, 46);

	r = coap_append_option_int(&cpkt, COAP_OPTION_OBSERVE, 0);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_7[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',
		's',  't',  'n',  'a',	'm',  'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p',
		'a',  't',  'h',  0x11, 0x32, 0x36, 'q',  'u',	'e',  'r',  'y',  0x30,
		0x06, 'q',  'u',  'e',	'r',  'y',  0x31, 0x21, 0x3c, 0xb1, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 38, expected_7, 47);

	r = coap_append_option_int(&cpkt, COAP_OPTION_MAX_AGE, 3);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_8[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h', 'o',  's',
		't',  'n',  'a',  'm',	'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p', 'a',  't',
		'h',  0x11, 0x32, 0x21, 0x03, 0x16, 'q',  'u',	'e',  'r',  'y', 0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 40, expected_8, 49);

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE1, 64);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_9[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',	's',
		't',  'n',  'a',  'm',	'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p',  'a',	't',
		'h',  0x11, 0x32, 0x21, 0x03, 0x16, 'q',  'u',	'e',  'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40,
	};

	ASSERT_OPTIONS(cpkt, 43, expected_9, 52);

	zassert_equal(cpkt.hdr_len, 9, "Wrong header len");
	zassert_equal(cpkt.opt_len, 43, "Wrong options size");
	zassert_equal(cpkt.delta, 60, "Wrong delta");
	zassert_equal(cpkt.offset, 52, "Wrong data size");
}

#define ASSERT_OPTIONS_AND_PAYLOAD(cpkt, expected_opt_len, expected_data, expected_offset,         \
				   expected_delta)                                                 \
	do {                                                                                       \
		size_t expected_data_l = ARRAY_SIZE(expected_data);                                \
		zassert_equal(expected_offset, expected_data_l);                                   \
		static const uint8_t expected_hdr_len = 9;                                         \
		zassert_equal(expected_hdr_len, cpkt.hdr_len, "Wrong header length");              \
		zassert_equal(expected_opt_len, cpkt.opt_len, "Wrong option length");              \
		zassert_equal(expected_offset, cpkt.offset, "Wrong offset");                       \
		zassert_mem_equal(expected_data, cpkt.data, expected_offset, "Wrong data");        \
		zassert_equal(expected_delta, cpkt.delta, "Wrong delta");                          \
	} while (0)

static void init_basic_test_msg(struct coap_packet *cpkt, uint8_t *data)
{
	static const char token[] = "token";
	const char *uri_path = "path";
	const char *uri_host = "hostname";
	const char *uri_query0 = "query0";
	const char *uri_query1 = "query1";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	int r;

	r = coap_packet_init(cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(cpkt, COAP_OPTION_SIZE2,
				   coap_block_size_to_bytes(COAP_BLOCK_128));
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_CONTENT_FORMAT, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_HOST, uri_host, strlen(uri_host));
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_URI_PORT, 5638);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_QUERY, uri_query0, strlen(uri_query0));
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_QUERY, uri_query1, strlen(uri_query1));
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_CBOR);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_OBSERVE, 0);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_MAX_AGE, 3);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_SIZE1, 64);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_9[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',	's',
		't',  'n',  'a',  'm',	'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p',  'a',	't',
		'h',  0x11, 0x32, 0x21, 0x03, 0x16, 'q',  'u',	'e',  'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40,
	};

	ASSERT_OPTIONS((*cpkt), 43, expected_9, 52);

	r = coap_packet_append_payload_marker(cpkt);
	zassert_equal(r, 0, "Could not append payload marker");

	static const uint8_t test_payload[] = {0xde, 0xad, 0xbe, 0xef};

	r = coap_packet_append_payload(cpkt, test_payload, ARRAY_SIZE(test_payload));
	zassert_equal(r, 0, "Could not append test payload");

	zassert_equal((*cpkt).hdr_len, 9, "Wrong header len");
	zassert_equal((*cpkt).opt_len, 43, "Wrong options size");
	zassert_equal((*cpkt).delta, 60, "Wrong delta");
	zassert_equal((*cpkt).offset, 57, "Wrong data size");
}

ZTEST(coap, test_remove_first_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_HOST);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x60, 0x12, 0x16,
		0x06, 0x44, 0x70, 0x61, 0x74, 0x68, 0x11, 0x32, 0x21, 0x03, 0x16, 0x71,
		0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72, 0x79, 0x31,
		0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 34, expected_0, 48, 60);
}

ZTEST(coap, test_remove_middle_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_OBSERVE);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73, 0x74,
		0x6e, 0x61, 0x6d, 0x65, 0x42, 0x16, 0x06, 0x44, 0x70, 0x61, 0x74, 0x68, 0x11, 0x32,
		0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72,
		0x79, 0x31, 0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 42, expected_0, 56, 60);
}

ZTEST(coap, test_remove_last_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_SIZE1);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73, 0x74,
		0x6e, 0x61, 0x6d, 0x65, 0x30, 0x12, 0x16, 0x06, 0x44, 0x70, 0x61, 0x74, 0x68, 0x11,
		0x32, 0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65,
		0x72, 0x79, 0x31, 0x21, 0x3c, 0xb1, 0x80, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 40, expected_0, 54, 28);

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE1, 65);
	zassert_equal(r, 0, "Could not add option at end");

	static const uint8_t expected_1[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f,
		0x73, 0x74, 0x6e, 0x61, 0x6d, 0x65, 0x30, 0x12, 0x16, 0x06, 0x44, 0x70,
		0x61, 0x74, 0x68, 0x11, 0x32, 0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72,
		0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72, 0x79, 0x31, 0x21, 0x3c, 0xb1,
		0x80, 0xd1, 0x13, 0x41, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 43, expected_1, 57, 60);
}

ZTEST(coap, test_remove_single_coap_option)
{
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	static const char token[] = "token";
	const char *uri_path = "path";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	int r1;

	r1 = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			      strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r1, 0, "Could not initialize packet");

	r1 = coap_packet_append_option(&cpkt, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
	zassert_equal(r1, 0, "Could not append option");

	r1 = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r1, 0, "Could not append payload marker");

	static const uint8_t test_payload[] = {0xde, 0xad, 0xbe, 0xef};

	r1 = coap_packet_append_payload(&cpkt, test_payload, ARRAY_SIZE(test_payload));
	zassert_equal(r1, 0, "Could not append test payload");

	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xb4, 0x70, 0x61, 0x74, 0x68,
					     0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 5, expected_0, 19, 11);

	/* remove the one and only option */
	r1 = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PATH);
	zassert_equal(r1, 0, "Could not remove option");

	static const uint8_t expected_1[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 0, expected_1, 14, 0);
}

ZTEST(coap, test_remove_repeatable_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73,
		0x74, 0x6e, 0x61, 0x6d, 0x65, 0x30, 0x12, 0x16, 0x06, 0x44, 0x70, 0x61, 0x74,
		0x68, 0x11, 0x32, 0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x31, 0x21,
		0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 36, expected_0, 50, 60);
}

ZTEST(coap, test_remove_all_coap_options)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PORT);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_OBSERVE);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_SIZE1);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73,
		0x74, 0x6e, 0x61, 0x6d, 0x65, 0x84, 0x70, 0x61, 0x74, 0x68, 0x11, 0x32, 0x21,
		0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72,
		0x79, 0x31, 0x21, 0x3c, 0xb1, 0x80, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 36, expected_0, 50, 28);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_HOST);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_SIZE2);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_CONTENT_FORMAT);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_1[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0xb4, 0x70, 0x61, 0x74,
		0x68, 0x31, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75,
		0x65, 0x72, 0x79, 0x31, 0x21, 0x3c, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 23, expected_1, 37, 17);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_ACCEPT);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PATH);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_2[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65,
					     0x6e, 0xd1, 0x01, 0x03, 0x16, 0x71, 0x75, 0x65,
					     0x72, 0x79, 0x31, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 10, expected_2, 24, 15);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_MAX_AGE);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_3[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 0, expected_3, 14, 0);

	/* remove option that is not there anymore */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_MAX_AGE);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 0, expected_3, 14, 0);
}

ZTEST(coap, test_remove_non_existent_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	static const char token[] = "token";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT, COAP_CONTENT_FORMAT_APP_CBOR);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(&cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_OCTET_STREAM);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Could not append payload marker");

	static const uint8_t test_payload[] = {0xde, 0xad, 0xbe, 0xef};

	r = coap_packet_append_payload(&cpkt, test_payload, ARRAY_SIZE(test_payload));

	static const uint8_t expected_original_msg[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f,
							0x6b, 0x65, 0x6e, 0xc1, 0x3c, 0x51,
							0x2a, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);

	/* remove option that is not there but would be before existing options */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PATH);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);

	/* remove option that is not there but would be between existing options */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_MAX_AGE);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);

	/* remove option that is not there but would be after existing options */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_LOCATION_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);
}

ZTEST(coap, test_coap_packet_options_with_large_values)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	static const char token[] = "token";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_MAX_AGE, 3600);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE1, 1048576);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e,
					     0xd2, 0x01, 0x0e, 0x10, 0xd3, 0x21, 0x10, 0x00, 0x00};
	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 9, expected_0, 18, 60);
}

ZTEST(coap, test_coap_packet_options_with_large_delta)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	static const char token[] = "token";
	static const uint8_t payload[] = {0xde, 0xad, 0xbe, 0xef};

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, 65100, 0x5678);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Could not append payload marker");

	r = coap_packet_append_payload(&cpkt, payload, ARRAY_SIZE(payload));
	zassert_equal(r, 0, "Could not append payload");

	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xe2, 0xfd, 0x3f, 0x56, 0x78,
					     0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 5, expected_0, 19, 65100);
}

static void assert_coap_packet_set_path_query_options(const char *path,
						      const char * const *expected,
						      size_t expected_len, uint16_t code)
{
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	struct coap_option options[16] = {0};
	int res;

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));
	TC_PRINT("Assert path: %s\n", path);

	res = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			       COAP_TOKEN_MAX_LEN, coap_next_token(),
			       COAP_METHOD_GET, 0x1234);
	zassert_equal(res, 0, "Could not initialize packet");

	res = coap_packet_set_path(&cpkt, path);
	zassert_equal(res, 0, "Could not set path/query, path: %s", path);

	res = coap_find_options(&cpkt, code, options, ARRAY_SIZE(options));
	if (res <= 0) {
		/* fail if we expect options */
		zassert_true(((expected == NULL) && (expected_len == 0U)),
			     "Expected options but found none, path: %s", path);
	}

	for (unsigned int i = 0U; i < expected_len; ++i) {
		/* validate expected options, the rest shall be 0 */
		if (i < expected_len) {
			zassert_true((options[i].len == strlen(expected[i])),
				     "Expected and parsed option lengths don't match"
				     ", path: %s",
				     path);

			zassert_mem_equal(options[i].value, expected[i], options[i].len,
					  "Expected and parsed option values don't match"
					  ", path: %s",
					  path);
		} else {
			zassert_true((options[i].len == 0U),
				     "Unexpected options shall be empty"
				     ", path: %s",
				     path);
		}
	}
}

ZTEST(coap, test_coap_packet_set_path)
{
	assert_coap_packet_set_path_query_options(" ", NULL, 0U, COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("", NULL, 0U, COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("/", NULL, 0U, COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("?", NULL, 0U, COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("?a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("?a&b",
						 (const char *const[]){"a", "b"}, 2U,
						  COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a", NULL, 0, COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a/",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);

	assert_coap_packet_set_path_query_options("a?b=t&a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a?b=t&a",
						  (const char *const[]){"b=t", "a"}, 2U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a?b=t&aa",
						  (const char *const[]){"b=t", "aa"},
						  2U, COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a?b&a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a?b&a",
						  (const char *const[]){"b", "a"}, 2U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a?b&aa",
						  (const char *const[]){"b", "aa"}, 2U,
						  COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a/b",
						  (const char *const[]){"a", "b"}, 2U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a/b/",
						  (const char *const[]){"a", "b"}, 2U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a/b?b&a",
						  (const char *const[]){"b", "a"}, 2U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a/b?b&aa",
						  (const char *const[]){"b", "aa"}, 2U,
						  COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a/bb",
						  (const char *const[]){"a", "bb"}, 2U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a/bb/",
						  (const char *const[]){"a", "bb"}, 2U,
						  COAP_OPTION_URI_PATH);
}
