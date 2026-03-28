/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2025, Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/sys/byteorder.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define ZCBOR_BUFFER_SIZE 128
#define OUTPUT_BUFFER_SIZE 384
#define ZCBOR_HISTORY_ARRAY_SIZE 8
#define TEST_MAX_HEAPS 2

struct heap_info_t {
	uint32_t block_size;
	uint32_t total_blocks;
	uint32_t free_blocks;
	uint32_t minimum_blocks;
};

struct memory_pools_info_t {
	struct heap_info_t heaps[TEST_MAX_HEAPS];
	uint8_t current_heap;
};

static struct net_buf *nb;
static void *malloc_test_object;
static void *k_malloc_test_object;

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_reset(nb);
		net_buf_unref(nb);
		nb = NULL;
	}

	if (malloc_test_object != NULL) {
		free(malloc_test_object);
		malloc_test_object = NULL;
	}

	if (k_malloc_test_object != NULL) {
		k_free(k_malloc_test_object);
		k_malloc_test_object = NULL;
	}
}

static bool parse_heap_entries(zcbor_state_t *state, void *user_data)
{
	struct memory_pools_info_t *receive_data = (struct memory_pools_info_t *)user_data;
	bool ok;

	receive_data->current_heap = 0;

	if (!zcbor_map_start_decode(state)) {
		return false;
	}

	while (!zcbor_array_at_end(state)) {
		size_t decoded = 0;
		struct heap_info_t *heap_data = &receive_data->heaps[receive_data->current_heap];
		struct zcbor_string block_name;
		uint8_t expected_block_name[8];
		uint8_t expected_block_name_size;

		zassert_not_equal(receive_data->current_heap, TEST_MAX_HEAPS,
				  "More heaps than were expected");

		struct zcbor_map_decode_key_val output_decode[] = {
			ZCBOR_MAP_DECODE_KEY_DECODER("blksiz", zcbor_uint32_decode,
						     &heap_data->block_size),
			ZCBOR_MAP_DECODE_KEY_DECODER("nblks", zcbor_uint32_decode,
						     &heap_data->total_blocks),
			ZCBOR_MAP_DECODE_KEY_DECODER("nfree", zcbor_uint32_decode,
						     &heap_data->free_blocks),
			ZCBOR_MAP_DECODE_KEY_DECODER("min", zcbor_uint32_decode,
						     &heap_data->minimum_blocks),
		};

		ok = zcbor_tstr_decode(state, &block_name);
		zassert_true(ok, "Expected to get name of memory block");

		expected_block_name_size = u8_to_dec(expected_block_name,
						     sizeof(expected_block_name),
						     receive_data->current_heap);
		zassert_equal(expected_block_name_size, block_name.len,
			      "Expected memory block name size to match expected value size");
		zassert_mem_equal(expected_block_name, block_name.value, block_name.len,
				  "Expected memory block name to match expected value");

		ok = zcbor_map_decode_bulk(state, output_decode, ARRAY_SIZE(output_decode),
					   &decoded) == 0;
		zassert_true(ok, "Expected decode to be successful");

#ifdef CONFIG_MCUMGR_GRP_OS_MPSTAT_ONLY_SUPPORTED_STATS
		zassert_true((decoded == 3),
			     "Expected to receive 3 decoded zcbor elements");
		zassert_false(zcbor_map_decode_bulk_key_found(output_decode,
							     ARRAY_SIZE(output_decode), "blksiz"),
			     "Did not expect to find blksize value");
#else
		zassert_true((decoded == 4),
			     "Expected to receive 4 decoded zcbor elements");
		zassert_equal(heap_data->block_size, 1,
			      "Expected memory block size to match expected value");
#endif

		zassert_true(zcbor_map_decode_bulk_key_found(output_decode,
							     ARRAY_SIZE(output_decode), "nblks"),
			     "Expected to find nblks value");
		zassert_true(zcbor_map_decode_bulk_key_found(output_decode,
							     ARRAY_SIZE(output_decode), "nfree"),
			     "Expected to find nfree value");
		zassert_true(zcbor_map_decode_bulk_key_found(output_decode,
							     ARRAY_SIZE(output_decode), "min"),
			     "Expected to find min value");

		++receive_data->current_heap;
	}

	(void)zcbor_map_end_decode(state);

	zassert_equal(receive_data->current_heap, TEST_MAX_HEAPS, "Less heaps than were expected");

	return true;
}

ZTEST(os_mgmt_mpstat, test_read)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct smp_hdr *header;
	struct memory_pools_info_t receive_response = { 0 };
	size_t decoded = 0;
	uint8_t i = 0;
	uint8_t common_malloc_index = 255;
	uint8_t kernel_malloc_index = 255;
	uint32_t common_malloc_normal_size = 0;
	uint32_t kernel_malloc_normal_size = 0;
	uint32_t common_malloc_diff_size = 0;
	uint32_t kernel_malloc_diff_size = 0;
	bool found_common_malloc_area = false;
	bool found_kernel_malloc_area = false;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("tasks", parse_heap_entries, &receive_response),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Get the unused default memory pool values as a baseline */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_mpstat_packet(zse, buffer, buffer_out, &buffer_size);
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
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_MPSTAT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	const uint32_t libc_malloc_min = (9 * CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE) / 10;
	const uint32_t libc_malloc_max = (11 * CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE) / 10;
	const uint32_t kernel_malloc_min = (9 * CONFIG_HEAP_MEM_POOL_SIZE) / 10;
	const uint32_t kernel_malloc_max = (11 * CONFIG_HEAP_MEM_POOL_SIZE) / 10;

	while (i < receive_response.current_heap) {
		struct heap_info_t *current_heap =
				&receive_response.heaps[i];
		/* Block size only set when CONFIG_MCUMGR_GRP_OS_MPSTAT_ONLY_SUPPORTED_STATS
		 * is disabled.
		 */
		uint32_t block_size = current_heap->block_size ? current_heap->block_size : 1;
		uint32_t heap_size = current_heap->total_blocks * block_size;

		if ((libc_malloc_min <= heap_size) && (heap_size <= libc_malloc_max)) {
			zassert_false(found_common_malloc_area,
				      "Already found common malloc heap area");
			found_common_malloc_area = true;
			common_malloc_normal_size = current_heap->free_blocks;
			common_malloc_index = i;
		} else if ((kernel_malloc_min <= heap_size) && (heap_size <= kernel_malloc_max)) {
			zassert_false(found_kernel_malloc_area,
				      "Already found kernel malloc heap area");
			found_kernel_malloc_area = true;
			kernel_malloc_normal_size = current_heap->free_blocks;
			kernel_malloc_index = i;
		} else {
			zassert_true(false, "Cannot determine heap owner");
		}

		++i;
	}

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 2: Malloc in the libc common area and ensure only that memory pool changes */
	malloc_test_object = malloc(32);
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_mpstat_packet(zse, buffer, buffer_out, &buffer_size);
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
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_MPSTAT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Check that the common libc heap size has reduced and that the kernel heap size is
	 * unchanged
	 */
	zassert_true(receive_response.heaps[common_malloc_index].free_blocks <
		     common_malloc_normal_size,
		     "Expected non-kernel memory heap free block reduction");
	zassert_true(receive_response.heaps[kernel_malloc_index].free_blocks ==
		     kernel_malloc_normal_size,
		     "Did not expect kernel memory heap free block reduction");
	common_malloc_diff_size = receive_response.heaps[common_malloc_index].total_blocks -
				  receive_response.heaps[common_malloc_index].free_blocks;

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 3: Malloc in the kernel area and ensure only that memory pool changes */
	k_malloc_test_object = k_malloc(4);
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_mpstat_packet(zse, buffer, buffer_out, &buffer_size);
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
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_MPSTAT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Check that the kernel heap size has reduced and that the common libc heap size is
	 * unchanged
	 */
	zassert_true(receive_response.heaps[common_malloc_index].free_blocks ==
		     common_malloc_normal_size,
		     "Did not expect non-kernel memory heap free block reduction");
	zassert_true(receive_response.heaps[kernel_malloc_index].free_blocks <
		     kernel_malloc_normal_size,
		     "Expected kernel memory heap free block reduction");
	kernel_malloc_diff_size = receive_response.heaps[kernel_malloc_index].total_blocks -
				  receive_response.heaps[kernel_malloc_index].free_blocks;

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;
	cleanup_test(NULL);

	/* Test 4: Check after both mallocs and frees that the values match the default */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_mpstat_packet(zse, buffer, buffer_out, &buffer_size);
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
	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));

	zassert_equal(header->nh_flags, 0, "SMP header flags mismatch");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP, "SMP header operation mismatch");
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "SMP header group mismatch");
	zassert_equal(header->nh_seq, 1, "SMP header sequence number mismatch");
	zassert_equal(header->nh_id, OS_MGMT_ID_MPSTAT, "SMP header command ID mismatch");
	zassert_equal(header->nh_version, 1, "SMP header version mismatch");

	/* Get the response value to compare */
	zcbor_new_decode_state(zsd, 6, nb->data, nb->len, 1, NULL, 0);
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;
	zassert_true(ok, "Expected decode to be successful");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element");

	/* Check that both heap sizes are unchanged from the original values */
	zassert_true(receive_response.heaps[common_malloc_index].free_blocks ==
		     common_malloc_normal_size,
		     "Did not expect non-kernel memory heap free block reduction");
	zassert_true(receive_response.heaps[kernel_malloc_index].free_blocks ==
		     kernel_malloc_normal_size,
		     "Did not expect kernel memory heap free block reduction");

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	output_decode[0].found = false;

	/* Ensure that a smaller malloc used less free blocks */
	zassert_true(kernel_malloc_diff_size < common_malloc_diff_size,
		     "Expected small kernel malloc to be smaller than larger libc malloc");
}

ZTEST_SUITE(os_mgmt_mpstat, NULL, NULL, NULL, cleanup_test, NULL);
