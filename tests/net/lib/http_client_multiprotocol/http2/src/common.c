/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <strings.h>

#include "common.h"

static void copy_str(char *dst, size_t size, const char *src)
{
	strncpy(dst, src, size - 1);
	dst[size - 1] = '\0';
}

void test_response_init(struct test_response *rsp, uint8_t *body, size_t body_size)
{
	memset(rsp, 0, sizeof(*rsp));
	rsp->body = body;
	rsp->body_size = body_size;
	k_sem_init(&rsp->done, 0, 1);
}

int test_response_cb(struct http_client *client, const struct http_client_request *request,
		     struct http_client_response *response,
		     enum http_client_transaction_status status, void *user_data)
{
	struct test_response *rsp = user_data;

	ARG_UNUSED(client);
	ARG_UNUSED(request);

	if (status == HTTP_CLIENT_TRANSACTION_ABORTED) {
		rsp->aborted = true;
		rsp->abort_count++;
		rsp->error = response->error;
		k_sem_give(&rsp->done);
		return 0;
	}

	rsp->status = response->status_code;
	rsp->content_len_present = response->content_len_present;
	rsp->content_len = response->content_len;

	if (response->header_count > 0) {
		if (rsp->header_blocks < TEST_MAX_BLOCKS) {
			rsp->block_status[rsp->header_blocks] = response->status_code;
		}

		rsp->header_blocks++;

		for (size_t i = 0; i < response->header_count; i++) {
			struct test_header *hdr;

			if (rsp->header_count >= TEST_MAX_HEADERS) {
				break;
			}

			hdr = &rsp->headers[rsp->header_count++];
			copy_str(hdr->name, sizeof(hdr->name), response->headers[i].name);
			copy_str(hdr->value, sizeof(hdr->value), response->headers[i].value);
		}

		if (rsp->header_cb_ret != 0) {
			return rsp->header_cb_ret;
		}
	}

	if (response->body_len > 0) {
		rsp->body_chunks++;

		if (rsp->body != NULL && rsp->body_len + response->body_len <= rsp->body_size) {
			memcpy(rsp->body + rsp->body_len, response->body, response->body_len);
		} else {
			rsp->body_overflow = true;
		}

		rsp->body_len += response->body_len;

		if (rsp->body_cb_ret != 0) {
			return rsp->body_cb_ret;
		}
	}

	if (status == HTTP_CLIENT_RESPONSE_DATA_FINAL) {
		rsp->final_count++;
		k_sem_give(&rsp->done);
	}

	return 0;
}

const char *test_response_header(const struct test_response *rsp, const char *name)
{
	for (size_t i = 0; i < rsp->header_count; i++) {
		if (strcasecmp(rsp->headers[i].name, name) == 0) {
			return rsp->headers[i].value;
		}
	}

	return NULL;
}

int test_response_wait(struct test_response *rsp, k_timeout_t timeout)
{
	return k_sem_take(&rsp->done, timeout);
}
