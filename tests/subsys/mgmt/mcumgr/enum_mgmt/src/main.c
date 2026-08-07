/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/enum_mgmt/enum_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_sample);

#define SHELL_MGMT_HANDLERS 1
#define OS_MGMT_HANDLERS 6
#define ENUM_MGMT_HANDLERS 4

#define OS_MGMT_NAME "os mgmt"
#define ENUM_MGMT_NAME "enum mgmt"
#define SHELL_MGMT_NAME "shell mgmt"

#if defined(CONFIG_MCUMGR_GRP_SHELL) && defined(CONFIG_MCUMGR_GRP_OS)
#define TEST_GROUPS 3
#elif defined(CONFIG_MCUMGR_GRP_SHELL) || defined(CONFIG_MCUMGR_GRP_OS)
#define TEST_GROUPS 2
#else
#define TEST_GROUPS 1
#endif

#if defined(CONFIG_MCUMGR_GRP_SHELL) && defined(CONFIG_MCUMGR_GRP_OS)
#define FOUND_INDEX_SHELL 2
#else
#define FOUND_INDEX_SHELL 1
#endif
#define FOUND_INDEX_OS 1
#define FOUND_INDEX_ENUM 0

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 128
#define OUTPUT_BUFFER_SIZE 512
#define ZCBOR_HISTORY_ARRAY_SIZE 7

static struct net_buf *nb;
static bool enum_valid_got;
static bool enum_field_added;
static bool event_invalid_got;
static bool block_access;
static bool add_field;

struct list_entries {
	uint8_t entries;
	uint16_t groups[16];
};

struct details_entries {
	char expected_name[32];
	uint8_t expected_handlers;
	bool expected_test;

	bool matched_name;
	bool matched_handlers;
	bool matched_test;
};

#if defined(CONFIG_MCUMGR_GRP_SHELL)
#define SINGLE_MATCHED_SHELL 0x1
#else
#define SINGLE_MATCHED_SHELL 0x0
#endif
#if defined(CONFIG_MCUMGR_GRP_OS)
#define SINGLE_MATCHED_OS 0x2
#else
#define SINGLE_MATCHED_OS 0x0
#endif
#define SINGLE_MATCHED_ENUM 0x4

#define SINGLE_MATCHED_ALL (SINGLE_MATCHED_SHELL | SINGLE_MATCHED_OS | SINGLE_MATCHED_ENUM)

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}

	enum_valid_got = false;
	enum_field_added = false;
	event_invalid_got = false;
	block_access = false;
	add_field = false;
}

ZTEST(enum_mgmt, test_count)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	uint32_t count_response = 0;
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("count", zcbor_uint32_decode, &count_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_enum_mgmt_count_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Wait for a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, ENUM_MGMT_ID_COUNT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the value matches */
	zassert_equal(count_response, TEST_GROUPS, "Expected data mismatch");

	/* Clean up test */
	cleanup_test(NULL);
}

static bool parse_list_entries(zcbor_state_t *state, void *user_data)
{
	uint32_t temp = 0;
	uint16_t i = 0;
	struct list_entries *entry_data = (struct list_entries *)user_data;

	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	while (!zcbor_array_at_end(state)) {
		if (!zcbor_uint32_decode(state, &temp)) {
			return false;
		}

		if (i > ARRAY_SIZE(entry_data->groups)) {
			return false;
		}

		entry_data->groups[i] = (uint16_t)temp;

		++i;
	}

	(void)zcbor_list_end_decode(state);

	entry_data->entries = i;

	return true;
}

ZTEST(enum_mgmt, test_list)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct list_entries list_response = { 0 };
	uint8_t i;
	bool found_groups[TEST_GROUPS] = { 0 };

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("groups", parse_list_entries, &list_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_enum_mgmt_list_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Wait for a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, ENUM_MGMT_ID_LIST, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the values match */
	zassert_equal(list_response.entries, TEST_GROUPS, "Expected data mismatch");

	i = 0;

	while (i < list_response.entries) {
		uint8_t index = 0xff;

		if (list_response.groups[i] == MGMT_GROUP_ID_ENUM) {
			index = FOUND_INDEX_ENUM;
		}
#if defined(CONFIG_MCUMGR_GRP_OS)
		if (list_response.groups[i] == MGMT_GROUP_ID_OS) {
			index = FOUND_INDEX_OS;
		}
#endif
#if defined(CONFIG_MCUMGR_GRP_SHELL)
		if (list_response.groups[i] == MGMT_GROUP_ID_SHELL) {
			index = FOUND_INDEX_SHELL;
		}
#endif

		if (index != 0xff) {
			found_groups[index] = true;
		}

		++i;
	}

	i = 0;

	while (i < TEST_GROUPS) {
		zassert_true(found_groups[i], "Expected group to be found in list");
		++i;
	}

	/* Clean up test */
	cleanup_test(NULL);
}

ZTEST(enum_mgmt, test_single)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	uint8_t i;
	uint8_t matched_entries = 0;
	uint32_t received_group = 0;
	bool received_end = false;

	i = 0;
	while (received_end == false) {
		struct zcbor_map_decode_key_val output_decode[] = {
			ZCBOR_MAP_DECODE_KEY_DECODER("group", zcbor_uint32_decode, &received_group),
			ZCBOR_MAP_DECODE_KEY_DECODER("end", zcbor_bool_decode, &received_end),
		};

		memset(buffer, 0, sizeof(buffer));
		memset(buffer_out, 0, sizeof(buffer_out));
		buffer_size = 0;
		memset(zse, 0, sizeof(zse));
		memset(zsd, 0, sizeof(zsd));

		zcbor_new_encode_state(zse, 3, buffer, ARRAY_SIZE(buffer), 0);
		ok = create_enum_mgmt_single_packet(zse, buffer, buffer_out, &buffer_size, i);
		zassert_true(ok, "Expected packet creation to be successful");

		/* Enable dummy SMP backend and ready for usage */
		smp_dummy_enable();
		smp_dummy_clear_state();

		/* Send query command to dummy SMP backend */
		(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
		smp_dummy_add_data();

		/* Wait for a short duration to see if response has been received */
		received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
		zassert_true(received, "Expected to receive data but timed out");

		/* Retrieve response buffer */
		nb = smp_dummy_get_outgoing();
		smp_dummy_disable();

		/* Check response is as expected */
		header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

		zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
		zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
		zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
			      "SMP header group mismatch");
		zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
		zassert_equal(header->nh_id, ENUM_MGMT_ID_SINGLE,
			      "SMP header command ID mismatch");
		zassert_equal(header->nh_version, 1, "SMP header version mismatch");

		/* Get the response value to compare */
		zcbor_new_decode_state(zsd, 7, nb->data, nb->len, 1, NULL, 0);
		ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;
		zassert_true(ok, "Expected decode to be successful");
		zassert_not_equal(decoded, 0,
				  "Expected to receive at least 1 decoded zcbor element");

		if (received_group == MGMT_GROUP_ID_SHELL) {
			matched_entries |= SINGLE_MATCHED_SHELL;
		} else if (received_group == MGMT_GROUP_ID_OS) {
			matched_entries |= SINGLE_MATCHED_OS;
		} else if (received_group == MGMT_GROUP_ID_ENUM) {
			matched_entries |= SINGLE_MATCHED_ENUM;
		} else {
			zassert_true(0, "Received unknown group");
		}

		if (matched_entries == SINGLE_MATCHED_ALL) {
			zassert_true(received_end, "Expected to have received end");
			zassert_equal(decoded, 2, "Expected to receive 2 decoded zcbor elements");
		} else {
			zassert_false(received_end, "Did not expect to receive end");
			zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor elements");
		}

		zassert_true((i <= TEST_GROUPS), "Loop ran too many times");

		/* Clean up test */
		cleanup_test(NULL);

		++i;
	}

	zassert_equal(matched_entries, SINGLE_MATCHED_ALL, "Received entries mismatch");
}

static bool parse_details_entries(zcbor_state_t *state, void *user_data)
{
	uint32_t group = 0;
	struct zcbor_string name = { 0 };
	uint32_t handlers = 0;
	uint32_t test = 0;
	uint16_t i = 0;
	struct details_entries *entry_data = (struct details_entries *)user_data;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("group", zcbor_uint32_decode, &group),
		ZCBOR_MAP_DECODE_KEY_DECODER("name", zcbor_tstr_decode, &name),
		ZCBOR_MAP_DECODE_KEY_DECODER("handlers", zcbor_uint32_decode, &handlers),
		ZCBOR_MAP_DECODE_KEY_DECODER("test", zcbor_uint32_decode, &test),
	};

	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	while (!zcbor_array_at_end(state)) {
		uint8_t index = 0;
		bool ok;
		size_t decoded = 0;

		/* Reset */
		group = 0;
		name.value = NULL;
		name.len = 0;
		handlers = 0;
		test = 0;

		i = 0;

		while (i < ARRAY_SIZE(output_decode)) {
			output_decode[i].found = false;
			++i;
		}

		ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;
		zassert_true(ok, "Expected decode to be successful");

		if (group == MGMT_GROUP_ID_ENUM) {
			index = FOUND_INDEX_ENUM;

			if (strcmp(name.value, ENUM_MGMT_NAME) == 0) {
				entry_data[index].matched_name = true;
			}
		}
#if defined(CONFIG_MCUMGR_GRP_OS)
		else if (group == MGMT_GROUP_ID_OS) {
			index = FOUND_INDEX_OS;

			if (strcmp(name.value, OS_MGMT_NAME) == 0) {
				entry_data[index].matched_name = true;
			}
		}
#endif
#if defined(CONFIG_MCUMGR_GRP_SHELL)
		else if (group == MGMT_GROUP_ID_SHELL) {
			index = FOUND_INDEX_SHELL;
		}
#endif
		else {
			return false;
		}

		if (entry_data[index].expected_test) {
			zassert_equal(decoded, 4, "Expected to receive 4 decoded zcbor element");
		} else {
			zassert_equal(decoded, 3, "Expected to receive 3 decoded zcbor element");
		}

		if (memcmp(name.value, entry_data[index].expected_name, name.len) == 0) {
			entry_data[index].matched_name = true;
		}

		if (handlers == entry_data[index].expected_handlers) {
			entry_data[index].matched_handlers = true;
		}

		if (output_decode[3].found == entry_data[index].expected_test) {
			/* Check value is correct */
			if (entry_data[index].expected_test == false ||
			    (entry_data[index].expected_test == true && test == (group * 3 + 1))) {
				entry_data[index].matched_test = true;
			}
		}
	}

	(void)zcbor_list_end_decode(state);

	return true;
}

ZTEST(enum_mgmt, test_details)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct details_entries details_response[TEST_GROUPS] = { 0 };
	uint8_t i;

#if defined(CONFIG_MCUMGR_GRP_SHELL)
	strcpy(details_response[FOUND_INDEX_SHELL].expected_name, SHELL_MGMT_NAME);
	details_response[FOUND_INDEX_SHELL].expected_handlers = SHELL_MGMT_HANDLERS;
	details_response[FOUND_INDEX_SHELL].expected_test = false;
#endif

#if defined(CONFIG_MCUMGR_GRP_OS)
	strcpy(details_response[FOUND_INDEX_OS].expected_name, OS_MGMT_NAME);
	details_response[FOUND_INDEX_OS].expected_handlers = OS_MGMT_HANDLERS;
	details_response[FOUND_INDEX_OS].expected_test = false;
#endif

	strcpy(details_response[FOUND_INDEX_ENUM].expected_name, ENUM_MGMT_NAME);
	details_response[FOUND_INDEX_ENUM].expected_handlers = ENUM_MGMT_HANDLERS;
	details_response[FOUND_INDEX_ENUM].expected_test = false;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("groups", parse_details_entries, &details_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 3, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_enum_mgmt_details_packet(zse, buffer, buffer_out, &buffer_size, NULL, 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Wait for a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, ENUM_MGMT_ID_DETAILS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 7, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	i = 0;

	while (i < TEST_GROUPS) {
		zassert_true(details_response[i].matched_name,
			     "Expected group name to be found in details");
		zassert_true(details_response[i].matched_handlers,
			     "Expected group handler to be found in details");
		zassert_true(details_response[i].matched_test,
			     "Did not expect group test to be found in details");
		++i;
	}

	zassert_true(enum_valid_got, "Expected callback to have ran");
	zassert_false(enum_field_added, "Did not expect field to be added");
	zassert_false(event_invalid_got, "Did not expect invalid callback to have ran");

	/* Clean up test */
	cleanup_test(NULL);
}

ZTEST(enum_mgmt, test_details_blocked)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	uint32_t rc = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_uint32_decode, &rc),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 3, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_enum_mgmt_details_packet(zse, buffer, buffer_out, &buffer_size, NULL, 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Force notification callback to return an error */
	block_access = true;

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Wait for a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, ENUM_MGMT_ID_DETAILS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 5, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	zassert_true(enum_valid_got, "Expected callback to have ran");
	zassert_false(enum_field_added, "Did not expect field to be added");
	zassert_false(event_invalid_got, "Did not expect invalid callback to have ran");

	/* Clean up test */
	cleanup_test(NULL);
}

ZTEST(enum_mgmt, test_details_extra)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	size_t decoded = 0;
	struct details_entries details_response[TEST_GROUPS] = { 0 };
	uint8_t i;

#if defined(CONFIG_MCUMGR_GRP_SHELL)
	strcpy(details_response[FOUND_INDEX_SHELL].expected_name, SHELL_MGMT_NAME);
	details_response[FOUND_INDEX_SHELL].expected_handlers = SHELL_MGMT_HANDLERS;
	details_response[FOUND_INDEX_SHELL].expected_test = true;
#endif

#if defined(CONFIG_MCUMGR_GRP_OS)
	strcpy(details_response[FOUND_INDEX_OS].expected_name, OS_MGMT_NAME);
	details_response[FOUND_INDEX_OS].expected_handlers = OS_MGMT_HANDLERS;
	details_response[FOUND_INDEX_OS].expected_test = true;
#endif

	strcpy(details_response[FOUND_INDEX_ENUM].expected_name, ENUM_MGMT_NAME);
	details_response[FOUND_INDEX_ENUM].expected_handlers = ENUM_MGMT_HANDLERS;
	details_response[FOUND_INDEX_ENUM].expected_test = true;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("groups", parse_details_entries, &details_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 3, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_enum_mgmt_details_packet(zse, buffer, buffer_out, &buffer_size, NULL, 0);
	zassert_true(ok, "Expected packet creation to be successful");

	add_field = true;

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* Wait for a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_ENUM),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, ENUM_MGMT_ID_DETAILS, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 5, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	i = 0;

	while (i < TEST_GROUPS) {
		zassert_true(details_response[i].matched_name,
			     "Expected group name to be found in details");
		zassert_true(details_response[i].matched_handlers,
			     "Expected group handler to be found in details");
		zassert_true(details_response[i].matched_test,
			     "Expected group test to be found in details");
		++i;
	}

	zassert_true(enum_valid_got, "Expected callback to have ran");
	zassert_true(enum_field_added, "Expected field to be added");
	zassert_false(event_invalid_got, "Did not expect invalid callback to have ran");

	/* Clean up test */
	cleanup_test(NULL);
}

static enum mgmt_cb_return mgmt_event_cmd_callback(uint32_t event, enum mgmt_cb_return prev_status,
						   int32_t *rc, uint16_t *group, bool *abort_more,
						   void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_ENUM_MGMT_DETAILS) {
		struct enum_mgmt_detail_output *enum_data = (struct enum_mgmt_detail_output *)data;

		enum_valid_got = true;

		if (add_field == true) {
			uint32_t temp = enum_data->group->mg_group_id * 3 + 1;
			bool ok;

			ok = zcbor_tstr_put_lit(enum_data->zse, "test") &&
			     zcbor_uint32_encode(enum_data->zse, &temp);

			if (!ok) {
				*rc = MGMT_ERR_EUNKNOWN;
				return MGMT_CB_ERROR_RC;
			}

			enum_field_added = true;
		}

		if (block_access == true) {
			*rc = MGMT_ERR_EPERUSER;
			return MGMT_CB_ERROR_RC;
		}
	} else {
		event_invalid_got = true;
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback mgmt_event_callback = {
	.callback = mgmt_event_cmd_callback,
	.event_id = MGMT_EVT_OP_ENUM_MGMT_DETAILS,
};

static void *setup_test(void)
{
	mgmt_callback_register(&mgmt_event_callback);

	return NULL;
}

ZTEST_SUITE(enum_mgmt, NULL, setup_test, NULL, cleanup_test, NULL);
