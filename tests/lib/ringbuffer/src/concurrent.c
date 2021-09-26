/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <sys/ring_buffer.h>
#include <sys/mutex.h>
#include <random/rand32.h>

/**
 * @defgroup lib_ringbuffer_tests Ringbuffer
 * @ingroup all_tests
 * @{
 * @}
 */

#define STACKSIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

#define RINGBUFFER			256
#define LENGTH				64
#define VALUE				0xb
#define TYPE				0xc

#define RINGBUFFER_API_ITEM  0
#define RINGBUFFER_API_CPY   1
#define RINGBUFFER_API_NOCPY 2

static K_THREAD_STACK_DEFINE(thread_low_stack, STACKSIZE);
static struct k_thread thread_low_data;
static K_THREAD_STACK_DEFINE(thread_high_stack, STACKSIZE);
static struct k_thread thread_high_data;

static ZTEST_BMEM SYS_MUTEX_DEFINE(mutex);
RING_BUF_ITEM_DECLARE_SIZE(ringbuf, RINGBUFFER);
static uint32_t output[LENGTH];
static uint32_t databuffer1[LENGTH];
static uint32_t databuffer2[LENGTH];

static volatile int preempt_cnt;
static volatile bool in_task;

typedef void (*test_ringbuf_action_t)(struct ring_buf *rbuf, bool reset);
static test_ringbuf_action_t produce_fn;
static test_ringbuf_action_t consume_fn;
volatile int test_microdelay_cnt;

static void data_write(uint32_t *input)
{
	sys_mutex_lock(&mutex, K_FOREVER);
	int ret = ring_buf_item_put(&ringbuf, TYPE, VALUE,
				   input, LENGTH);
	zassert_equal(ret, 0, NULL);
	sys_mutex_unlock(&mutex);
}

static void data_read(uint32_t *output)
{
	uint16_t type;
	uint8_t value, size32 = LENGTH;
	int ret;

	sys_mutex_lock(&mutex, K_FOREVER);
	ret = ring_buf_item_get(&ringbuf, &type, &value, output, &size32);
	sys_mutex_unlock(&mutex);

	zassert_equal(ret, 0, NULL);
	zassert_equal(type, TYPE, NULL);
	zassert_equal(value, VALUE, NULL);
	zassert_equal(size32, LENGTH, NULL);
	if (output[0] == 1) {
		zassert_equal(memcmp(output, databuffer1, size32), 0, NULL);
	} else {
		zassert_equal(memcmp(output, databuffer2, size32), 0, NULL);
	}
}

static void thread_entry_t1(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < LENGTH; i++) {
		databuffer1[i] = 1;
	}

	/* Try to write data into the ringbuffer */
	data_write(databuffer1);
	/* Try to get data from the ringbuffer and check */
	data_read(output);
}

static void thread_entry_t2(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < LENGTH; i++) {
		databuffer2[i] = 2;
	}
	/* Try to write data into the ringbuffer */
	data_write(databuffer2);
	/* Try to get data from the ringbuffer and check */
	data_read(output);
}

/**
 * @brief Test that prevent concurrent writing
 * operations by using a mutex
 *
 * @details Define a ring buffer and a mutex,
 * and then spawn two threads to read and
 * write the same buffer at the same time to
 * check the integrity of data reading and writing.
 *
 * @ingroup lib_ringbuffer_tests
 */
void test_ringbuffer_concurrent(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int prio = 10;

	k_thread_priority_set(k_current_get(), prio);

	k_thread_create(&thread_high_data, thread_high_stack, STACKSIZE,
			thread_entry_t1,
			NULL, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);
	k_thread_create(&thread_low_data, thread_low_stack, STACKSIZE,
			thread_entry_t2,
			NULL, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	/* Wait for thread exiting */
	k_thread_join(&thread_low_data, K_FOREVER);
	k_thread_join(&thread_high_data, K_FOREVER);


	/* Revert priority of the main thread */
	k_thread_priority_set(k_current_get(), old_prio);
}

static void produce_cpy(struct ring_buf *rbuf, bool reset)
{
	static int cnt;
	uint8_t buf[3];
	uint32_t len;

	if (reset) {
		cnt = 0;
		return;
	}

	for (int i = 0; i < sizeof(buf); i++) {
		buf[i] = (uint8_t)cnt++;
	}

	len = ring_buf_put(rbuf, buf, sizeof(buf));
	cnt -= (sizeof(buf) - len);
}

static void consume_cpy(struct ring_buf *rbuf, bool reset)
{
	static int cnt;
	uint8_t buf[3];
	uint32_t len;

	if (reset) {
		cnt = 0;
		return;
	}

	len = ring_buf_get(rbuf, buf, sizeof(buf));
	for (int i = 0; i < len; i++) {
		zassert_equal(buf[i], (uint8_t)cnt, NULL);
		cnt++;
	}
}

static void produce_item(struct ring_buf *rbuf, bool reset)
{
	int err;
	static uint16_t cnt;
	uint32_t buf[2];

	if (reset) {
		cnt = 0;
		return;
	}

	err = ring_buf_item_put(rbuf, cnt++, VALUE, buf, 2);
	(void)err;
}

static void consume_item(struct ring_buf *rbuf, bool reset)
{
	int err;
	static uint16_t cnt;
	uint32_t data[2];
	uint16_t type;
	uint8_t value;
	uint8_t size32 = ARRAY_SIZE(data);

	if (reset) {
		cnt = 0;
		return;
	}

	err = ring_buf_item_get(rbuf, &type, &value, data, &size32);
	if (err == 0) {
		zassert_equal(type, cnt++, NULL);
		zassert_equal(value, VALUE, NULL);
	} else if (err == -EMSGSIZE) {
		zassert_true(false, NULL);
	}
}

static void produce(struct ring_buf *rbuf, bool reset)
{
	static int cnt;
	static int wr = 8;
	uint32_t len;
	uint8_t *data;

	if (reset) {
		cnt = 0;
		return;
	}

	len = ring_buf_put_claim(rbuf, &data, wr);
	if (len == 0) {
		len = ring_buf_put_claim(rbuf, &data, wr);
	}

	if (len == 0) {
		return;
	}

	for (uint32_t i = 0; i < len; i++) {
		data[i] = cnt++;
	}

	wr++;
	if (wr == 15) {
		wr = 8;
	}

	int err = ring_buf_put_finish(rbuf, len);

	zassert_equal(err, 0, "cnt: %d", cnt);
}

static void consume(struct ring_buf *rbuf, bool reset)
{
	static int rd = 8;
	static int cnt;
	uint32_t len;
	uint8_t *data;

	if (reset) {
		cnt = 0;
		return;
	}

	len = ring_buf_get_claim(rbuf, &data, rd);
	if (len == 0) {
		len = ring_buf_get_claim(rbuf, &data, rd);
	}

	if (len == 0) {
		return;
	}

	for (uint32_t i = 0; i < len; i++) {
		zassert_equal(data[i], (uint8_t)cnt,
			      "Got %02x, exp: %02x", data[i], (uint8_t)cnt);
		cnt++;
	}

	rd++;
	if (rd == 15) {
		rd = 8;
	}

	int err = ring_buf_get_finish(rbuf, len);

	zassert_equal(err, 0, NULL);
}


static void produce_timeout(struct k_timer *timer)
{
	struct ring_buf *rbuf = k_timer_user_data_get(timer);

	if (in_task) {
		preempt_cnt++;
	}

	produce_fn(rbuf, false);
}

static void consume_timeout(struct k_timer *timer)
{
	struct ring_buf *rbuf = k_timer_user_data_get(timer);

	if (in_task) {
		preempt_cnt++;
	}

	consume_fn(rbuf, false);
}

static void microdelay(int delay)
{
	for (int i = 0; i < delay; i++) {
		test_microdelay_cnt++;
	}
}

/* Test is running 2 parts of ring buffer operations (producing, consuming) in
 * two different contexts. One is the thread context and second is k_timer
 * timeout interrupt which can preempt thread. The goal of this test is to
 * provoke cases when one operation is preempted by another at multiple locations.
 * It is achieved by starting a timer and then busywaiting for similar time
 * before starting an operation in the thread context. Number of thread context
 * preemptions is counted and test is considered valid if certain amount of
 * preemptions occurred.
 *
 * Ring buffer claims that it is thread safe and requires no additional locking
 * in single producer, single consumer case and this test aims to prove that.
 *
 * Depending on input parameter @p p2 thread context is used for producing or
 * consuming.
 */
static void thread_entry_spsc(void *p1, void *p2, void *p3)
{
	struct ring_buf *rbuf = p1;
	uint32_t timeout = 6000;
	bool high_producer = (bool)p2;
	uint32_t start = k_uptime_get_32();
	struct k_timer timer;
	int i = 0;
	int backoff_us = MAX(100, 3 * (1000000 / CONFIG_SYS_CLOCK_TICKS_PER_SEC));
	k_timeout_t t = K_USEC(backoff_us);

	k_timer_init(&timer,
		     high_producer ? produce_timeout : consume_timeout,
		     NULL);
	k_timer_user_data_set(&timer, rbuf);

	preempt_cnt = 0;
	consume_fn(rbuf, true);
	produce_fn(rbuf, true);

	while (k_uptime_get_32() < (start + timeout)) {
		int r = sys_rand32_get() % 200;

		k_timer_start(&timer, t, K_NO_WAIT);
		k_busy_wait(backoff_us - 50 + i);
		microdelay(r);

		in_task = true;
		if (high_producer) {
			consume_fn(rbuf, false);
		} else {
			produce_fn(rbuf, false);
		}
		in_task = false;

		i++;
		if (i > 60) {
			i = 0;
		}

		k_timer_status_sync(&timer);
	}

	PRINT("preempted: %d\n", preempt_cnt);
	/* Test is tailored for qemu_x86 to generate enough number of preemptions
	 * to validate that ring buffer is safe to be used without any locks in
	 * single producer single consumer scenario.
	 */
	if (IS_ENABLED(CONFIG_BOARD_QEMU_X86)) {
		zassert_true(preempt_cnt > 1500, "If thread operation was not preempted "
			"multiple times then we cannot have confidance that it "
			"validated the module properly. Platform should not be "
			"used in that case");
	}
}

extern uint32_t test_rewind_threshold;

/* Single producer, single consumer test */
static void test_ringbuffer_spsc(bool higher_producer, int api_type)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int prio = 10;
	uint32_t old_rewind_threshold = test_rewind_threshold;
	uint8_t buf[32];
	uint32_t buf32[32];

	if (CONFIG_SYS_CLOCK_TICKS_PER_SEC < 100000) {
		ztest_test_skip();
	}

	test_rewind_threshold = 64;

	switch (api_type) {
	case RINGBUFFER_API_ITEM:
		ring_buf_init(&ringbuf, ARRAY_SIZE(buf32), buf32);
		consume_fn = consume_item;
		produce_fn = produce_item;
		break;
	case RINGBUFFER_API_NOCPY:
		ring_buf_init(&ringbuf, ARRAY_SIZE(buf), buf);
		consume_fn = consume;
		produce_fn = produce;
		break;
	case RINGBUFFER_API_CPY:
		ring_buf_init(&ringbuf, ARRAY_SIZE(buf), buf);
		consume_fn = consume_cpy;
		produce_fn = produce_cpy;
		break;
	default:
		zassert_true(false, NULL);
	}

	k_thread_priority_set(k_current_get(), prio);

	k_thread_create(&thread_high_data, thread_high_stack, STACKSIZE,
			thread_entry_spsc,
			&ringbuf, (void *)higher_producer, NULL,
			prio + 1, 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));

	/* Wait for thread exiting */
	k_thread_join(&thread_high_data, K_FOREVER);


	/* Revert priority of the main thread */
	k_thread_priority_set(k_current_get(), old_prio);
	test_rewind_threshold = old_rewind_threshold;
}

/* Zero-copy API. Test is validating single producer, single consumer where
 * producer has higher priority context which can preempt consumer.
 */
void test_ringbuffer_shpsc(void)
{
	test_ringbuffer_spsc(true, RINGBUFFER_API_NOCPY);
}

/* Zero-copy API. Test is validating single producer, single consumer where
 * consumer has higher priority context which can preempt producer.
 */
void test_ringbuffer_spshc(void)
{
	test_ringbuffer_spsc(false, RINGBUFFER_API_NOCPY);
}

/* Copy API. Test is validating single producer, single consumer where
 * producer has higher priority context which can preempt consumer.
 */
void test_ringbuffer_cpy_shpsc(void)
{
	test_ringbuffer_spsc(true, RINGBUFFER_API_CPY);
}

/* Copy API. Test is validating single producer, single consumer where
 * consumer has higher priority context which can preempt producer.
 */
void test_ringbuffer_cpy_spshc(void)
{
	test_ringbuffer_spsc(false, RINGBUFFER_API_CPY);
}
/* Item API. Test is validating single producer, single consumer where producer
 * has higher priority context which can preempt consumer.
 */
void test_ringbuffer_item_shpsc(void)
{
	test_ringbuffer_spsc(true, RINGBUFFER_API_ITEM);
}

/* Item API. Test is validating single producer, single consumer where consumer
 * has higher priority context which can preempt producer.
 */
void test_ringbuffer_item_spshc(void)
{
	test_ringbuffer_spsc(false, RINGBUFFER_API_ITEM);
}
