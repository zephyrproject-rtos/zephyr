/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


#if defined(CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC)

/* Test that /.well-known/core includes EDHOC resource link */
ZTEST(coap, test_well_known_core_edhoc_link)
{
	uint8_t request_buf[128];
	uint8_t response_buf[512];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build GET request to /.well-known/core */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token123",
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options for /.well-known/core */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "core", strlen("core"));
	zassert_equal(r, 0, "Failed to add Uri-Path core");

	/* Call coap_well_known_core_get_len with empty resource list */
	struct coap_resource resources[] = {
		{ .path = (const char * const[]){"test", NULL} },
		{ .path = NULL }
	};

	r = coap_well_known_core_get_len(resources, 1, &request, &response,
					 response_buf, sizeof(response_buf));
	zassert_equal(r, 0, "coap_well_known_core_get_len failed");

	/* Get the payload */
	const uint8_t *payload;
	uint16_t payload_len;

	payload = coap_packet_get_payload(&response, &payload_len);
	zassert_not_null(payload, "Payload should be present");
	zassert_true(payload_len > 0, "Payload should not be empty");

	/* Convert payload to string for easier checking */
	char payload_str[512];

	zassert_true(payload_len < sizeof(payload_str), "Payload too large for buffer");
	memcpy(payload_str, payload, payload_len);
	payload_str[payload_len] = '\0';

	/* Verify EDHOC link is present */
	zassert_not_null(strstr(payload_str, "</.well-known/edhoc>"),
			 "Should contain </.well-known/edhoc>, got: %s", payload_str);
	zassert_not_null(strstr(payload_str, ";rt=core.edhoc"),
			 "Should contain ;rt=core.edhoc, got: %s", payload_str);
	zassert_not_null(strstr(payload_str, ";ed-r"),
			 "Should contain ;ed-r, got: %s", payload_str);

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
	zassert_not_null(strstr(payload_str, ";ed-comb-req"),
			 "Should contain ;ed-comb-req, got: %s", payload_str);
#endif

	/* Verify valueless attributes don't have '=' */
	zassert_is_null(strstr(payload_str, "ed-r="),
			"ed-r should be valueless (no '='), got: %s", payload_str);
	zassert_is_null(strstr(payload_str, "ed-comb-req="),
			"ed-comb-req should be valueless (no '='), got: %s", payload_str);
}

/* Test that /.well-known/core?rt=core.edhoc filters correctly */
ZTEST(coap, test_well_known_core_edhoc_query_filter)
{
	uint8_t request_buf[128];
	uint8_t response_buf[512];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build GET request to /.well-known/core?rt=core.edhoc */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token123",
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "core", strlen("core"));
	zassert_equal(r, 0, "Failed to add Uri-Path core");

	/* Add Uri-Query option for filtering */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_QUERY,
				      "rt=core.edhoc", strlen("rt=core.edhoc"));
	zassert_equal(r, 0, "Failed to add Uri-Query");

	/* Call with resources that don't match the query */
	struct coap_resource resources[] = {
		{ .path = (const char * const[]){"test", NULL} },
		{ .path = NULL }
	};

	r = coap_well_known_core_get_len(resources, 1, &request, &response,
					 response_buf, sizeof(response_buf));
	zassert_equal(r, 0, "coap_well_known_core_get_len failed");

	/* Get the payload */
	const uint8_t *payload;
	uint16_t payload_len;

	payload = coap_packet_get_payload(&response, &payload_len);
	zassert_not_null(payload, "Payload should be present");
	zassert_true(payload_len > 0, "Payload should not be empty");

	/* Convert payload to string */
	char payload_str[512];

	zassert_true(payload_len < sizeof(payload_str), "Payload too large for buffer");
	memcpy(payload_str, payload, payload_len);
	payload_str[payload_len] = '\0';

	/* Verify EDHOC link is present */
	zassert_not_null(strstr(payload_str, "</.well-known/edhoc>"),
			 "Should contain EDHOC link, got: %s", payload_str);

	/* Verify test resource is NOT present (doesn't match rt=core.edhoc) */
	zassert_is_null(strstr(payload_str, "</test>"),
			"Should not contain </test> resource, got: %s", payload_str);
}

/* Test that EDHOC link is not duplicated if resource already exists */
ZTEST(coap, test_well_known_core_edhoc_no_duplicate)
{
	uint8_t request_buf[128];
	uint8_t response_buf[512];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build GET request to /.well-known/core */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token123",
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "core", strlen("core"));
	zassert_equal(r, 0, "Failed to add Uri-Path core");

	/* Create resources including an existing EDHOC resource */
	static const char *edhoc_attrs[] = { "rt=custom.edhoc", NULL };
	static struct coap_core_metadata edhoc_meta = { .attributes = edhoc_attrs };
	struct coap_resource resources[] = {
		{
			.path = (const char * const[]){".well-known", "edhoc", NULL},
			.user_data = &edhoc_meta
		},
		{ .path = NULL }
	};

	r = coap_well_known_core_get_len(resources, 1, &request, &response,
					 response_buf, sizeof(response_buf));
	zassert_equal(r, 0, "coap_well_known_core_get_len failed");

	/* Get the payload */
	const uint8_t *payload;
	uint16_t payload_len;

	payload = coap_packet_get_payload(&response, &payload_len);
	zassert_not_null(payload, "Payload should be present");

	/* Convert payload to string */
	char payload_str[512];

	zassert_true(payload_len < sizeof(payload_str), "Payload too large for buffer");
	memcpy(payload_str, payload, payload_len);
	payload_str[payload_len] = '\0';

	/* Count occurrences of </.well-known/edhoc> - should only appear once */
	const char *search_str = "</.well-known/edhoc>";
	const char *pos = payload_str;
	int count = 0;

	while ((pos = strstr(pos, search_str)) != NULL) {
		count++;
		pos += strlen(search_str);
	}

	zassert_equal(count, 1, "EDHOC link should appear exactly once, got %d times in: %s",
		      count, payload_str);

	/* Should contain the custom attribute from the real resource */
	zassert_not_null(strstr(payload_str, "rt=custom.edhoc"),
			 "Should contain custom attribute, got: %s", payload_str);
}

/* Helper function for EDHOC query filter tests (RFC 9668 Section 6) */
static void test_edhoc_query_filter(const char *query_str, const char *expected_attr)
{
	uint8_t request_buf[128];
	uint8_t response_buf[512];
	struct coap_packet request;
	struct coap_packet response;
	const uint8_t *payload;
	uint16_t payload_len;
	char payload_str[512];
	int r;

	/* Build GET request to /.well-known/core with query */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, (uint8_t *)"token123",
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Failed to init request");

	/* Add Uri-Path options */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      ".well-known", strlen(".well-known"));
	zassert_equal(r, 0, "Failed to add Uri-Path .well-known");

	r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
				      "core", strlen("core"));
	zassert_equal(r, 0, "Failed to add Uri-Path core");

	/* Add Uri-Query option */
	r = coap_packet_append_option(&request, COAP_OPTION_URI_QUERY,
				      query_str, strlen(query_str));
	zassert_equal(r, 0, "Failed to add Uri-Query");

	/* Call with resources that don't match the query */
	struct coap_resource resources[] = {
		{ .path = (const char * const[]){"test", NULL} },
		{ .path = NULL }
	};

	r = coap_well_known_core_get_len(resources, 1, &request, &response,
					 response_buf, sizeof(response_buf));
	zassert_equal(r, 0, "coap_well_known_core_get_len failed");

	/* Get and convert payload to string */
	payload = coap_packet_get_payload(&response, &payload_len);
	zassert_not_null(payload, "Payload should be present");
	zassert_true(payload_len > 0, "Payload should not be empty");
	zassert_true(payload_len < sizeof(payload_str), "Payload too large for buffer");
	memcpy(payload_str, payload, payload_len);
	payload_str[payload_len] = '\0';

	/* Verify EDHOC link is present */
	zassert_not_null(strstr(payload_str, "</.well-known/edhoc>"),
			 "Should contain EDHOC link, got: %s", payload_str);
	zassert_not_null(strstr(payload_str, expected_attr),
			 "Should contain %s attribute, got: %s", expected_attr, payload_str);

	/* Verify test resource is NOT present (doesn't match query) */
	zassert_is_null(strstr(payload_str, "</test>"),
			"Should not contain </test> resource, got: %s", payload_str);
}

/* Test that /.well-known/core?ed-r filters correctly (RFC 9668 Section 6) */
ZTEST(coap, test_well_known_core_edhoc_ed_r_filter)
{
	test_edhoc_query_filter("ed-r", ";ed-r");
}

/* Test that /.well-known/core?ed-r=<value> ignores value (RFC 9668 Section 6) */
ZTEST(coap, test_well_known_core_edhoc_ed_r_value_ignored)
{
	test_edhoc_query_filter("ed-r=1", ";ed-r");
}

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)

/* Test that /.well-known/core?ed-comb-req filters correctly (RFC 9668 Section 6) */
ZTEST(coap, test_well_known_core_edhoc_ed_comb_req_filter)
{
	test_edhoc_query_filter("ed-comb-req", ";ed-comb-req");
}

/* Test that /.well-known/core?ed-comb-req=<value> ignores value (RFC 9668 Section 6) */
ZTEST(coap, test_well_known_core_edhoc_ed_comb_req_value_ignored)
{
	test_edhoc_query_filter("ed-comb-req=1", ";ed-comb-req");
}

#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

#endif /* CONFIG_COAP_SERVER_WELL_KNOWN_EDHOC */
