/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP2_CLIENT_TEST_COMMON_H_
#define HTTP2_CLIENT_TEST_COMMON_H_

#include <zephyr/kernel.h>
#include <zephyr/net/http/client_multiprotocol.h>

#define TEST_MAX_HEADERS 16
#define TEST_MAX_BLOCKS 4
#define TEST_HEADER_LEN 64
#define TEST_REQUEST_TIMEOUT_MS 5000

struct test_header {
	char name[TEST_HEADER_LEN];
	char value[TEST_HEADER_LEN];
};

/* Everything the response callback records. In an asynchronous session the
 * callback runs on the socket service thread or the client's work queue, so
 * it only records here and the test thread does the asserting.
 */
struct test_response {
	uint8_t *body;
	size_t body_size;
	size_t body_len;
	bool body_overflow;
	uint16_t status;
	bool content_len_present;
	size_t content_len;
	struct test_header headers[TEST_MAX_HEADERS];
	size_t header_count;
	uint16_t block_status[TEST_MAX_BLOCKS];
	int header_blocks;
	int body_chunks;
	int final_count;
	bool aborted;
	int abort_count;
	int error;
	/* Returned from the callback for body chunks, to test aborting. */
	int body_cb_ret;
	/* The same for header blocks. */
	int header_cb_ret;
	/* Given on completion or abort. */
	struct k_sem done;
};

void test_response_init(struct test_response *rsp, uint8_t *body, size_t body_size);
int test_response_cb(struct http_client *client, const struct http_client_request *request,
		     struct http_client_response *response,
		     enum http_client_transaction_status status, void *user_data);
const char *test_response_header(const struct test_response *rsp, const char *name);
int test_response_wait(struct test_response *rsp, k_timeout_t timeout);

#endif /* HTTP2_CLIENT_TEST_COMMON_H_ */
