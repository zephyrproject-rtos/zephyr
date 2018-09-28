/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <logging/log_msg.h>

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>

extern struct k_mem_slab log_msg_pool;
static const char my_string[] = "test_string";
void test_log_std_msg(void)
{
	zassert_true(LOG_MSG_NARGS_SINGLE_CHUNK == 3,
		     "test assumes following setting");

	u32_t used_slabs = k_mem_slab_num_used_get(&log_msg_pool);
	u32_t args[] = {1, 2, 3, 4, 5, 6};
	struct log_msg *msg;

	/* allocation of 0 argument fits in single buffer */
	msg = log_msg_create_0(my_string);

	zassert_equal((used_slabs + 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs++;

	log_msg_put(msg);

	zassert_equal((used_slabs - 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs--;

	/* allocation of 1 argument fits in single buffer */
	msg = log_msg_create_1(my_string, 1);
	zassert_equal((used_slabs + 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs++;

	log_msg_put(msg);

	zassert_equal((used_slabs - 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs--;

	/* allocation of 2 argument fits in single buffer */
	msg = log_msg_create_2(my_string, 1, 2);
	zassert_equal((used_slabs + 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs++;

	log_msg_put(msg);

	zassert_equal((used_slabs - 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs--;

	/* allocation of 3 argument fits in single buffer */
	msg = log_msg_create_3(my_string, 1, 2, 3);

	zassert_equal((used_slabs + 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs++;

	log_msg_put(msg);

	zassert_equal((used_slabs - 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs--;

	/* allocation of 4 argument fits in 2 buffers */
	msg = log_msg_create_n(my_string, args, 4);

	zassert_equal((used_slabs + 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 2;

	log_msg_put(msg);

	zassert_equal((used_slabs - 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 2;

	/* allocation of 5 argument fits in 2 buffers */
	msg = log_msg_create_n(my_string, args, 5);

	zassert_equal((used_slabs + 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 2;

	log_msg_put(msg);

	zassert_equal((used_slabs - 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 2;

	/* allocation of 5 argument fits in 2 buffers */
	msg = log_msg_create_n(my_string, args, 6);

	zassert_equal((used_slabs + 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 2;

	log_msg_put(msg);

	zassert_equal((used_slabs - 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 2;
}

void test_log_hexdump_msg(void)
{

	u32_t used_slabs = k_mem_slab_num_used_get(&log_msg_pool);
	struct log_msg *msg;
	u8_t data[128];

	for (int i = 0; i < sizeof(data); i++) {
		data[i] = i;
	}

	/* allocation of buffer that fits in single buffer */
	msg = log_msg_hexdump_create("test", data,
				     LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK - 4);

	zassert_equal((used_slabs + 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs++;

	log_msg_put(msg);

	zassert_equal((used_slabs - 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs--;

	/* allocation of buffer that fits in single buffer */
	msg = log_msg_hexdump_create("test", data,
				     LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK);

	zassert_equal((used_slabs + 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs++;

	log_msg_put(msg);

	zassert_equal((used_slabs - 1),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs--;

	/* allocation of buffer that fits in 2 buffers */
	msg = log_msg_hexdump_create("test", data,
				     LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK + 1);

	zassert_equal((used_slabs + 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 2;

	log_msg_put(msg);

	zassert_equal((used_slabs - 2),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 2;

	/* allocation of buffer that fits in 3 buffers */
	msg = log_msg_hexdump_create("test", data,
				     LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK +
				     HEXDUMP_BYTES_CONT_MSG + 1);

	zassert_equal((used_slabs + 3),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 3;

	log_msg_put(msg);

	zassert_equal((used_slabs - 3),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 3;
}

void test_log_hexdump_data_get_single_chunk(void)
{
	struct log_msg *msg;
	u8_t data[128];
	u8_t read_data[128];
	size_t offset;
	u32_t wr_length;
	size_t rd_length;
	size_t rd_req_length;

	for (int i = 0; i < sizeof(data); i++) {
		data[i] = i;
	}

	/* allocation of buffer that fits in single buffer */
	wr_length = LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK - 4;
	msg = log_msg_hexdump_create("test", data, wr_length);

	offset = 0;
	rd_length = wr_length - 1;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset],
		     read_data,
		     rd_length) == 0,
			"Expected data.\n");

	/* Attempt to read more data than present in the message */
	offset = 0;
	rd_length = wr_length + 1; /* requesting read more data */
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);
	zassert_equal(rd_length,
		      wr_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset],
		     read_data,
		     rd_length) == 0,
		     "Expected data.\n");

	/* Attempt to read with non zero offset, requested length fits in the
	 * buffer.
	 */
	offset = 4;
	rd_length = 1; /* requesting read more data */
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset],
		     read_data,
		     rd_length) == 0,
		     "Expected data.\n");

	/* Attempt to read with non zero offset, requested length DOES NOT fit
	 * in the buffer.
	 */
	offset = 4;
	rd_length = LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      wr_length - offset,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset],
		     read_data,
		     rd_length) == 0,
		     "Expected data.\n");

	log_msg_put(msg);
}

void test_log_hexdump_data_get_two_chunks(void)
{
	struct log_msg *msg;
	u8_t data[128];
	u8_t read_data[128];
	size_t offset;
	u32_t wr_length;
	size_t rd_length;
	size_t rd_req_length;

	for (int i = 0; i < sizeof(data); i++) {
		data[i] = i;
	}

	/* allocation of buffer that fits in two chunks. */
	wr_length = LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK;
	msg = log_msg_hexdump_create("test", data, wr_length);

	/* Read whole data from offset = 0*/
	offset = 0;
	rd_length = wr_length;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset],
		     read_data,
		     rd_length) == 0,
		     "Expected data.\n");

	/* Read data from first and second chunk. */
	offset = 1;
	rd_length = wr_length - 2;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset],
		     read_data,
		     rd_length) == 0,
		     "Expected data.\n");

	/* Read data from second chunk. */
	offset = wr_length - 2;
	rd_length = 1;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset], read_data, rd_length) == 0,
		     "Expected data.\n");

	/* Read more than available */
	offset = wr_length - 2;
	rd_length = wr_length;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      wr_length - offset,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset], read_data, rd_length) == 0,
		     "Expected data.\n");

	log_msg_put(msg);
}

void test_log_hexdump_data_get_multiple_chunks(void)
{
	struct log_msg *msg;
	u8_t data[128];
	u8_t read_data[128];
	size_t offset;
	u32_t wr_length;
	size_t rd_length;
	size_t rd_req_length;

	for (int i = 0; i < sizeof(data); i++) {
		data[i] = i;
	}

	/* allocation of buffer that fits in two chunks. */
	wr_length = 40;
	msg = log_msg_hexdump_create("test", data, wr_length);

	/* Read whole data from offset = 0*/
	offset = 0;
	rd_length = wr_length;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);


	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset], read_data, rd_length) == 0,
		     "Expected data.\n");

	/* Read data with offset starting from second chunk. */
	offset = LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK + 4;
	rd_length = wr_length - offset - 2;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      rd_req_length,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset], read_data, rd_length) == 0,
		     "Expected data.\n");

	/* Read data from second chunk with saturation. */
	offset = LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK + 4;
	rd_length = wr_length - offset + 1;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      wr_length - offset,
		      "Expected to read requested amount of data\n");

	zassert_true(memcmp(&data[offset], read_data, rd_length) == 0,
		     "Expected data.\n");


	/* Read beyond message */
	offset = wr_length + 1;
	rd_length = 1;
	rd_req_length = rd_length;

	log_msg_hexdump_data_get(msg,
				 read_data,
				 &rd_length,
				 offset);

	zassert_equal(rd_length,
		      0,
		      "Expected to read requested amount of data\n");

	log_msg_put(msg);
}


/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_message,
		ztest_unit_test(test_log_std_msg),
		ztest_unit_test(test_log_hexdump_msg),
		ztest_unit_test(test_log_hexdump_data_get_single_chunk),
		ztest_unit_test(test_log_hexdump_data_get_two_chunks),
		ztest_unit_test(test_log_hexdump_data_get_multiple_chunks));
	ztest_run_test_suite(test_log_message);
}
