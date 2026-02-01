/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


struct test_coap_request {
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;
	uint8_t code;
	enum coap_msgtype type;
	struct coap_reply *match;
};

static int reply_cb(const struct coap_packet *response,
		    struct coap_reply *reply,
		    const struct net_sockaddr *from)
{
	return 0;
}

ZTEST(coap, test_response_matching)
{
	struct coap_reply matches[] = {
		{ }, /* Non-initialized (unused) entry. */
		{ .id = 100, .reply = reply_cb },
		{ .id = 101, .token = { 1, 2, 3, 4 }, .tkl = 4, .reply = reply_cb },
	};
	struct test_coap_request test_responses[] = {
		/* #0 Piggybacked ACK, empty token */
		{ .id = 100, .type = COAP_TYPE_ACK, .match = &matches[1],
		  .code = COAP_RESPONSE_CODE_CONTENT },
		/* #1 Piggybacked ACK, matching token */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = &matches[2],
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4  },
		/* #2 Piggybacked ACK, token mismatch */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 3 },
		  .tkl = 4 },
		/* #3 Piggybacked ACK, token mismatch 2 */
		{ .id = 100, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #4 Piggybacked ACK, token mismatch 3 */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3 },
		  .tkl = 3 },
		/* #5 Piggybacked ACK, token mismatch 4 */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT },
		/* #6 Piggybacked ACK, id mismatch */
		{ .id = 102, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #7 Separate reply, empty token */
		{ .id = 101, .type = COAP_TYPE_CON, .match = &matches[1],
		  .code = COAP_RESPONSE_CODE_CONTENT },
		/* #8 Separate reply, matching token 1 */
		{ .id = 101, .type = COAP_TYPE_CON, .match = &matches[2],
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #9 Separate reply, matching token 2 */
		{ .id = 102, .type = COAP_TYPE_CON, .match = &matches[2],
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #10 Separate reply, token mismatch */
		{ .id = 101, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 3 },
		  .tkl = 4 },
		/* #11 Separate reply, token mismatch 2 */
		{ .id = 100, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 3 },
		  .tkl = 4 },
		/* #12 Separate reply, token mismatch 3 */
		{ .id = 100, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3 },
		  .tkl = 3 },
		/* #13 Request, empty token */
		{ .id = 100, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_METHOD_GET },
		/* #14 Request, matching token */
		{ .id = 101, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_METHOD_GET, .token = { 1, 2, 3, 4 }, .tkl = 4 },
		/* #15 Empty ACK */
		{ .id = 100, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_CODE_EMPTY },
		/* #16 Empty ACK 2 */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_CODE_EMPTY },
		/* #17 Empty RESET */
		{ .id = 100, .type = COAP_TYPE_RESET, .match = &matches[1],
		  .code = COAP_CODE_EMPTY },
		/* #18 Empty RESET 2 */
		{ .id = 101, .type = COAP_TYPE_RESET, .match = &matches[2],
		  .code = COAP_CODE_EMPTY },
		/* #19 Empty RESET, id mismatch */
		{ .id = 102, .type = COAP_TYPE_RESET, .match = NULL,
		  .code = COAP_CODE_EMPTY },
	};

	ARRAY_FOR_EACH_PTR(test_responses, response) {
		struct coap_packet response_pkt = { 0 };
		struct net_sockaddr from = { 0 };
		struct coap_reply *match;
		uint8_t data[64];
		int ret;

		ret = coap_packet_init(&response_pkt, data, sizeof(data), COAP_VERSION_1,
				       response->type, response->tkl, response->token,
				       response->code, response->id);
		zassert_ok(ret, "Failed to initialize test packet: %d", ret);

		match = coap_response_received(&response_pkt, &from, matches,
					       ARRAY_SIZE(matches));
		if (response->match != NULL) {
			zassert_not_null(match, "Did not found a response match when expected");
			zassert_equal_ptr(response->match, match,
					  "Wrong response match, test %d match %d",
					  response - test_responses, match - matches);
		} else {
			zassert_is_null(match,
					"Found unexpected response match, test %d match %d",
					response - test_responses, match - matches);
		}
	}
}
