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
#include <irq_offload.h>
#include <sys/ring_buffer.h>

#include <logging/log.h>
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


/**
 * @brief test APIs of ring buffer
 *
 * @details define and initialize a ring buffer
 * the ring buffer copy data out of the array by
 * ring_buf_item_put(), and then ring buffer data
 * is copied into the array by ring_buf_item_get()
 * return error when full/empty.
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put(), ring_buf_item_get()
 *
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
		LOG_DBG("Allowed retreival with insufficient "
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
			    ring_buf_space_get(&ring_buf1));

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

/**TESTPOINT: init via RING_BUF_ITEM_DECLARE_SIZE*/
/**
 * @brief define a ring buffer with arbitrary size
 *
 * @see RING_BUF_ITEM_DECLARE_SIZE(),RING_BUF_DECLARE()
 */
RING_BUF_ITEM_DECLARE_SIZE(ringbuf_size, RINGBUFFER_SIZE);

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
static void tringbuf_put(void *p)
{
	int index = POINTER_TO_INT(p);
	/**TESTPOINT: ring buffer put*/
	int ret = ring_buf_item_put(pbuf, data[index].type, data[index].value,
				   data[index].buffer, data[index].length);

	zassert_equal(ret, 0, NULL);
}

static void tringbuf_get(void *p)
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

/*test cases*/
void test_ringbuffer_init(void)
{
	/**TESTPOINT: init via ring_buf_init*/
	ring_buf_init(&ringbuf, RINGBUFFER_SIZE, buffer);
	zassert_true(ring_buf_is_empty(&ringbuf), NULL);
	zassert_equal(ring_buf_space_get(&ringbuf), RINGBUFFER_SIZE - 1, NULL);
}

void test_ringbuffer_declare_pow2(void)
{
	zassert_true(ring_buf_is_empty(&ringbuf_pow2), NULL);
	zassert_equal(ring_buf_space_get(&ringbuf_pow2), (1 << POW) - 1, NULL);
}

void test_ringbuffer_declare_size(void)
{
	zassert_true(ring_buf_is_empty(&ringbuf_size), NULL);
	zassert_equal(ring_buf_space_get(&ringbuf_size), RINGBUFFER_SIZE - 1,
		      NULL);
}

void test_ringbuffer_put_get_thread(void)
{
	pbuf = &ringbuf;
	tringbuf_put((void *)0);
	tringbuf_put((void *)1);
	tringbuf_get((void *)0);
	tringbuf_get((void *)1);
	tringbuf_put((void *)2);
	zassert_false(ring_buf_is_empty(pbuf), NULL);
	tringbuf_get((void *)2);
	zassert_true(ring_buf_is_empty(pbuf), NULL);
}

void test_ringbuffer_put_get_isr(void)
{
	pbuf = &ringbuf;
	irq_offload(tringbuf_put, (void *)0);
	irq_offload(tringbuf_put, (void *)1);
	irq_offload(tringbuf_get, (void *)0);
	irq_offload(tringbuf_get, (void *)1);
	irq_offload(tringbuf_put, (void *)2);
	zassert_false(ring_buf_is_empty(pbuf), NULL);
	irq_offload(tringbuf_get, (void *)2);
	zassert_true(ring_buf_is_empty(pbuf), NULL);
}

void test_ringbuffer_put_get_thread_isr(void)
{
	pbuf = &ringbuf;
	tringbuf_put((void *)0);
	irq_offload(tringbuf_put, (void *)1);
	tringbuf_get((void *)0);
	irq_offload(tringbuf_get, (void *)1);
	tringbuf_put((void *)2);
	irq_offload(tringbuf_get, (void *)2);
}

void test_ringbuffer_pow2_put_get_thread_isr(void)
{
	pbuf = &ringbuf_pow2;
	tringbuf_put((void *)0);
	irq_offload(tringbuf_put, (void *)1);
	tringbuf_get((void *)0);
	irq_offload(tringbuf_get, (void *)1);
	tringbuf_put((void *)1);
	irq_offload(tringbuf_get, (void *)1);
}

void test_ringbuffer_size_put_get_thread_isr(void)
{
	pbuf = &ringbuf_size;
	tringbuf_put((void *)0);
	irq_offload(tringbuf_put, (void *)1);
	tringbuf_get((void *)0);
	irq_offload(tringbuf_get, (void *)1);
	tringbuf_put((void *)2);
	irq_offload(tringbuf_get, (void *)2);
}

/**
 * @brief verify data is passed between ring buffer and array
 *
 * @details The ringbuffer is defined and initialized.
 * put data of type u8_t to a ring buffer from inbuf array.
 * get data to outbuf array from ring buffer.
 * verify if the data put is equal to the data got.
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_put(), ring_buf_get(),#RING_BUF_DECLARE()
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
	zassert_equal(in_size, RINGBUFFER_SIZE - 1, NULL);

	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       1);
	zassert_equal(in_size, 0, NULL);

	out_size = ring_buf_get(&ringbuf_raw, outbuf,
					RINGBUFFER_SIZE);

	zassert_true(out_size == RINGBUFFER_SIZE - 1, NULL);

	out_size = ring_buf_get(&ringbuf_raw, outbuf,
					RINGBUFFER_SIZE + 1);
	zassert_true(out_size == 0, NULL);


}

void test_ringbuffer_alloc_put(void)
{
	uint8_t outputbuf[RINGBUFFER_SIZE];
	uint8_t inputbuf[] = {1, 2, 3, 4};
	uint32_t read_size;
	uint32_t allocated;
	uint32_t sum_allocated;
	uint8_t *data;
	int err;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buf.buf8);

	allocated = ring_buf_put_claim(&ringbuf_raw, &data, 1);
	sum_allocated = allocated;
	zassert_true(allocated == 1U, NULL);


	allocated = ring_buf_put_claim(&ringbuf_raw, &data,
					   RINGBUFFER_SIZE - 1);
	sum_allocated += allocated;
	zassert_true(allocated == RINGBUFFER_SIZE - 2, NULL);

	/* Putting too much returns error */
	err = ring_buf_put_finish(&ringbuf_raw, RINGBUFFER_SIZE);
	zassert_true(err != 0, NULL);

	err = ring_buf_put_finish(&ringbuf_raw, 1);
	zassert_true(err == 0, NULL);

	err = ring_buf_put_finish(&ringbuf_raw, RINGBUFFER_SIZE - 2);
	zassert_true(err == 0, NULL);

	read_size = ring_buf_get(&ringbuf_raw, outputbuf,
					     RINGBUFFER_SIZE - 1);
	zassert_true(read_size == (RINGBUFFER_SIZE - 1), NULL);

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

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buf.buf8);

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

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buf.buf8);

	/* capacity equals buffer size dedicated for ring buffer - 1 because
	 * 1 byte is used for distinguishing between full and empty state.
	 */
	capacity = ring_buf_capacity_get(&ringbuf_raw);
	zassert_equal(RINGBUFFER_SIZE - 1, capacity,
			"Unexpected capacity");
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

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE, ringbuf_raw.buf.buf8);

	len = 3;
	out_len = ring_buf_put(&ringbuf_raw, indata, len);
	zassert_equal(out_len, len, NULL);

	out_len = ring_buf_get(&ringbuf_raw, outdata, len);
	zassert_equal(out_len, len, NULL);

	space = ring_buf_space_get(&ringbuf_raw);
	zassert_equal(space, RINGBUFFER_SIZE - 1, NULL);

	/* Even though ringbuffer is empty, full buffer cannot be allocated
	 * because internal pointers are not at the beginning.
	 */
	granted = ring_buf_put_claim(&ringbuf_raw, &outbuf, RINGBUFFER_SIZE);
	zassert_false(granted == RINGBUFFER_SIZE - 1, NULL);

	/* After reset full buffer can be allocated. */
	ring_buf_reset(&ringbuf_raw);
	granted = ring_buf_put_claim(&ringbuf_raw, &outbuf, RINGBUFFER_SIZE);
	zassert_true(granted == RINGBUFFER_SIZE - 1, NULL);
}

#ifdef CONFIG_64BIT
static uint64_t ringbuf_stored[RINGBUFFER_SIZE];
#else
static uint32_t ringbuf_stored[RINGBUFFER_SIZE];
#endif

/**
 * @brief verify the array stored by ringbuf
 *
 * @details Define a buffer stored by ringbuffer
 * and keep that the buffer's size is always
 * equal to the pointer's size.
 * And also verify that the address of the buffer
 * is contiguous.
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_item_put(), ring_buf_item_get()
 */
void test_ringbuffer_array_perf(void)
{
	struct ring_buf buf_ii;
	uint32_t input[3] = {0xaa, 0xbb, 0xcc};
	uint32_t output[3] = {0};
	uint16_t type = 0;
	uint8_t value = 0, size = 3;
	void *tp;

	ring_buf_init(&buf_ii, RINGBUFFER_SIZE, ringbuf_stored);

	/*Data from the beginning of the array can be copied into the ringbuf*/
	zassert_true(ring_buf_item_put(&buf_ii, 1, 2, input, 3) == 0, NULL);

	/*Verify the address stored by ringbuf is contiguous*/
	for (int i = 0; i < 3; i++) {
		zassert_equal(input[i], buf_ii.buf.buf32[i+1], NULL);
	}

	/*Data from the end of the ringbuf can be copied into the array*/
	zassert_true(ring_buf_item_get(&buf_ii, &type, &value,
				output, &size) == 0, NULL);

	/*Verify the ringbuf defined is working*/
	for (int i = 0; i < 3; i++) {
		zassert_equal(input[i], output[i], NULL);
	}

	/*The size of array stored by ringbuf is equal to the size of pointer*/
	zassert_equal(sizeof(tp), sizeof(ringbuf_stored[0]), NULL);
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
			 ztest_unit_test(test_ringbuffer_pow2_put_get_thread_isr),
			 ztest_unit_test(test_ringbuffer_size_put_get_thread_isr),
			 ztest_unit_test(test_ringbuffer_array_perf),
			 ztest_unit_test(test_ring_buffer_main),
			 ztest_unit_test(test_ringbuffer_raw),
			 ztest_unit_test(test_ringbuffer_alloc_put),
			 ztest_unit_test(test_byte_put_free),
			 ztest_unit_test(test_byte_put_free),
			 ztest_unit_test(test_capacity),
			 ztest_unit_test(test_reset)
			 );
	ztest_run_test_suite(test_ringbuffer_api);
}
