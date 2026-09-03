/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/coap.h>
#include <zephyr/net/coap_link_format.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_COAP_WELL_KNOWN_BLOCK_WISE)

#define WK_BUF_SIZE 256U

#define WK_DEFAULT_BLOCK_SIZE CONFIG_COAP_WELL_KNOWN_BLOCK_WISE_SIZE

static const char * const path_a[] = { "large", NULL };
static const char * const attrs_a[] = { "rt=\"observe\"", "if=\"core.s\"", NULL };
static struct coap_core_metadata meta_a = { .attributes = attrs_a };

static const char * const path_b[] = { "location", "1", NULL };

static const char * const path_c[] = { "switch", NULL };
static const char * const attrs_c[] = { "rt=\"actuator\"", NULL };
static struct coap_core_metadata meta_c = { .attributes = attrs_c };

static struct coap_resource test_resources[] = {
	{ .path = path_a, .metadata = &meta_a },
	{ .path = path_b },
	{ .path = path_c, .metadata = &meta_c },
};

static const char expected_doc[] =
	"</large>;rt=\"observe\";if=\"core.s\",</location/1>,</switch>;rt=\"actuator\"";

/* The tests request blocks of up to 32 bytes and expect the document to span
 * multiple blocks of the default size.
 */
BUILD_ASSERT(WK_DEFAULT_BLOCK_SIZE >= 32, "Tests request blocks of up to 32 bytes");
BUILD_ASSERT(sizeof(expected_doc) - 1U > WK_DEFAULT_BLOCK_SIZE,
	     "Document must span multiple blocks");

struct wk_transfer {
	uint8_t doc[WK_BUF_SIZE];
	size_t len;
	uint8_t etag[4];
};

/* Issue a .well-known/core GET, with a Block2 option when block_num >= 0 */
static int well_known_req(struct coap_packet *response, uint8_t *buf, uint16_t buf_len,
			  size_t num_resources, int block_num, enum coap_block_size size)
{
	struct coap_packet request;
	uint8_t req_buf[64];
	int r;

	r = coap_packet_init(&request, req_buf, sizeof(req_buf), COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(), COAP_METHOD_GET,
			     coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	if (block_num >= 0) {
		r = coap_append_option_int(&request, COAP_OPTION_BLOCK2,
					   (block_num << 4) | size);
		zassert_equal(r, 0, "Failed to append block2 option");
	}

	return coap_well_known_core_get_len(test_resources, num_resources, &request, response,
					    buf, buf_len);
}

/* Fetch one block into the transfer, verify its options, return the more flag */
static bool fetch_block(struct wk_transfer *tr, int block_num, enum coap_block_size size)
{
	struct coap_packet response;
	struct coap_packet parsed;
	struct coap_option etag_opt = { 0 };
	uint8_t buf[WK_BUF_SIZE];
	const uint8_t *payload;
	uint16_t payload_len;
	int block2;
	int r;

	r = well_known_req(&response, buf, sizeof(buf), ARRAY_SIZE(test_resources), block_num,
			   size);
	zassert_equal(r, 0, "Well-known request failed (%d)", r);

	r = coap_packet_parse(&parsed, buf, response.offset, NULL, 0U);
	zassert_equal(r, 0, "Failed to parse response (%d)", r);

	block2 = coap_get_option_int(&parsed, COAP_OPTION_BLOCK2);
	zassert_true(block2 >= 0, "No block2 option in response");
	zassert_equal(GET_BLOCK_NUM(block2), block_num, "Unexpected block number");
	zassert_equal(GET_BLOCK_SIZE(block2), size, "Unexpected block size");

	if (IS_ENABLED(CONFIG_SYS_HASH_FUNC32)) {
		r = coap_find_options(&parsed, COAP_OPTION_ETAG, &etag_opt, 1U);
		zassert_equal(r, 1, "No ETag option in response");
		zassert_equal(etag_opt.len, sizeof(tr->etag), "Unexpected ETag length");

		if (block_num == 0) {
			memcpy(tr->etag, etag_opt.value, sizeof(tr->etag));
		} else {
			zassert_mem_equal(etag_opt.value, tr->etag, sizeof(tr->etag),
					  "ETag changed during transfer");
		}
	}

	payload = coap_packet_get_payload(&parsed, &payload_len);
	zassert_not_null(payload, "No payload in response");
	zassert_true(tr->len + payload_len <= sizeof(tr->doc), "Transfer too large");
	zassert_equal(tr->len, (size_t)block_num * coap_block_size_to_bytes(size),
		      "Unexpected payload offset");

	memcpy(&tr->doc[tr->len], payload, payload_len);
	tr->len += payload_len;

	return GET_MORE(block2);
}

ZTEST(coap_wellknown, test_blockwise_reassembly)
{
	struct wk_transfer tr = { 0 };
	bool more = true;

	for (int i = 0; more; i++) {
		more = fetch_block(&tr, i, COAP_BLOCK_16);
	}

	zassert_equal(tr.len, strlen(expected_doc), "Unexpected document length");
	zassert_mem_equal(tr.doc, expected_doc, tr.len, "Unexpected document content");
}

ZTEST(coap_wellknown, test_blockwise_interleaved)
{
	struct wk_transfer tr_a = { 0 };
	struct wk_transfer tr_b = { 0 };
	bool more_a = true;
	bool more_b = true;

	/* Two interleaved transfers with different block sizes must not
	 * interfere with each other.
	 */
	for (int i = 0; more_a || more_b; i++) {
		if (more_a) {
			more_a = fetch_block(&tr_a, i, COAP_BLOCK_16);
		}
		if (more_b) {
			more_b = fetch_block(&tr_b, i, COAP_BLOCK_32);
		}
	}

	zassert_equal(tr_a.len, strlen(expected_doc), "Unexpected document length");
	zassert_mem_equal(tr_a.doc, expected_doc, tr_a.len, "Unexpected document content");
	zassert_equal(tr_b.len, strlen(expected_doc), "Unexpected document length");
	zassert_mem_equal(tr_b.doc, expected_doc, tr_b.len, "Unexpected document content");
}

ZTEST(coap_wellknown, test_blockwise_fresh_after_abandoned)
{
	struct coap_packet response;
	struct coap_packet parsed;
	struct wk_transfer tr = { 0 };
	uint8_t buf[WK_BUF_SIZE];
	const uint8_t *payload;
	uint16_t payload_len;
	int block2;
	int r;

	/* Abandon a transfer after two blocks */
	(void)fetch_block(&tr, 0, COAP_BLOCK_16);
	(void)fetch_block(&tr, 1, COAP_BLOCK_16);

	/* A fresh request without a block2 option starts over at block 0 */
	r = well_known_req(&response, buf, sizeof(buf), ARRAY_SIZE(test_resources), -1,
			   COAP_BLOCK_16);
	zassert_equal(r, 0, "Well-known request failed (%d)", r);

	r = coap_packet_parse(&parsed, buf, response.offset, NULL, 0U);
	zassert_equal(r, 0, "Failed to parse response (%d)", r);

	block2 = coap_get_option_int(&parsed, COAP_OPTION_BLOCK2);
	zassert_true(block2 >= 0, "No block2 option in response");
	zassert_equal(GET_BLOCK_NUM(block2), 0, "Fresh request did not start at block 0");
	zassert_true(GET_MORE(block2), "Expected more blocks");

	payload = coap_packet_get_payload(&parsed, &payload_len);
	zassert_not_null(payload, "No payload in response");
	zassert_equal(payload_len, WK_DEFAULT_BLOCK_SIZE, "Unexpected payload length");
	zassert_mem_equal(payload, expected_doc, payload_len, "Unexpected document content");
}

ZTEST(coap_wellknown, test_blockwise_etag_representation)
{
	struct coap_packet response;
	struct coap_packet parsed;
	struct coap_option etag_opt = { 0 };
	struct wk_transfer tr_a = { 0 };
	struct wk_transfer tr_b = { 0 };
	uint8_t buf[WK_BUF_SIZE];
	int r;

	if (!IS_ENABLED(CONFIG_SYS_HASH_FUNC32)) {
		ztest_test_skip();
	}

	/* The same resource set produces the same ETag across transfers */
	(void)fetch_block(&tr_a, 0, COAP_BLOCK_16);
	(void)fetch_block(&tr_b, 0, COAP_BLOCK_16);
	zassert_mem_equal(tr_a.etag, tr_b.etag, sizeof(tr_a.etag),
			  "ETag differs for the same resource set");

	/* A different resource set produces a different ETag */
	r = well_known_req(&response, buf, sizeof(buf), ARRAY_SIZE(test_resources) - 1U, 0,
			   COAP_BLOCK_16);
	zassert_equal(r, 0, "Well-known request failed (%d)", r);

	r = coap_packet_parse(&parsed, buf, response.offset, NULL, 0U);
	zassert_equal(r, 0, "Failed to parse response (%d)", r);

	r = coap_find_options(&parsed, COAP_OPTION_ETAG, &etag_opt, 1U);
	zassert_equal(r, 1, "No ETag option in response");
	zassert_equal(etag_opt.len, sizeof(tr_a.etag), "Unexpected ETag length");
	zassert_true(memcmp(etag_opt.value, tr_a.etag, sizeof(tr_a.etag)) != 0,
		     "ETag did not change with the resource set");
}

ZTEST_SUITE(coap_wellknown, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_COAP_WELL_KNOWN_BLOCK_WISE */
