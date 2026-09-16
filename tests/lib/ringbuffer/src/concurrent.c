/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/random/random.h>
#include <stdint.h>

/**
 * @defgroup lib_ringbuffer_tests Ringbuffer
 * @ingroup all_tests
 * @{
 * @}
 */

#define PTR_RINGBUFFER			32

static ZTEST_BMEM struct ring_buf ptr_ringbuf;
static ZTEST_BMEM uint8_t ptr_buffer[PTR_RINGBUFFER];

static bool produce_ptr(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	static int cnt;
	static uint32_t wr = 8;
	uint32_t len;
	uint8_t *data;

	if (iter_cnt == 0) {
		cnt = 0;
		wr = 8;
	}

	len = ring_buf_put_ptr(&ptr_ringbuf, &data, 0);
	if (len == 0) {
		return true;
	}

	len = MIN(len, wr);
	for (uint32_t i = 0; i < len; i++) {
		data[i] = cnt++;
	}
	ring_buf_commit(&ptr_ringbuf, len);

	wr++;
	if (wr == 15) {
		wr = 8;
	}

	return true;
}

static bool consume_ptr(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	static int cnt;
	static uint32_t rd = 8;
	uint32_t len;
	uint8_t *data;

	if (iter_cnt == 0) {
		cnt = 0;
		rd = 8;
	}

	len = ring_buf_get_ptr(&ptr_ringbuf, &data, 0);
	if (len == 0) {
		return true;
	}

	len = MIN(len, rd);
	for (uint32_t i = 0; i < len; i++) {
		zassert_equal(data[i], (uint8_t)cnt,
			      "Got %02x, exp: %02x", data[i], (uint8_t)cnt);
		cnt++;
	}
	ring_buf_consume(&ptr_ringbuf, len);

	rd++;
	if (rd == 15) {
		rd = 8;
	}

	return true;
}

static void test_ptr_ztress(ztress_handler high_handler, ztress_handler low_handler)
{
	ring_buf_idx_t offset;
	k_timeout_t timeout;

	ring_buf_init(&ptr_ringbuf, sizeof(ptr_buffer), ptr_buffer);

	offset = 2 * ring_buf_capacity_get(&ptr_ringbuf) -
		 ring_buf_capacity_get(&ptr_ringbuf) / 2;
	ptr_ringbuf.read_idx = offset;
	ptr_ringbuf.write_idx = offset;

	timeout = (CONFIG_SYS_CLOCK_TICKS_PER_SEC < 10000) ? K_MSEC(1000) : K_MSEC(10000);
	ztress_set_timeout(timeout);
	ZTRESS_EXECUTE(ZTRESS_THREAD(high_handler, NULL, 0, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(low_handler, NULL, 0, 2000, Z_TIMEOUT_TICKS(20)));
}

ZTEST(ringbuffer_api, test_ringbuffer_ptr_stress)
{
	PRINT("Producing interrupts consuming\n");
	test_ptr_ztress(produce_ptr, consume_ptr);

	PRINT("Consuming interrupts producing\n");
	test_ptr_ztress(consume_ptr, produce_ptr);
}
