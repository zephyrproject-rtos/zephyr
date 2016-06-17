/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <misc/printk.h>

#include <nanokernel.h>

#include <net/buf.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <net/net_core.h>

#include <tc_util.h>

#include <zoap.h>

#define ZOAP_BUF_SIZE 128
#define ZOAP_LIMITED_BUF_SIZE 9

static struct nano_fifo zoap_fifo;
static NET_BUF_POOL(zoap_pool, 1, ZOAP_BUF_SIZE,
		    &zoap_fifo, NULL, sizeof(struct ip_buf));

static struct nano_fifo zoap_limited_fifo;
static NET_BUF_POOL(zoap_limited_pool, 1, ZOAP_LIMITED_BUF_SIZE,
		    &zoap_limited_fifo, NULL, sizeof(struct ip_buf));

static int test_build_empty_pdu(void)
{
	uint8_t result_pdu[] = { 0x40, 0x01, 0x0, 0x0 };
	struct zoap_packet pkt;
	struct net_buf *buf;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_CON);
	zoap_header_set_code(&pkt, ZOAP_METHOD_GET);
	zoap_header_set_id(&pkt, 0);

	if (ip_buf_appdatalen(buf) != sizeof(result_pdu)) {
		TC_PRINT("Failed to build packet\n");
		goto done;
	}

	if (memcmp(result_pdu, ip_buf_appdata(buf), ip_buf_appdatalen(buf))) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_build_simple_pdu(void)
{
	uint8_t result_pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
				 'n', 0xC1, 0x00, 0xFF, 'p', 'a', 'y', 'l',
				 'o', 'a', 'd', 0x00 };
	struct zoap_packet pkt;
	struct net_buf *buf;
	const char token[] = "token";
	uint8_t *appdata, payload[] = "payload";
	uint16_t buflen;
	uint8_t format = 0;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_NON_CON);
	zoap_header_set_code(&pkt,
			      ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED);
	zoap_header_set_id(&pkt, 0x1234);

	r = zoap_header_set_token(&pkt, (uint8_t *)token, strlen(token));
	if (r) {
		TC_PRINT("Could not set token\n");
		goto done;
	}

	r = zoap_add_option(&pkt, ZOAP_OPTION_CONTENT_FORMAT,
			     &format, sizeof(format));
	if (r) {
		TC_PRINT("Could not add option\n");
		goto done;
	}

	appdata = zoap_packet_get_payload(&pkt, &buflen);
	if (!buf || buflen <= sizeof(payload)) {
		TC_PRINT("Not enough space to insert payload\n");
		goto done;
	}

	memcpy(appdata, payload, sizeof(payload));

	r = zoap_packet_set_used(&pkt, sizeof(payload));
	if (r) {
		TC_PRINT("Failed to set the amount of bytes used\n");
		goto done;
	}

	if (ip_buf_appdatalen(buf) != sizeof(result_pdu)) {
		TC_PRINT("Different size from the reference packet\n");
		goto done;
	}

	if (memcmp(result_pdu, ip_buf_appdata(buf), ip_buf_appdatalen(buf))) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_build_no_size_for_options(void)
{
	struct zoap_packet pkt;
	struct net_buf *buf;
	const char token[] = "token";
	uint8_t format = 0;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_limited_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_NON_CON);
	zoap_header_set_code(&pkt,
			      ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED);
	zoap_header_set_id(&pkt, 0x1234);

	r = zoap_header_set_token(&pkt, (uint8_t *)token, strlen(token));
	if (r) {
		TC_PRINT("Could not set token\n");
		goto done;
	}

	/* There won't be enough space for the option value */
	r = zoap_add_option(&pkt, ZOAP_OPTION_CONTENT_FORMAT,
			     &format, sizeof(format));
	if (!r) {
		TC_PRINT("Shouldn't have added the option, not enough space\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_empty_pdu(void)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0 };
	struct net_buf *buf;
	struct zoap_packet pkt;
	uint8_t ver, type, code;
	uint16_t id;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	memcpy(ip_buf_appdata(buf), pdu, sizeof(pdu));
	ip_buf_appdatalen(buf) = sizeof(pdu);

	r = zoap_packet_parse(&pkt, buf);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = zoap_header_get_version(&pkt);
	type = zoap_header_get_type(&pkt);
	code = zoap_header_get_code(&pkt);
	id = zoap_header_get_id(&pkt);

	if (ver != 1) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != ZOAP_TYPE_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != ZOAP_METHOD_GET) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_simple_pdu(void)
{
	uint8_t pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
			  'n',  0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y',
			  'l', 'o', 'a', 'd', 0x00 };
	struct zoap_packet pkt;
	struct net_buf *buf;
	struct zoap_option options[16];
	uint8_t ver, type, code, tkl;
	const uint8_t *token;
	uint16_t id;
	int result = TC_FAIL;
	int r, count = 16;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	memcpy(ip_buf_appdata(buf), pdu, sizeof(pdu));
	ip_buf_appdatalen(buf) = sizeof(pdu);

	r = zoap_packet_parse(&pkt, buf);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = zoap_header_get_version(&pkt);
	type = zoap_header_get_type(&pkt);
	code = zoap_header_get_code(&pkt);
	id = zoap_header_get_id(&pkt);

	if (ver != 1) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != ZOAP_TYPE_NON_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0x1234) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	token = zoap_header_get_token(&pkt, &tkl);

	if (!token) {
		TC_PRINT("Couldn't extract token from packet\n");
		goto done;
	}

	if (tkl != 5) {
		TC_PRINT("Token length doesn't match reference\n");
		goto done;
	}

	if (memcmp(token, "token", tkl)) {
		TC_PRINT("Token value doesn't match the reference\n");
		goto done;
	}

	count = zoap_find_options(&pkt, ZOAP_OPTION_CONTENT_FORMAT,
				   options, count);
	if (count != 1) {
		TC_PRINT("Unexpected number of options in the packet\n");
		goto done;
	}

	if (options[0].len != 1) {
		TC_PRINT("Option length doesn't match the reference\n");
		goto done;
	}

	if (((uint8_t *)options[0].value)[0] != 0) {
		TC_PRINT("Option value doesn't match the reference\n");
		goto done;
	}

	/* Not existent */
	count = zoap_find_options(&pkt, ZOAP_OPTION_ETAG, options, count);
	if (count) {
		TC_PRINT("There shouldn't be any ETAG option in the packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static const struct {
	const char *name;
	int (*func)(void);
} tests[] = {
	{ "Build empty PDU test", test_build_empty_pdu, },
	{ "Build simple PDU test", test_build_simple_pdu, },
	{ "No size for options test", test_build_no_size_for_options, },
	{ "Parse emtpy PDU test", test_parse_empty_pdu, },
	{ "Parse simple PDU test", test_parse_simple_pdu, },
};

int main(int argc, char *argv[])
{
	int count, pass, result;

	TC_START("Test Zoap CoAP PDU parsing and building");

	net_buf_pool_init(zoap_pool);
	net_buf_pool_init(zoap_limited_pool);

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		if (tests[count].func() == TC_PASS) {
			pass++;
		}
	}

	TC_PRINT("%d / %d tests passed\n", pass, count);

	result = pass == count ? TC_PASS : TC_FAIL;

	TC_END_REPORT(result);

	return 0;
}
