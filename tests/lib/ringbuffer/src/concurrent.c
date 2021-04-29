/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <sys/ring_buffer.h>
#include <sys/mutex.h>

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

static K_THREAD_STACK_DEFINE(thread_low_stack, STACKSIZE);
static struct k_thread thread_low_data;
static K_THREAD_STACK_DEFINE(thread_high_stack, STACKSIZE);
static struct k_thread thread_high_data;

static ZTEST_BMEM SYS_MUTEX_DEFINE(mutex);
RING_BUF_ITEM_DECLARE_SIZE(ringbuf, RINGBUFFER);
static uint32_t output[LENGTH];
static uint32_t databuffer1[LENGTH];
static uint32_t databuffer2[LENGTH];

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
