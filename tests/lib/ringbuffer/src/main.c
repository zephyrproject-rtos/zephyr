/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

/**
 * @defgroup lib_ringbuffer_tests Ringbuffer
 * @ingroup all_tests
 * @{
 * @}
 */

RING_BUF_ITEM_DECLARE_POW2(ring_buf1, 8);

#define TYPE    1
#define VALUE   2
#define INITIAL_SIZE    2


#define RINGBUFFER_SIZE 5
#define DATA_MAX_SIZE 3
#define POW 2
extern void test_ringbuffer_concurrent(void);
extern void test_ringbuffer_zerocpy_stress(void);
extern void test_ringbuffer_cpy_stress(void);
extern void test_ringbuffer_item_stress(void);
/**
 * @brief Test APIs of ring buffer
 *
 * @details
 * Test Objective:
 * - Define and initialize a ring buffer and
 * the ring buffer copy data out of the array by
 * ring_buf_item_put(), and then ring buffer data
 * is copied into the array by ring_buf_item_get()
 * return error when full/empty.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Equivalence classes and input partition testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define and initialize a ringbuffer by using macro
 * RING_BUF_ITEM_DECLARE_POW2
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Defined an array with some data items that ready for being
 * put.
 * -# Put data items with "while loop".
 * -# Check if an error will be seen when the ringbuffer is full.
 * -# Get data items from the ringbuffer.
 * -# Check if the data put are equal to the data got.
 * -# Going on getting data from the ringbuffer.
 * -# Check if an error will be seen when the ringbuffer is empty.
 *
 * Expected Test Result:
 * - Data items pushed shall be equal to what are gotten. And
 * An error shall be shown up when an item is put into a full ringbuffer or
 * get some items from an empty ringbuffer.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 3.5,7 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put, ring_buf_item_get
 */
void test_ring_buffer_main(void)
{
	int ret, put_count, i;
	uint32_t getdata[6];
	uint8_t getsize, getval;
	uint16_t gettype;
	int dsize = INITIAL_SIZE;

	__aligned(sizeof(uint32_t)) char rb_data[] = "ABCDEFGHIJKLMNOPQRSTUVWX";
	put_count = 0;

	while (1) {
		ret = ring_buf_item_put(&ring_buf1, TYPE, VALUE,
				       (uint32_t *)rb_data, dsize);
		if (ret == -EMSGSIZE) {
			LOG_DBG("ring buffer is full");
			break;
		}
		LOG_DBG("inserted %d chunks, %d remaining", dsize,
			    ring_buf_space_get(&ring_buf1));
		dsize = (dsize + 1) % SIZE32_OF(rb_data);
		put_count++;
	}

	getsize = INITIAL_SIZE - 1;
	ret = ring_buf_item_get(&ring_buf1, &gettype, &getval,
				getdata, &getsize);
	if (ret != -EMSGSIZE) {
		LOG_DBG("Allowed retrieval with insufficient "
			"destination buffer space");
		zassert_true((getsize == INITIAL_SIZE),
			     "Correct size wasn't reported back to the caller");
	}

	for (i = 0; i < put_count; i++) {
		getsize = SIZE32_OF(getdata);
		ret = ring_buf_item_get(&ring_buf1, &gettype, &getval, getdata,
				       &getsize);
		zassert_true((ret == 0), "Couldn't retrieve a stored value");
		LOG_DBG("got %u chunks of type %u and val %u, %u remaining",
			    getsize, gettype, getval,
			    ring_buf_item_space_get(&ring_buf1));

		zassert_true((memcmp((char *)getdata, rb_data,
			getsize * sizeof(uint32_t)) == 0), "data corrupted");
		zassert_true((gettype == TYPE), "type information corrupted");
		zassert_true((getval == VALUE), "value information corrupted");
	}

	getsize = SIZE32_OF(getdata);
	ret = ring_buf_item_get(&ring_buf1, &gettype, &getval, getdata,
			       &getsize);
	zassert_true((ret == -EAGAIN), "Got data out of an empty buffer");
}

/**TESTPOINT: init via RING_BUF_ITEM_DECLARE_POW2*/
RING_BUF_ITEM_DECLARE_POW2(ringbuf_pow2, POW);

/**TESTPOINT: init via RING_BUF_ITEM_DECLARE*/
/**
 * @brief define a ring buffer with arbitrary size
 *
 * @see RING_BUF_ITEM_DECLARE(), RING_BUF_DECLARE()
 */
RING_BUF_ITEM_DECLARE(ringbuf_size, RINGBUFFER_SIZE);

RING_BUF_DECLARE(ringbuf_raw, RINGBUFFER_SIZE);

static struct ring_buf ringbuf, *pbuf;

static uint32_t buffer[RINGBUFFER_SIZE];

static struct {
	uint8_t length;
	uint8_t value;
	uint16_t type;
	uint32_t buffer[DATA_MAX_SIZE];
} data[] = {
	{ 0, 32, 1, {} },
	{ 1, 76, 54, { 0x89ab } },
	{ 3, 0xff, 0xffff, { 0x0f0f, 0xf0f0, 0xff00 } }
};

/*entry of contexts*/
static void tringbuf_put(const void *p)
{
	int index = POINTER_TO_INT(p);
	/**TESTPOINT: ring buffer put*/
	int ret = ring_buf_item_put(pbuf, data[index].type, data[index].value,
				   data[index].buffer, data[index].length);

	zassert_equal(ret, 0, NULL);
}

static void tringbuf_get(const void *p)
{
	uint16_t type;
	uint8_t value, size32 = DATA_MAX_SIZE;
	uint32_t rx_data[DATA_MAX_SIZE];
	int ret, index = POINTER_TO_INT(p);

	/**TESTPOINT: ring buffer get*/
	ret = ring_buf_item_get(pbuf, &type, &value, rx_data, &size32);
	zassert_equal(ret, 0, NULL);
	zassert_equal(type, data[index].type, NULL);
	zassert_equal(value, data[index].value, NULL);
	zassert_equal(size32, data[index].length, NULL);
	zassert_equal(memcmp(rx_data, data[index].buffer, size32), 0, NULL);
}

static void tringbuf_get_discard(const void *p)
{
	uint16_t type;
	uint8_t value, size32;
	int ret, index = POINTER_TO_INT(p);

	/**TESTPOINT: ring buffer get*/
	ret = ring_buf_item_get(pbuf, &type, &value, NULL, &size32);
	zassert_equal(ret, 0, NULL);
	zassert_equal(type, data[index].type, NULL);
	zassert_equal(value, data[index].value, NULL);
	zassert_equal(size32, data[index].length, NULL);
}

/*test cases*/
void test_ringbuffer_init(void)
{
	/**TESTPOINT: init via ring_buf_item_init*/
	ring_buf_item_init(&ringbuf, RINGBUFFER_SIZE, buffer);
	zassert_true(ring_buf_is_empty(&ringbuf), NULL);
	zassert_equal(ring_buf_item_space_get(&ringbuf), RINGBUFFER_SIZE, NULL);
}

void test_ringbuffer_declare_pow2(void)
{
	zassert_true(ring_buf_is_empty(&ringbuf_pow2), NULL);
	zassert_equal(ring_buf_item_space_get(&ringbuf_pow2), (1 << POW), NULL);
}

void test_ringbuffer_declare_size(void)
{
	zassert_true(ring_buf_is_empty(&ringbuf_size), NULL);
	zassert_equal(ring_buf_item_space_get(&ringbuf_size), RINGBUFFER_SIZE,
		      NULL);
}

/**
 * @brief verify that ringbuffer can be placed in any user-controlled memory
 *
 * @details
 * Test Objective:
 * - define and initialize a ring buffer by struct ring_buf,
 * then passing data by thread to verify the ringbuffer
 * if it works to be placed in any user-controlled memory.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define and initialize a ringbuffer by using struct ring_buf
 * - Define a pointer of ring buffer type.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Put data items into a ringbuffer
 * -# Get data items from a ringbuffer
 * -# Check if data items pushed are equal to what are gotten.
 * -# Repeat 1,2,3 to verify the ringbuffer is working normally.
 *
 * Expected Test Result:
 * - data items pushed shall be equal to what are gotten.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 3,4 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put, ring_buf_item_get
 */
void test_ringbuffer_put_get_thread(void)
{
	pbuf = &ringbuf;
	for (int i = 0; i < 1000; i++) {
		tringbuf_put((const void *)0);
		tringbuf_put((const void *)1);
		tringbuf_get((const void *)0);
		tringbuf_get((const void *)1);
		tringbuf_put((const void *)2);
		zassert_false(ring_buf_is_empty(pbuf), NULL);
		tringbuf_get((const void *)2);
		zassert_true(ring_buf_is_empty(pbuf), NULL);
	}
}

void test_ringbuffer_put_get_isr(void)
{
	pbuf = &ringbuf;
	irq_offload(tringbuf_put, (const void *)0);
	irq_offload(tringbuf_put, (const void *)1);
	irq_offload(tringbuf_get, (const void *)0);
	irq_offload(tringbuf_get, (const void *)1);
	irq_offload(tringbuf_put, (const void *)2);
	zassert_false(ring_buf_is_empty(pbuf), NULL);
	irq_offload(tringbuf_get, (const void *)2);
	zassert_true(ring_buf_is_empty(pbuf), NULL);
}

void test_ringbuffer_put_get_thread_isr(void)
{
	pbuf = &ringbuf;
	tringbuf_put((const void *)0);
	irq_offload(tringbuf_put, (const void *)1);
	tringbuf_get((const void *)0);
	irq_offload(tringbuf_get, (const void *)1);
	tringbuf_put((const void *)2);
	irq_offload(tringbuf_get, (const void *)2);
}

void test_ringbuffer_put_get_discard(void)
{
	pbuf = &ringbuf;
	tringbuf_put((const void *)0);
	tringbuf_put((const void *)1);
	zassert_false(ring_buf_is_empty(pbuf), NULL);
	tringbuf_get_discard((const void *)0);
	tringbuf_get_discard((const void *)1);
	zassert_true(ring_buf_is_empty(pbuf), NULL);
}

/**
 * @brief verify that ringbuffer can be placed in any user-controlled memory
 *
 * @details
 * Test Objective:
 * - define and initialize a ring buffer by macro RING_BUF_ITEM_DECLARE_POW2,
 * then passing data by thread and isr to verify the ringbuffer
 * if it works to be placed in any user-controlled memory.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define and initialize a ringbuffer by RING_BUF_ITEM_DECLARE_POW2
 * - Define a pointer of ring_buffer type.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Put data items into the ringbuffer by a thread
 * -# Put data items into the ringbuffer by a ISR
 * -# Get data items from the ringbuffer by the thread
 * -# Check if data items pushed are equal to what are gotten.
 * -# Get data items from the ringbuffer by the ISR
 * -# Check if data items pushed are equal to what are gotten.
 * -# Put data items into the ringbuffer by the thread
 * -# Get data items from the ringbuffer by the ISR
 * -# Check if data items pushed are equal to what are gotten.
 *
 * Expected Test Result:
 * - data items pushed shall be equal to what are gotten.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 4,6,9 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put, ring_buf_item_get
 */
void test_ringbuffer_pow2_put_get_thread_isr(void)
{
	pbuf = &ringbuf_pow2;
	tringbuf_put((const void *)0);
	irq_offload(tringbuf_put, (const void *)1);
	tringbuf_get((const void *)0);
	irq_offload(tringbuf_get, (const void *)1);
	tringbuf_put((const void *)1);
	irq_offload(tringbuf_get, (const void *)1);
}

/**
 * @brief verify that ringbuffer can be placed in any user-controlled memory
 *
 * @details
 * Test Objective:
 * - define and initialize a ring buffer by macro RING_BUF_ITEM_DECLARE,
 * then passing data by thread and isr to verify the ringbuffer
 * if it works to be placed in any user-controlled memory.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define and initialize a ringbuffer by RING_BUF_ITEM_DECLARE
 * - Define a pointer of ring buffer type.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Put data items into the ringbuffer by a thread
 * -# Put data items into the ringbuffer by a ISR
 * -# Get data items from the ringbuffer by the thread
 * -# Check if data items pushed are equal to what are gotten.
 * -# Get data items from the ringbuffer by the ISR
 * -# Check if data items pushed are equal to what are gotten.
 * -# Put data items into the ringbuffer by the thread
 * -# Get data items from the ringbuffer by the ISR
 * -# Check if data items pushed are equal to what are gotten.
 *
 * Expected Test Result:
 * - data items pushed shall be equal to what are gotten.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 4,6,9 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put, ring_buf_item_get
 */
void test_ringbuffer_size_put_get_thread_isr(void)
{
	pbuf = &ringbuf_size;
	tringbuf_put((const void *)0);
	irq_offload(tringbuf_put, (const void *)1);
	tringbuf_get((const void *)0);
	irq_offload(tringbuf_get, (const void *)1);
	tringbuf_put((const void *)2);
	irq_offload(tringbuf_get, (const void *)2);
}

/**
 * @brief verify that ringbuffer can be placed in any user-controlled memory
 *
 * @details
 * Test Objective:
 * - define and initialize a ring buffer by macro RING_BUF_DECLARE,
 * then verify data is passed between ring buffer and array
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define and initialize a ringbuffer by RING_BUF_DECLARE
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define two arrays(inbuf,outbuf) and initialize inbuf
 * -# Put and get data with "for loop"
 * -# Check if data size pushed is equal to what are gotten.
 * -# Then initialize the output buffer
 * -# Put data with different size to check if data size
 * pushed is equal to what are gotten.
 *
 * Expected Test Result:
 * - data items pushed shall be equal to what are gotten.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 4,5 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_put, ring_buf_get
 */
void test_ringbuffer_raw(void)
{
	int i;
	uint8_t inbuf[RINGBUFFER_SIZE];
	uint8_t outbuf[RINGBUFFER_SIZE];
	size_t in_size;
	size_t out_size;

	/* Initialize test buffer. */
	for (i = 0; i < RINGBUFFER_SIZE; i++) {
		inbuf[i] = i;
	}

	for (i = 0; i < 10; i++) {
		memset(outbuf, 0, sizeof(outbuf));
		in_size = ring_buf_put(&ringbuf_raw, inbuf,
					       RINGBUFFER_SIZE - 2);
		out_size = ring_buf_get(&ringbuf_raw, outbuf,
						RINGBUFFER_SIZE - 2);

		zassert_true(in_size == RINGBUFFER_SIZE - 2, NULL);
		zassert_true(in_size == out_size, NULL);
		zassert_true(memcmp(inbuf, outbuf, RINGBUFFER_SIZE - 2) == 0,
			     NULL);
	}

	memset(outbuf, 0, sizeof(outbuf));
	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       RINGBUFFER_SIZE);
	zassert_equal(in_size, RINGBUFFER_SIZE, NULL);

	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       1);
	zassert_equal(in_size, 0, NULL);

	out_size = ring_buf_get(&ringbuf_raw, outbuf,
					RINGBUFFER_SIZE);

	zassert_true(out_size == RINGBUFFER_SIZE, NULL);

	out_size = ring_buf_get(&ringbuf_raw, outbuf,
					RINGBUFFER_SIZE + 1);
	zassert_true(out_size == 0, NULL);
	zassert_true(ring_buf_is_empty(&ringbuf_raw), NULL);

	/* Validate that raw bytes can be discarded */
	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       RINGBUFFER_SIZE);
	zassert_equal(in_size, RINGBUFFER_SIZE, NULL);

	out_size = ring_buf_get(&ringbuf_raw, NULL,
					RINGBUFFER_SIZE);
	zassert_true(out_size == RINGBUFFER_SIZE, NULL);

	out_size = ring_buf_get(&ringbuf_raw, NULL,
					RINGBUFFER_SIZE + 1);
	zassert_true(out_size == 0, NULL);
	zassert_true(ring_buf_is_empty(&ringbuf_raw), NULL);
}

void test_ringbuffer_alloc_put(void)
{
	uint8_t outputbuf[RINGBUFFER_SIZE];
	uint8_t inputbuf[] = {1, 2, 3, 4};
	uint32_t read_size;
	uint32_t allocated;
	uint8_t *data;
	int err;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buffer);

	allocated = ring_buf_put_claim(&ringbuf_raw, &data, 1);
	zassert_true(allocated == 1U, NULL);


	allocated = ring_buf_put_claim(&ringbuf_raw, &data,
					   RINGBUFFER_SIZE - 1);
	zassert_true(allocated == RINGBUFFER_SIZE - 1, NULL);

	/* Putting too much returns error */
	err = ring_buf_put_finish(&ringbuf_raw, RINGBUFFER_SIZE + 1);
	zassert_true(err != 0, NULL);

	err = ring_buf_put_finish(&ringbuf_raw, 1);
	zassert_true(err == 0, NULL);

	err = ring_buf_put_finish(&ringbuf_raw, RINGBUFFER_SIZE - 1);
	zassert_true(err == -EINVAL, NULL);

	read_size = ring_buf_get(&ringbuf_raw, outputbuf,
					     RINGBUFFER_SIZE);
	zassert_true(read_size == 1, NULL);

	for (int i = 0; i < 10; i++) {
		allocated = ring_buf_put_claim(&ringbuf_raw, &data, 2);
		if (allocated == 2U) {
			data[0] = inputbuf[0];
			data[1] = inputbuf[1];
		} else {
			data[0] = inputbuf[0];
			ring_buf_put_claim(&ringbuf_raw, &data, 1);
			data[0] = inputbuf[1];
		}

		allocated = ring_buf_put_claim(&ringbuf_raw, &data, 2);
		if (allocated == 2U) {
			data[0] = inputbuf[2];
			data[1] = inputbuf[3];
		} else {
			data[0] = inputbuf[2];
			ring_buf_put_claim(&ringbuf_raw, &data, 1);
			data[0] = inputbuf[3];
		}

		err = ring_buf_put_finish(&ringbuf_raw, 4);
		zassert_true(err == 0, NULL);

		read_size = ring_buf_get(&ringbuf_raw,
						     outputbuf, 4);
		zassert_true(read_size == 4U, NULL);

		zassert_true(memcmp(outputbuf, inputbuf, 4) == 0, NULL);
	}
}

void test_byte_put_free(void)
{
	uint8_t indata[] = {1, 2, 3, 4, 5};
	int err;
	uint32_t granted;
	uint8_t *data;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buffer);

	/* Ring buffer is empty */
	granted = ring_buf_get_claim(&ringbuf_raw, &data, RINGBUFFER_SIZE);
	zassert_true(granted == 0U, NULL);

	for (int i = 0; i < 10; i++) {
		ring_buf_put(&ringbuf_raw, indata,
					 RINGBUFFER_SIZE-2);

		granted = ring_buf_get_claim(&ringbuf_raw, &data,
					       RINGBUFFER_SIZE);

		if (granted == (RINGBUFFER_SIZE-2)) {
			zassert_true(memcmp(indata, data, granted) == 0, NULL);
		} else if (granted < (RINGBUFFER_SIZE-2)) {
			/* When buffer wraps, operation is split. */
			uint32_t granted_1 = granted;

			zassert_true(memcmp(indata, data, granted) == 0, NULL);
			granted = ring_buf_get_claim(&ringbuf_raw, &data,
						       RINGBUFFER_SIZE);

			zassert_true((granted + granted_1) ==
					RINGBUFFER_SIZE - 2, NULL);
			zassert_true(memcmp(&indata[granted_1], data, granted)
					== 0, NULL);
		} else {
			zassert_true(false, NULL);
		}

		/* Freeing more than possible case. */
		err = ring_buf_get_finish(&ringbuf_raw, RINGBUFFER_SIZE-1);
		zassert_true(err != 0, NULL);

		err = ring_buf_get_finish(&ringbuf_raw, RINGBUFFER_SIZE-2);
		zassert_true(err == 0, NULL);
	}
}

void test_capacity(void)
{
	uint32_t capacity;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buffer);

	capacity = ring_buf_capacity_get(&ringbuf_raw);
	zassert_equal(RINGBUFFER_SIZE, capacity,
			"Unexpected capacity");
}

void test_size(void)
{
	uint32_t size;
	static uint8_t buf[RINGBUFFER_SIZE];

	ring_buf_init(&ringbuf_raw, sizeof(buf), ringbuf_raw.buffer);

	/* Test 0 */
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(0, size, "wrong size: exp: %u act: %u", 0, size);

	/* Test 1 */
	ring_buf_put(&ringbuf_raw, "x", 1);
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(1, size, "wrong size: exp: %u act: %u", 1, size);

	/* Test N */
	ring_buf_reset(&ringbuf_raw);
	ring_buf_put(&ringbuf_raw, buf, sizeof(buf));
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(sizeof(buf), size, "wrong size: exp: %u: actual: %u", sizeof(buf), size);

	/* Test N - 2 with wrap-around */
	ring_buf_put(&ringbuf_raw, buf, sizeof(buf));
	ring_buf_get(&ringbuf_raw, NULL, 3);
	ring_buf_put(&ringbuf_raw, "x", 1);

	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(sizeof(buf) - 2, size, "wrong size: exp: %u: actual: %u", sizeof(buf) - 2,
		      size);
}

void test_peek(void)
{
	uint32_t size;
	uint8_t byte = 0x42;
	static uint8_t buf[RINGBUFFER_SIZE];

	ring_buf_init(&ringbuf_raw, sizeof(buf), ringbuf_raw.buffer);

	/* Test 0 */
	size = ring_buf_peek(&ringbuf_raw, (uint8_t *)0x1, 42424242);
	zassert_equal(0, size, "wrong peek size: exp: %u: actual: %u", 0, size);

	/* Test 1 */
	ring_buf_put(&ringbuf_raw, "*", 1);
	size = ring_buf_peek(&ringbuf_raw, &byte, 1);
	zassert_equal(1, size, "wrong peek size: exp: %u: actual: %u", 1, size);
	zassert_equal('*', byte, "wrong buffer contents: exp: %u: actual: %u", '*', byte);
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(1, size, "wrong buffer size: exp: %u: actual: %u", 1, size);

	/* Test N */
	ring_buf_reset(&ringbuf_raw);
	for (size = 0; size < sizeof(buf); ++size) {
		buf[size] = 'A' + (size % ('Z' - 'A' + 1));
	}

	ring_buf_put(&ringbuf_raw, buf, sizeof(buf));
	memset(buf, '*', sizeof(buf)); /* fill with pattern */

	size = ring_buf_peek(&ringbuf_raw, buf, sizeof(buf));
	zassert_equal(sizeof(buf), size, "wrong peek size: exp: %u: actual: %u", sizeof(buf), size);
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(sizeof(buf), size, "wrong buffer size: exp: %u: actual: %u", sizeof(buf),
		      size);

	for (size = 0; size < sizeof(buf); ++size) {
		ringbuf_raw.buffer[size] = 'A' + (size % ('Z' - 'A' + 1));
	}

	zassert_equal(0, memcmp(buf, ringbuf_raw.buffer, sizeof(buf)),
		      "content validation failed");
}

void test_reset(void)
{
	uint8_t indata[] = {1, 2, 3, 4, 5};
	uint8_t outdata[RINGBUFFER_SIZE];
	uint8_t *outbuf;
	uint32_t len;
	uint32_t out_len;
	uint32_t granted;
	uint32_t space;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buffer);

	len = 3;
	out_len = ring_buf_put(&ringbuf_raw, indata, len);
	zassert_equal(out_len, len, NULL);

	out_len = ring_buf_get(&ringbuf_raw, outdata, len);
	zassert_equal(out_len, len, NULL);

	space = ring_buf_space_get(&ringbuf_raw);
	zassert_equal(space, RINGBUFFER_SIZE, NULL);

	/* Even though ringbuffer is empty, full buffer cannot be allocated
	 * because internal pointers are not at the beginning.
	 */
	granted = ring_buf_put_claim(&ringbuf_raw, &outbuf, RINGBUFFER_SIZE);
	zassert_false(granted == RINGBUFFER_SIZE, NULL);

	/* After reset full buffer can be allocated. */
	ring_buf_reset(&ringbuf_raw);
	granted = ring_buf_put_claim(&ringbuf_raw, &outbuf, RINGBUFFER_SIZE);
	zassert_true(granted == RINGBUFFER_SIZE, NULL);
}

static uint32_t ringbuf_stored[RINGBUFFER_SIZE];

/**
 * @brief verify the array stored by ringbuf
 *
 * @details
 * Test Objective:
 * - Define a buffer stored by ringbuffer. Verify that the address
 * of the buffer is contiguous.And also verify that data can pass
 * between buffer and ringbuffer.
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define an array that changes as the system changes.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define two buffers(input and output)
 * -# Put data from input buffer into the ringbuffer
 * and check if put data are successful.
 * -# Check if the address stored by ringbuf is contiguous.
 * -# Get data from the ringbuffer and put them into output buffer
 * and check if getting data are successful.
 *
 * Expected Test Result:
 * - All assertions can pass.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 2,3,4,5 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put, ring_buf_item_get
 */
void test_ringbuffer_array_perf(void)
{
	struct ring_buf buf_ii;
	uint32_t input[3] = {0xaa, 0xbb, 0xcc};
	uint32_t output[3] = {0};
	uint16_t type = 0;
	uint8_t value = 0, size = 3;

	ring_buf_item_init(&buf_ii, RINGBUFFER_SIZE, ringbuf_stored);

	/*Data from the beginning of the array can be copied into the ringbuf*/
	zassert_true(ring_buf_item_put(&buf_ii, 1, 2, input, 3) == 0, NULL);

	/*Verify the address stored by ringbuf is contiguous*/
	for (int i = 0; i < 3; i++) {
		uint32_t *buf32 = (uint32_t *)buf_ii.buffer;

		zassert_equal(input[i], buf32[i+1], NULL);
	}

	/*Data from the end of the ringbuf can be copied into the array*/
	zassert_true(ring_buf_item_get(&buf_ii, &type, &value,
				output, &size) == 0, NULL);

	/*Verify the ringbuf defined is working*/
	for (int i = 0; i < 3; i++) {
		zassert_equal(input[i], output[i], NULL);
	}
}

void test_ringbuffer_partial_putting(void)
{
	uint8_t indata[RINGBUFFER_SIZE];
	uint8_t outdata[RINGBUFFER_SIZE];
	uint32_t len;
	uint32_t len2;
	uint32_t req_len;
	uint8_t *ptr;

	ring_buf_reset(&ringbuf_raw);

	for (int i = 0; i < 100; i++) {
		req_len = (i % RINGBUFFER_SIZE) + 1;
		len = ring_buf_put(&ringbuf_raw, indata, req_len);
		zassert_equal(req_len, len, NULL);

		len = ring_buf_get(&ringbuf_raw, outdata, req_len);
		zassert_equal(req_len, len, NULL);

		req_len = 2;
		len = ring_buf_put_claim(&ringbuf_raw, &ptr, req_len);
		zassert_equal(len, req_len, NULL);

		req_len = RINGBUFFER_SIZE;
		len = ring_buf_put_claim(&ringbuf_raw, &ptr, req_len);
		len2 = ring_buf_put_claim(&ringbuf_raw, &ptr, req_len);
		zassert_equal(len + len2, req_len - 2, NULL);

		ring_buf_put_finish(&ringbuf_raw, req_len);

		len = ring_buf_get(&ringbuf_raw, indata, req_len);
		zassert_equal(len, req_len, NULL);
	}
}

void test_ringbuffer_partial_getting(void)
{
	uint8_t indata[RINGBUFFER_SIZE];
	uint8_t outdata[RINGBUFFER_SIZE];
	uint32_t len;
	uint32_t len2;
	uint32_t req_len;
	uint8_t *ptr;

	ring_buf_reset(&ringbuf_raw);

	for (int i = 0; i < 100; i++) {
		req_len = (i % RINGBUFFER_SIZE) + 1;
		len = ring_buf_put(&ringbuf_raw, indata, req_len);
		zassert_equal(req_len, len, NULL);

		len = ring_buf_get(&ringbuf_raw, outdata, req_len);
		zassert_equal(req_len, len, NULL);

		req_len = sizeof(indata);
		len = ring_buf_put(&ringbuf_raw, indata, req_len);
		zassert_equal(req_len, len, NULL);

		len = ring_buf_get_claim(&ringbuf_raw, &ptr, 2);
		zassert_equal(len, 2, NULL);

		len = ring_buf_get_claim(&ringbuf_raw, &ptr, RINGBUFFER_SIZE);
		len2 = ring_buf_get_claim(&ringbuf_raw, &ptr, RINGBUFFER_SIZE);
		zassert_equal(len + len2, RINGBUFFER_SIZE - 2, NULL);

		ring_buf_get_finish(&ringbuf_raw, RINGBUFFER_SIZE);
	}
}

void test_ringbuffer_equal_bufs(void)
{
	struct ring_buf buf_ii;
	uint8_t *data;
	uint32_t claimed;
	uint8_t buf[8];
	size_t halfsize = sizeof(buf)/2;

	ring_buf_init(&buf_ii, sizeof(buf), buf);

	for (int i = 0; i < 100; i++) {
		claimed = ring_buf_put_claim(&buf_ii, &data, halfsize);
		zassert_equal(claimed, halfsize, NULL);
		ring_buf_put_finish(&buf_ii, claimed);

		claimed = ring_buf_get_claim(&buf_ii, &data, halfsize);
		zassert_equal(claimed, halfsize, NULL);
		ring_buf_get_finish(&buf_ii, claimed);
	}
}

void test_ringbuffer_performance(void)
{
	uint8_t buf[16];
	static struct ring_buf rbuf;
	uint8_t indata[16];
	uint8_t outdata[16];
	uint8_t *ptr;
	uint32_t timestamp;
	int loop = 1000;

	ring_buf_init(&rbuf, sizeof(buf), buf);

	/* Test performance of copy put get 1 byte */
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buf_put(&rbuf, indata, 1);
		ring_buf_get(&rbuf, outdata, 1);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	PRINT("1 byte put-get, avg cycles: %d\n", timestamp/loop);

	/* Test performance of copy put get 1 byte */
	ring_buf_reset(&rbuf);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buf_put(&rbuf, indata, 4);
		ring_buf_get(&rbuf, outdata, 4);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	PRINT("4 byte put-get, avg cycles: %d\n", timestamp/loop);

	/* Test performance of put claim finish 1 byte */
	ring_buf_reset(&rbuf);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buf_put_claim(&rbuf, &ptr, 1);
		ring_buf_put_finish(&rbuf, 1);
		ring_buf_get(&rbuf, outdata, 1);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	PRINT("1 byte put claim-finish, avg cycles: %d\n", timestamp/loop);

	/* Test performance of put claim finish 5 byte */
	ring_buf_reset(&rbuf);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buf_put_claim(&rbuf, &ptr, 5);
		ring_buf_put_finish(&rbuf, 5);
		ring_buf_get(&rbuf, outdata, 5);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	PRINT("5 byte put claim-finish, avg cycles: %d\n", timestamp/loop);

	/* Test performance of copy put claim finish 5 byte */
	ring_buf_reset(&rbuf);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buf_put(&rbuf, indata, 5);
		ring_buf_get_claim(&rbuf, &ptr, 5);
		ring_buf_get_finish(&rbuf, 5);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	PRINT("5 byte get claim-finish, avg cycles: %d\n", timestamp/loop);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_ringbuffer_api,
		       ztest_unit_test(test_ringbuffer_init),/*keep init first!*/
		       ztest_unit_test(test_ringbuffer_declare_pow2),
		       ztest_unit_test(test_ringbuffer_declare_size),
		       ztest_unit_test(test_ringbuffer_put_get_thread),
		       ztest_unit_test(test_ringbuffer_put_get_isr),
		       ztest_unit_test(test_ringbuffer_put_get_thread_isr),
		       ztest_unit_test(test_ringbuffer_put_get_discard),
		       ztest_unit_test(test_ringbuffer_pow2_put_get_thread_isr),
		       ztest_unit_test(test_ringbuffer_size_put_get_thread_isr),
		       ztest_unit_test(test_ringbuffer_array_perf),
		       ztest_unit_test(test_ringbuffer_partial_putting),
		       ztest_unit_test(test_ringbuffer_partial_getting),
		       ztest_unit_test(test_ring_buffer_main),
		       ztest_unit_test(test_ringbuffer_raw),
		       ztest_unit_test(test_ringbuffer_alloc_put),
		       ztest_unit_test(test_byte_put_free),
		       ztest_unit_test(test_ringbuffer_equal_bufs),
		       ztest_unit_test(test_capacity),
		       ztest_unit_test(test_size),
		       ztest_unit_test(test_peek),
		       ztest_unit_test(test_reset),
		       ztest_unit_test(test_ringbuffer_performance),
		       ztest_unit_test(test_ringbuffer_concurrent),
		       ztest_unit_test(test_ringbuffer_zerocpy_stress),
		       ztest_unit_test(test_ringbuffer_cpy_stress),
		       ztest_unit_test(test_ringbuffer_item_stress)
		);
	ztest_run_test_suite(test_ringbuffer_api);
}
