/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 128
#define OUTPUT_BUFFER_SIZE 512
#define ZCBOR_HISTORY_ARRAY_SIZE 10

static struct net_buf *nb;
static bool slot_info_image_callback_got;
static bool slot_info_slot_callback_got;
static bool other_callback_got;
static bool block_access;
static bool add_field;

struct partition_entries_t {
	uint8_t partition_id;
	uint32_t size;
};

struct partition_entries_t partition_entries[] = {
	{ FIXED_PARTITION_ID(slot0_partition), 0 },
	{ FIXED_PARTITION_ID(slot1_partition), 0 },
#if FIXED_PARTITION_EXISTS(slot2_partition)
	{ FIXED_PARTITION_ID(slot2_partition), 0 },
#endif
#if FIXED_PARTITION_EXISTS(slot3_partition)
	{ FIXED_PARTITION_ID(slot3_partition), 0 },
#endif
};

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}

	slot_info_image_callback_got = false;
	slot_info_slot_callback_got = false;
	other_callback_got = false;
	block_access = false;
	add_field = false;
}

struct slot_info_t {
	uint32_t slot;
	bool slot_received;
	uint32_t size;
	bool size_received;
	uint32_t upload_image_id;
	bool upload_image_id_received;
	uint32_t test2;
	bool test2_received;
};

struct image_info_t {
	uint32_t image;
	bool image_received;
	struct slot_info_t slots[2];
	uint32_t max_image_size;
	bool max_image_size_received;
	uint32_t test1;
	bool test1_received;

	uint8_t current_slot;
};

struct receive_info_t {
	struct image_info_t images[CONFIG_UPDATEABLE_IMAGE_NUMBER];

	uint8_t current_image;
};

static bool parse_slot_entries(zcbor_state_t *state, void *user_data)
{
	size_t decoded = 0;
	struct image_info_t *image_data = (struct image_info_t *)user_data;
	bool ok;

	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	while (!zcbor_array_at_end(state)) {
		struct slot_info_t *slot_data = &image_data->slots[image_data->current_slot];
		struct zcbor_map_decode_key_val output_decode[] = {
			ZCBOR_MAP_DECODE_KEY_DECODER("slot", zcbor_uint32_decode,
						     &slot_data->slot),
			ZCBOR_MAP_DECODE_KEY_DECODER("size", zcbor_uint32_decode,
						     &slot_data->size),
			ZCBOR_MAP_DECODE_KEY_DECODER("upload_image_id", zcbor_uint32_decode,
						     &slot_data->upload_image_id),
			ZCBOR_MAP_DECODE_KEY_DECODER("test2", zcbor_uint32_decode,
						     &slot_data->test2),
		};


		ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;
		zassert_true(ok, "Expected decode to be successful");
		zassert_true((decoded == 2 || decoded == 3 || decoded == 4),
			     "Expected to receive 2-4 decoded zcbor elements");

		slot_data->slot_received = zcbor_map_decode_bulk_key_found(output_decode,
									ARRAY_SIZE(output_decode),
									"slot");
		slot_data->size_received = zcbor_map_decode_bulk_key_found(output_decode,
									ARRAY_SIZE(output_decode),
									"slot");
		slot_data->upload_image_id_received = zcbor_map_decode_bulk_key_found(
									output_decode,
									ARRAY_SIZE(output_decode),
									"upload_image_id");
		slot_data->test2_received = zcbor_map_decode_bulk_key_found(output_decode,
									ARRAY_SIZE(output_decode),
									"test2");

		++image_data->current_slot;
	}

	(void)zcbor_list_end_decode(state);

	return true;
}

static bool parse_images_entries(zcbor_state_t *state, void *user_data)
{
	struct receive_info_t *receive_data = (struct receive_info_t *)user_data;
	bool ok;

	if (!zcbor_list_start_decode(state)) {
		return false;
	}

	while (!zcbor_array_at_end(state)) {
		size_t decoded = 0;
		struct image_info_t *image_data =
					&receive_data->images[receive_data->current_image];

		struct zcbor_map_decode_key_val output_decode[] = {
			ZCBOR_MAP_DECODE_KEY_DECODER("image", zcbor_uint32_decode,
						     &image_data->image),
			ZCBOR_MAP_DECODE_KEY_DECODER("slots", parse_slot_entries, image_data),
			ZCBOR_MAP_DECODE_KEY_DECODER("max_image_size", zcbor_uint32_decode,
						     &image_data->max_image_size),
			ZCBOR_MAP_DECODE_KEY_DECODER("test1", zcbor_uint32_decode,
						     &image_data->test1),
		};

		ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;
		zassert_true(ok, "Expected decode to be successful");
		zassert_true((decoded == 2 || decoded == 3 || decoded == 4),
			     "Expected to receive 2-4 decoded zcbor elements");

		image_data->image_received = zcbor_map_decode_bulk_key_found(output_decode,
								ARRAY_SIZE(output_decode),
								"image");
		image_data->max_image_size_received = zcbor_map_decode_bulk_key_found(
								output_decode,
								ARRAY_SIZE(output_decode),
								"max_image_size");
		image_data->test1_received = zcbor_map_decode_bulk_key_found(output_decode,
								ARRAY_SIZE(output_decode),
								"test1");

		++receive_data->current_image;
	}

	(void)zcbor_list_end_decode(state);

	return true;
}

ZTEST(img_mgmt, test_list)
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
	struct receive_info_t receive_response = { 0 };
	uint8_t i = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("images", parse_images_entries, &receive_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_img_mgmt_slot_info_packet(zse, buffer, buffer_out, &buffer_size);
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
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_IMAGE),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, IMG_MGMT_ID_SLOT_INFO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 8, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the values match */
	zassert_equal(receive_response.current_image, CONFIG_UPDATEABLE_IMAGE_NUMBER,
		      "Expected data mismatch");

	while (i < receive_response.current_image) {
		struct image_info_t *current_image =
				&receive_response.images[i];
		uint8_t l = 0;
#ifdef CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD
		uint32_t expected_max_size;
#endif

		zassert_equal(current_image->current_slot, 2, "Expected data mismatch");
		zassert_true(current_image->image_received, "Expected data mismatch");

#ifdef CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD
		expected_max_size = MAX(partition_entries[(i * 2)].size,
					partition_entries[((i * 2) + 1)].size);
		expected_max_size -= CONFIG_MCUBOOT_UPDATE_FOOTER_SIZE;

		zassert_true(current_image->max_image_size_received, "Expected data mismatch");
		zassert_equal(current_image->max_image_size, expected_max_size,
			      "Expected data mismatch");
#else
		zassert_false(current_image->max_image_size_received, "Expected data mismatch");
#endif

		zassert_false(current_image->test1_received, "Expected data mismatch");

		while (l < current_image->current_slot) {
			struct slot_info_t *current_slot =
					&current_image->slots[l];

			zassert_true(current_slot->slot_received, "Expected data mismatch");
			zassert_true(current_slot->size_received, "Expected data mismatch");
			zassert_false(current_slot->test2_received, "Expected data mismatch");

			if (l == 1) {
				zassert_true(current_slot->upload_image_id_received,
					     "Expected data mismatch");
				zassert_equal(current_slot->upload_image_id, i,
					      "Expected data mismatch");
			} else {
				zassert_false(current_slot->upload_image_id_received,
					      "Expected data mismatch");
			}

			zassert_equal(current_slot->slot, l, "Expected data mismatch");
			zassert_equal(current_slot->size, partition_entries[((i * 2) + l)].size,
				      "Expected data mismatch");

			++l;
		}

		++i;
	}

	zassert_false(slot_info_image_callback_got, "Did not expect to get image callback");
	zassert_false(slot_info_slot_callback_got, "Did not expect to get slot callback");
	zassert_false(other_callback_got, "Did not expect to get other callback");

	/* Clean up test */
	cleanup_test(NULL);
}

ZTEST(img_mgmt, test_blocked)
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

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_img_mgmt_slot_info_packet(zse, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful");

	block_access = true;

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
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_IMAGE),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, IMG_MGMT_ID_SLOT_INFO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 8, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	zassert_true(slot_info_slot_callback_got, "Expected callback to have ran");
	zassert_false(slot_info_image_callback_got, "Did not expect other callback to have ran");
	zassert_false(other_callback_got, "Did not expect invalid callback to have ran");
	zassert_equal(rc, MGMT_ERR_EPERUSER, "Expected error was not returned");

	/* Clean up test */
	cleanup_test(NULL);
}

ZTEST(img_mgmt, test_callback)
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
	struct receive_info_t receive_response = { 0 };
	uint8_t i = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("images", parse_images_entries, &receive_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_img_mgmt_slot_info_packet(zse, buffer, buffer_out, &buffer_size);
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
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_IMAGE),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, IMG_MGMT_ID_SLOT_INFO, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 8, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Ensure the right amount of data was read and that the values match */
	zassert_equal(receive_response.current_image, CONFIG_UPDATEABLE_IMAGE_NUMBER,
		      "Expected data mismatch");

	while (i < receive_response.current_image) {
		struct image_info_t *current_image =
				&receive_response.images[i];
		uint8_t l = 0;
#ifdef CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD
		uint32_t expected_max_size;
#endif

		zassert_equal(current_image->current_slot, 2, "Expected data mismatch");
		zassert_true(current_image->image_received, "Expected data mismatch");

#ifdef CONFIG_MCUMGR_GRP_IMG_TOO_LARGE_SYSBUILD
		expected_max_size = MAX(partition_entries[(i * 2)].size,
					partition_entries[((i * 2) + 1)].size);
		expected_max_size -= CONFIG_MCUBOOT_UPDATE_FOOTER_SIZE;

		zassert_true(current_image->max_image_size_received, "Expected data mismatch");
		zassert_equal(current_image->max_image_size, expected_max_size,
			      "Expected data mismatch");
#else
		zassert_false(current_image->max_image_size_received, "Expected data mismatch");
#endif

		zassert_true(current_image->test1_received, "Expected data mismatch");
		zassert_equal(current_image->test1, (i + 18), "Expected data mismatch");

		while (l < current_image->current_slot) {
			struct slot_info_t *current_slot =
					&current_image->slots[l];

			zassert_true(current_slot->test2_received, "Expected data mismatch");
			zassert_true(current_slot->slot_received, "Expected data mismatch");
			zassert_true(current_slot->size_received, "Expected data mismatch");

			if (l == 1) {
				zassert_true(current_slot->upload_image_id_received,
					     "Expected data mismatch");
				zassert_equal(current_slot->upload_image_id, i,
					      "Expected data mismatch");
			} else {
				zassert_false(current_slot->upload_image_id_received,
					      "Expected data mismatch");
			}

			zassert_equal(current_slot->slot, l, "Expected data mismatch");
			zassert_equal(current_slot->size, partition_entries[((i * 2) + l)].size,
				      "Expected data mismatch");
			zassert_equal(current_slot->test2,
				      ((current_image->image * 2) + current_slot->slot + 3),
				      "Expected data mismatch");

			++l;
		}

		++i;
	}

	zassert_true(slot_info_image_callback_got, "Expected to get image callback");
	zassert_true(slot_info_slot_callback_got, "Expected to get slot callback");
	zassert_false(other_callback_got, "Did not expect to get other callback");

	/* Clean up test */
	cleanup_test(NULL);
}

static enum mgmt_cb_return mgmt_event_cmd_callback(uint32_t event, enum mgmt_cb_return prev_status,
						   int32_t *rc, uint16_t *group, bool *abort_more,
						   void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_IMAGE) {
		if (add_field) {
			struct img_mgmt_slot_info_image *img_data =
							(struct img_mgmt_slot_info_image *)data;
			uint32_t temp = (img_data->image + 18);
			bool ok;

			slot_info_image_callback_got = true;

			ok = zcbor_tstr_put_lit(img_data->zse, "test1") &&
			     zcbor_uint32_encode(img_data->zse, &temp);

			if (!ok) {
				*rc = MGMT_ERR_EUNKNOWN;
				return MGMT_CB_ERROR_RC;
			}
		}
	} else if (event == MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_SLOT) {
		if (block_access) {
			slot_info_slot_callback_got = true;
			*rc = MGMT_ERR_EPERUSER;
			return MGMT_CB_ERROR_RC;
		} else if (add_field) {
			struct img_mgmt_slot_info_slot *slot_data =
							(struct img_mgmt_slot_info_slot *)data;
			uint32_t temp = ((slot_data->image * 2) + slot_data->slot + 3);
			bool ok;

			slot_info_slot_callback_got = true;

			ok = zcbor_tstr_put_lit(slot_data->zse, "test2") &&
			     zcbor_uint32_encode(slot_data->zse, &temp);

			if (!ok) {
				*rc = MGMT_ERR_EUNKNOWN;
				return MGMT_CB_ERROR_RC;
			}
		}
	} else {
		other_callback_got = true;
	}

	return MGMT_CB_OK;
}

static struct mgmt_callback mgmt_event_callback = {
	.callback = mgmt_event_cmd_callback,
	.event_id = (MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_IMAGE | MGMT_EVT_OP_IMG_MGMT_SLOT_INFO_SLOT),
};

static void *setup_test(void)
{
	int rc;
	uint8_t i = 0;

	while (i < ARRAY_SIZE(partition_entries)) {
		const struct flash_area *fa;

		rc = flash_area_open(partition_entries[i].partition_id, &fa);
		zassert_ok(rc, "Expected data mismatch");
		flash_area_close(fa);

		partition_entries[i].size = fa->fa_size;

		++i;
	}

	mgmt_callback_register(&mgmt_event_callback);

	return NULL;
}

ZTEST_SUITE(img_mgmt, NULL, setup_test, NULL, cleanup_test, NULL);
