/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/settings_mgmt/settings_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <smp_internal.h>
#include "smp_test_util.h"
#include "settings.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 64
#define OUTPUT_BUFFER_SIZE 64
#define ZCBOR_HISTORY_ARRAY_SIZE 4

#define TEST_RESPONSE_OK_DATA_LENGTH sizeof(test_response_ok_data)
#define TEST_RESPONSE_OK_LENGTH (sizeof(struct smp_hdr) + TEST_RESPONSE_OK_DATA_LENGTH)
#define TEST_RESPONSE_ERROR_DATA_LENGTH sizeof(test_response_error_data)
#define TEST_RESPONSE_ERROR_LENGTH (sizeof(struct smp_hdr) + TEST_RESPONSE_ERROR_DATA_LENGTH)

#define TEST_RESPONSE_READ_DATA_LENGTH (sizeof(test_response_read_data_start) + \
					sizeof(test_response_read_data_end))
#define TEST_RESPONSE_READ_LENGTH (sizeof(struct smp_hdr) + TEST_RESPONSE_READ_DATA_LENGTH)

static struct net_buf *nb;
static bool access_read_got;
static bool access_write_got;
static bool access_delete_got;
static bool access_load_got;
static bool access_save_got;
static bool access_commit_got;
static bool access_invalid_got;
static bool event_invalid_got;
static bool block_access;
static char access_name[32];

static const uint8_t val_aa_valid_1[4] = {
	83, 86, 77, 15,
};
static const uint8_t val_aa_valid_2[4] = {
	93, 35, 86, 92,
};
static const uint8_t val_aa_invalid[4] = {
	0, 0, 0, 0
};
static const uint8_t val_bb_valid_1 = 0xab;

static const uint8_t test_response_ok_data[2] = {
	0xbf, 0xff
};

static const uint8_t test_response_error_data[8] = {
	0xbf, 0x62, 0x72, 0x63, 0x19, 0x01, 0x00, 0xff
};

static const uint8_t test_response_read_data_start[5] = {
	0xbf, 0x63, 0x76, 0x61, 0x6c
};

static const uint8_t test_response_read_data_end[1] = {
	0xff
};

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}

	access_read_got = false;
	access_write_got = false;
	access_delete_got = false;
	access_load_got = false;
	access_save_got = false;
	access_commit_got = false;
	access_invalid_got = false;
	event_invalid_got = false;
	block_access = false;
	memset(access_name, 0, sizeof(access_name));

	settings_state_reset();
}

ZTEST(settings_mgmt, test_commit)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	bool set_called;
	bool get_called;
	bool export_called;
	bool commit_called;
	struct smp_hdr *header;

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_settings_mgmt_commit_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_COMMIT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_true(access_commit_got, "Expected commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_true(commit_called, "Expected setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);

	/* Force notification callback to return an error */
	block_access = true;

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_ERROR_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_ERROR_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_COMMIT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_ERROR_LENGTH -
				     TEST_RESPONSE_ERROR_DATA_LENGTH)],
			  test_response_error_data, TEST_RESPONSE_ERROR_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_true(access_commit_got, "Expected commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");
}

ZTEST(settings_mgmt, test_save)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	bool set_called;
	bool get_called;
	bool export_called;
	bool commit_called;
	struct smp_hdr *header;

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_settings_mgmt_save_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_LOAD_SAVE, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_true(access_save_got, "Expected save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_true(export_called, "Expected setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);

	/* Force notification callback to return an error */
	block_access = true;

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_ERROR_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_ERROR_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_LOAD_SAVE, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_ERROR_LENGTH -
				     TEST_RESPONSE_ERROR_DATA_LENGTH)],
			  test_response_error_data, TEST_RESPONSE_ERROR_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_true(access_save_got, "Expected save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");
}

ZTEST(settings_mgmt, test_set_read)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	bool set_called;
	bool get_called;
	bool export_called;
	bool commit_called;
	struct smp_hdr *header;
	struct zcbor_string tmp_data = { 0 };
	struct smp_hdr *smp_header;
	size_t decoded = 0;
	uint8_t loop_number = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("val", zcbor_bstr_decode, &tmp_data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Set "test_val/aa" value */
	ok = create_settings_mgmt_write_packet(zse, buffer, buffer_out, &buffer_size,
					       "test_val/aa", val_aa_valid_1,
					       sizeof(val_aa_valid_1));
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_true(access_write_got, "Expected write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	while (loop_number <= sizeof(val_aa_valid_1)) {
		/* Clean up test */
		cleanup_test(NULL);
		zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));

		zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

		/* Get "test_val/aa" value */
		ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
						      "test_val/aa", (uint32_t)loop_number);
		zassert_true(ok, "Expected packet creation to be successful");

		/* Enable dummy SMP backend and ready for usage */
		smp_dummy_enable();
		smp_dummy_clear_state();

		/* Send query command to dummy SMP backend */
		(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
		smp_dummy_add_data();

		/* For a short duration to see if response has been received */
		received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
		zassert_true(received, "Expected to receive data but timed out");

		/* Retrieve response buffer */
		nb = smp_dummy_get_outgoing();
		smp_dummy_disable();

		/* Check response is as expected */
		zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

		smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

		zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
			      "SMP header length mismatch");
		zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
		zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP,
			      "SMP header operation mismatch");
		zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
			      "SMP header group mismatch");
		zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
		zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
			      "SMP header command ID mismatch");
		zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

		/* Get the response value to compare */
		zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
		ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;
		zassert_true(ok, "Expected decode to be successful");
		zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

		/* Ensure the right amount of data was read and that the value matches */
		zassert_equal(tmp_data.len, (loop_number == 0 ? sizeof(val_aa_valid_1) :
			      loop_number), "Expected data size mismatch");

		zassert_mem_equal(tmp_data.value, val_aa_valid_1, tmp_data.len,
				  "Read data mismatch");

		/* Check events */
		settings_state_get(&set_called, &get_called, &export_called, &commit_called);
		zassert_true(access_read_got, "Expected read access notification");
		zassert_false(access_write_got, "Did not expect write access notification");
		zassert_false(access_delete_got, "Did not expect delete access notification");
		zassert_false(access_load_got, "Did not expect load access notification");
		zassert_false(access_save_got, "Did no expect save access notification");
		zassert_false(access_commit_got, "Did not expect commit access notification");
		zassert_false(access_invalid_got,
			      "Did not expect an invalid access notification type");
		zassert_false(event_invalid_got, "Did not expect an invalid event");

		zassert_false(set_called, "Did not expect setting set function to be called");
		zassert_true(get_called, "Expected setting get function to be called");
		zassert_false(export_called, "Did not expect setting export function to be called");
		zassert_false(commit_called, "Did not expect setting commit function to be called");

		++loop_number;
	}
}

ZTEST(settings_mgmt, test_read_max_size)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	bool set_called;
	bool get_called;
	bool export_called;
	bool commit_called;
	struct zcbor_string tmp_data = { 0 };
	struct smp_hdr *smp_header;
	size_t decoded = 0;
	uint32_t max_size_response = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("val", zcbor_bstr_decode, &tmp_data),
		ZCBOR_MAP_DECODE_KEY_DECODER("max_size", zcbor_uint32_decode, &max_size_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Get "test_val/aa" value */
	ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
					      "test_val/aa", 4019);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 2, "Expected to receive 2 decoded zcbor elements");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_true(access_read_got, "Expected read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_true(get_called, "Expected setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");
	zassert_equal(max_size_response, CONFIG_MCUMGR_GRP_SETTINGS_VALUE_LEN,
		      "Expected to get max_size response");
}

ZTEST(settings_mgmt, test_set_disallowed)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	bool set_called;
	bool get_called;
	bool export_called;
	bool commit_called;
	struct smp_hdr *header;
	struct zcbor_string tmp_data = { 0 };
	struct smp_hdr *smp_header;
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("val", zcbor_bstr_decode, &tmp_data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Set "test_val/bb" value when block is active */
	block_access = true;
	ok = create_settings_mgmt_write_packet(zse, buffer, buffer_out, &buffer_size,
					       "test_val/bb", &val_bb_valid_1,
					       sizeof(val_bb_valid_1));
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_ERROR_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_ERROR_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_ERROR_LENGTH -
				     TEST_RESPONSE_ERROR_DATA_LENGTH)],
			  test_response_error_data, TEST_RESPONSE_ERROR_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_true(access_write_got, "Expected write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Get "test_val/bb" value */
	ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
					      "test_val/bb", 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the value does not match */
	zassert_equal(tmp_data.len, sizeof(val_bb_valid_1), "Expected data size mismatch");
	zassert_not_equal(tmp_data.value[0], val_bb_valid_1, "Read data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_true(access_read_got, "Expected read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_true(get_called, "Expected setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Set "test_val/bb" value when block is not active */
	block_access = false;
	ok = create_settings_mgmt_write_packet(zse, buffer, buffer_out, &buffer_size,
					       "test_val/bb", &val_bb_valid_1,
					       sizeof(val_bb_valid_1));
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_true(access_write_got, "Expected write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Get "test_val/bb" value */
	ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
					      "test_val/bb", 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the value matches */
	zassert_equal(tmp_data.len, sizeof(val_bb_valid_1), "Expected data size mismatch");
	zassert_equal(tmp_data.value[0], val_bb_valid_1, "Read data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_true(access_read_got, "Expected read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_true(get_called, "Expected setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");
}

ZTEST(settings_mgmt, test_delete)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	bool set_called;
	bool get_called;
	bool export_called;
	bool commit_called;
	struct smp_hdr *header;
	struct zcbor_string tmp_data = { 0 };
	struct smp_hdr *smp_header;
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("val", zcbor_bstr_decode, &tmp_data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Set "test_val/aa" value */
	ok = create_settings_mgmt_write_packet(zse, buffer, buffer_out, &buffer_size,
					       "test_val/aa", val_aa_valid_1,
					       sizeof(val_aa_valid_1));
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_true(access_write_got, "Expected write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Save data to persistent storage */
	ok = create_settings_mgmt_save_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(smp_header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_LOAD_SAVE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_true(access_save_got, "Expected save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_true(export_called, "Expected setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Set "test_val/aa" value to other valid */
	ok = create_settings_mgmt_write_packet(zse, buffer, buffer_out, &buffer_size,
					       "test_val/aa", val_aa_valid_2,
					       sizeof(val_aa_valid_2));
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_true(access_write_got, "Expected write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Get "test_val/aa" value */
	ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
					      "test_val/aa", 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the value matches */
	zassert_equal(tmp_data.len, sizeof(val_aa_valid_2), "Expected data size mismatch");
	zassert_mem_equal(tmp_data.value, val_aa_valid_2, tmp_data.len, "Read data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_true(access_read_got, "Expected read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_true(get_called, "Expected setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Load data from persistent storage */
	ok = create_settings_mgmt_load_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(smp_header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_LOAD_SAVE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_true(access_load_got, "Expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expext setting export function to be called");
	zassert_true(commit_called, "Expected setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Get "test_val/aa" value */
	ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
					      "test_val/aa", 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the value matches */
	zassert_equal(tmp_data.len, sizeof(val_aa_valid_1), "Expected data size mismatch");
	zassert_mem_equal(tmp_data.value, val_aa_valid_1, tmp_data.len, "Read data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_true(access_read_got, "Expected read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_true(get_called, "Expected setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Delete "test_val/aa" value */
	ok = create_settings_mgmt_delete_packet(zse, buffer, buffer_out, &buffer_size,
						"test_val/aa");
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(smp_header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_DELETE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_true(access_delete_got, "Expected delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Set "test_val/aa" value to other value */
	ok = create_settings_mgmt_write_packet(zse, buffer, buffer_out, &buffer_size,
					       "test_val/aa", val_aa_invalid,
					       sizeof(val_aa_invalid));
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");
	header = (struct smp_hdr *)nb->data;
	zassert_equal(header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");
	zassert_mem_equal(&nb->data[(TEST_RESPONSE_OK_LENGTH - TEST_RESPONSE_OK_DATA_LENGTH)],
			  test_response_ok_data, TEST_RESPONSE_OK_DATA_LENGTH,
			  "SMP data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_true(access_write_got, "Expected write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Load data from persistent storage */
	ok = create_settings_mgmt_load_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_equal(nb->len, TEST_RESPONSE_OK_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(smp_header->nh_len, sys_cpu_to_be16(TEST_RESPONSE_OK_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_LOAD_SAVE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_false(access_read_got, "Did not expect read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_true(access_load_got, "Expected load access notification");
	zassert_false(access_save_got, "Did not expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_true(set_called, "Expected setting set function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(get_called, "Did not expect setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_true(commit_called, "Expected setting commit function to be called");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Get "test_val/aa" value */
	ok = create_settings_mgmt_read_packet(zse, buffer, buffer_out, &buffer_size,
					      "test_val/aa", 0);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);
	zassert_true(received, "Expected to receive data but timed out");

	/* Retrieve response buffer */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Check response is as expected */
	zassert_true(nb->len > TEST_RESPONSE_READ_LENGTH, "SMP response mismatch");

	smp_header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_true(smp_header->nh_len > sys_cpu_to_be16(TEST_RESPONSE_READ_DATA_LENGTH),
		      "SMP header length mismatch");
	zassert_equal(smp_header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(smp_header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(smp_header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_SETTINGS),
		      "SMP header group mismatch");
	zassert_equal(smp_header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(smp_header->nh_id, SETTINGS_MGMT_ID_READ_WRITE,
		      "SMP header command ID mismatch");
	zassert_equal(smp_header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the value matches */
	zassert_equal(tmp_data.len, sizeof(val_aa_invalid), "Expected data size mismatch");
	zassert_mem_equal(tmp_data.value, val_aa_invalid, tmp_data.len, "Read data mismatch");

	/* Check events */
	settings_state_get(&set_called, &get_called, &export_called, &commit_called);
	zassert_true(access_read_got, "Expected read access notification");
	zassert_false(access_write_got, "Did not expect write access notification");
	zassert_false(access_delete_got, "Did not expect delete access notification");
	zassert_false(access_load_got, "Did not expect load access notification");
	zassert_false(access_save_got, "Did no expect save access notification");
	zassert_false(access_commit_got, "Did not expect commit access notification");
	zassert_false(access_invalid_got, "Did not expect an invalid access notification type");
	zassert_false(event_invalid_got, "Did not expect an invalid event");

	zassert_false(set_called, "Did not expect setting set function to be called");
	zassert_true(get_called, "Expected setting get function to be called");
	zassert_false(export_called, "Did not expect setting export function to be called");
	zassert_false(commit_called, "Did not expect setting commit function to be called");
}

static enum mgmt_cb_return mgmt_event_cmd_callback(uint32_t event, enum mgmt_cb_return prev_status,
						   int32_t *rc, uint16_t *group, bool *abort_more,
						   void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_SETTINGS_MGMT_ACCESS) {
		struct settings_mgmt_access *settings_data = (struct settings_mgmt_access *)data;

		if (settings_data->access == SETTINGS_ACCESS_READ) {
			access_read_got = true;
		} else if (settings_data->access == SETTINGS_ACCESS_WRITE) {
			access_write_got = true;
		} else if (settings_data->access == SETTINGS_ACCESS_DELETE) {
			access_delete_got = true;
		} else if (settings_data->access == SETTINGS_ACCESS_LOAD) {
			access_load_got = true;
		} else if (settings_data->access == SETTINGS_ACCESS_SAVE) {
			access_save_got = true;
		} else if (settings_data->access == SETTINGS_ACCESS_COMMIT) {
			access_commit_got = true;
		} else {
			access_invalid_got = true;
		}

		if ((settings_data->access == SETTINGS_ACCESS_READ ||
		     settings_data->access == SETTINGS_ACCESS_WRITE ||
		     settings_data->access == SETTINGS_ACCESS_DELETE) &&
		    settings_data->name != NULL) {
			strncpy(access_name, settings_data->name, (sizeof(access_name) - 1));
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
	.event_id = MGMT_EVT_OP_SETTINGS_MGMT_ACCESS,
};

static void *setup_test(void)
{
	int rc;

	mgmt_callback_register(&mgmt_event_callback);
	rc = settings_subsys_init();

	zassert_ok(rc, "Expected settings to init");

	return NULL;
}

ZTEST_SUITE(settings_mgmt, NULL, setup_test, NULL, cleanup_test, NULL);
