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
	zassert_equal(LOG_MSG_NARGS_SINGLE_CHUNK,
		      IS_ENABLED(CONFIG_64BIT) ? 4 : 3,
		      "test assumes following setting");

	u32_t used_slabs = k_mem_slab_num_used_get(&log_msg_pool);
	log_arg_t args[] = {1, 2, 3, 4, 5, 6};
	struct log_msg *msg;

	/* Test for expected buffer usage based on number of arguments */
	for (int i = 0; i <= 6; i++) {
		switch (i) {
		case 0:
			msg = log_msg_create_0(my_string);
			break;
		case 1:
			msg = log_msg_create_1(my_string, 1);
			break;
		case 2:
			msg = log_msg_create_2(my_string, 1, 2);
			break;
		case 3:
			msg = log_msg_create_3(my_string, 1, 2, 3);
			break;
		default:
			msg = log_msg_create_n(my_string, args, i);
			break;
		}

		used_slabs += (i > LOG_MSG_NARGS_SINGLE_CHUNK) ? 2 : 1;
		zassert_equal(used_slabs,
			      k_mem_slab_num_used_get(&log_msg_pool),
			      "%d: Unexpected slabs used (expected:%d, got %d).",
			      i, used_slabs,
			      k_mem_slab_num_used_get(&log_msg_pool));

		log_msg_put(msg);

		used_slabs -= (i > LOG_MSG_NARGS_SINGLE_CHUNK) ? 2 : 1;
		zassert_equal(used_slabs,
			      k_mem_slab_num_used_get(&log_msg_pool),
			      "Expected mem slab allocation.");
	}
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

	zassert_equal((used_slabs + 2U),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 2U;

	log_msg_put(msg);

	zassert_equal((used_slabs - 2U),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 2U;

	/* allocation of buffer that fits in 3 buffers */
	msg = log_msg_hexdump_create("test", data,
				     LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK +
				     HEXDUMP_BYTES_CONT_MSG + 1);

	zassert_equal((used_slabs + 3U),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs += 3U;

	log_msg_put(msg);

	zassert_equal((used_slabs - 3U),
		      k_mem_slab_num_used_get(&log_msg_pool),
		      "Expected mem slab allocation.");
	used_slabs -= 3U;
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
	wr_length = 40U;
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

void test_hexdump_realloc(void)
{
	u8_t data[1];
	u8_t new_data[HEXDUMP_BYTES_CONT_MSG+1];
	u8_t rbuf[HEXDUMP_BYTES_CONT_MSG];
	size_t len;
	u32_t offset;
	int err;
	struct log_msg *msg = log_msg_hexdump_create(NULL, data, sizeof(data));

	for (int i = 0; i < sizeof(new_data); i++) {
		new_data[i] = i;
	}

	len = 4;
	log_msg_hexdump_data_put(msg, new_data, &len, 0);
	zassert_equal(1, len, "Unexpected len:%d", len);

	err = log_msg_hexdump_extend(msg, 4);
	zassert_equal(0, err, "Unexpected err:%d", err);

	len = 4;
	log_msg_hexdump_data_put(msg, new_data, &len, 0);
	zassert_equal(4, len, "Unexpected len:%d", len);

	err = log_msg_hexdump_extend(msg, HEXDUMP_BYTES_CONT_MSG);
	zassert_equal(0, err, "Unexpected err:%d", err);

	len = sizeof(new_data);
	log_msg_hexdump_data_put(msg, new_data, &len, 0);
	zassert_equal(sizeof(new_data), len, "Unexpected len:%d", len);

	err = log_msg_hexdump_extend(msg, HEXDUMP_BYTES_CONT_MSG);
	len = 4;
	offset = LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	log_msg_hexdump_data_put(msg, new_data, &len, offset);
	zassert_equal(4, len, "Unexpected len:%d", len);

	log_msg_hexdump_data_get(msg, rbuf, &len, offset);
	zassert_equal(4, len, "Unexpected len:%d", len);
	zassert_equal(0, memcmp(new_data, rbuf, len),
			"Expected different buffer content");

	log_msg_put(msg);
}


void test_hexdump_realloc_mutlichunk(void)
{
	struct log_msg *msg = log_msg_hexdump_create(NULL, NULL, 0);
	char inbuf[] = "123456789 qwerty uiopasd fghjk lzxcv bbnnm";
	u32_t test_len = sizeof(inbuf);
	char outbuf[sizeof(inbuf)];
	int err;
	size_t len = 1;

	for (int i = 0; i < test_len; i++) {
		err = log_msg_hexdump_extend(msg, 1);
		zassert_equal(0, err, "Unexpected err:%d", err);

		log_msg_hexdump_data_put(msg, &inbuf[i], &len, i);
		zassert_equal(1, len, "Unexpected len:%d", len);
	}

	len = test_len;
	log_msg_hexdump_data_get(msg, outbuf, &len, 0);
	zassert_equal(test_len, len, "Unexpected len:%d", len);

	err = memcmp(inbuf, outbuf, test_len);

	zassert_equal(err, 0, "Buffers do not match (ret: %d)", err);

	log_msg_put(msg);
}

static u32_t hf_cycle_get(void)
{

#if CONFIG_SOC_SERIES_NRF52X
	return DWT->CYCCNT;
#else
	return k_cycle_get_32();
#endif
}

static u32_t hexdump_create_timing(u32_t buf_size)
{
	u8_t buf[buf_size];
	static const int iterations = 5;
	struct log_msg *msg[iterations];
	u32_t elapsed;
	u32_t used = k_mem_slab_num_used_get(&log_msg_pool);
	u32_t start;

#if CONFIG_SOC_SERIES_NRF52X
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	DWT->CYCCNT = 0;
#endif
	start = hf_cycle_get();

	for (int i = 0; i < iterations; i++) {
		msg[i] = log_msg_hexdump_create(NULL, buf, buf_size);
	}
	elapsed = hf_cycle_get() - start;

	for (int i = 0; i < iterations; i++) {

		log_msg_put(msg[i]);
	}

	PRINT("Create %d byte hexdump message tool %d cycles\n",
			buf_size, elapsed/iterations);

	zassert_equal(used, k_mem_slab_num_used_get(&log_msg_pool),
			"Expected freeing all messages");
	return elapsed/iterations;
}

/* Test used to profile log_msg_hexdump_create function. Reliable results
 * require high frequency clock.
 */

void test_hexdump_create_timings(void)
{
	hexdump_create_timing(5);
	hexdump_create_timing(15);
	hexdump_create_timing(50);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_message,
		ztest_unit_test(test_hexdump_create_timings),
		ztest_unit_test(test_hexdump_realloc_mutlichunk),
		ztest_unit_test(test_hexdump_realloc),
		ztest_unit_test(test_log_std_msg),
		ztest_unit_test(test_log_hexdump_msg),
		ztest_unit_test(test_log_hexdump_data_get_single_chunk),
		ztest_unit_test(test_log_hexdump_data_get_two_chunks),
		ztest_unit_test(test_log_hexdump_data_get_multiple_chunks));
	ztest_run_test_suite(test_log_message);
}
