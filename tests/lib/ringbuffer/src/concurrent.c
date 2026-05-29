/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <stdint.h>

/**
 * @defgroup lib_ringbuffer_tests Ringbuffer
 * @ingroup all_tests
 * @{
 * @}
 */
static struct ring_buf *stress_rb;
static bool produce_cpy(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	static int cnt;
	uint8_t buf[3];
	uint32_t len;

	if (iter_cnt == 0) {
		cnt = 0;
	}

	for (int i = 0; i < sizeof(buf); i++) {
		buf[i] = (uint8_t)cnt++;
	}

	len = ring_buf_put(stress_rb, buf, sizeof(buf));
	cnt -= (sizeof(buf) - len);

	return true;
}

static bool consume_cpy(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	static int cnt;
	uint8_t buf[3];
	uint32_t len;

	if (iter_cnt == 0) {
		cnt = 0;
	}

	len = ring_buf_get(stress_rb, buf, sizeof(buf));
	for (int i = 0; i < len; i++) {
		zassert_equal(buf[i], (uint8_t)cnt);
		cnt++;
	}

	return true;
}

static bool produce(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	static int cnt;
	static int wr = 8;
	uint32_t avail;
	uint32_t len;
	uint8_t *data;

	if (iter_cnt == 0) {
		cnt = 0;
	}

	avail = ring_buf_put_ptr(stress_rb, &data);
	if (avail == 0) {
		return true;
	}

	len = MIN(avail, (uint32_t)wr);
	for (uint32_t i = 0; i < len; i++) {
		data[i] = cnt++;
	}

	wr++;
	if (wr == 15) {
		wr = 8;
	}

	ring_buf_commit(stress_rb, len);

	return true;
}

static bool consume(void *user_data, uint32_t iter_cnt, bool last, int prio)
{
	static int rd = 8;
	static int cnt;
	uint32_t avail;
	uint32_t len;
	uint8_t *data;

	if (iter_cnt == 0) {
		cnt = 0;
	}

	avail = ring_buf_get_ptr(stress_rb, &data);
	if (avail == 0) {
		return true;
	}

	len = MIN(avail, (uint32_t)rd);
	for (uint32_t i = 0; i < len; i++) {
		zassert_equal(data[i], (uint8_t)cnt, "Got %02x, exp: %02x", data[i], (uint8_t)cnt);
		cnt++;
	}

	rd++;
	if (rd == 15) {
		rd = 8;
	}

	ring_buf_consume(stress_rb, len);

	return true;
}

static void test_ztress(ztress_handler high_handler,
			ztress_handler low_handler)
{
	static uint8_t buf[32];
	static struct ring_buf rb;
	k_timeout_t timeout;

	ring_buf_init(&rb, sizeof(buf), buf);
	stress_rb = &rb;

	timeout = (CONFIG_SYS_CLOCK_TICKS_PER_SEC < 10000) ? K_MSEC(1000) : K_MSEC(10000);

	ztress_set_timeout(timeout);
	ZTRESS_EXECUTE(ZTRESS_THREAD(high_handler, NULL, 0, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(low_handler, NULL, 0, 2000, Z_TIMEOUT_TICKS(20)));
}

void test_ringbuffer_stress(ztress_handler produce_handler,
			    ztress_handler consume_handler)
{
	PRINT("Producing interrupts consuming\n");
	test_ztress(produce_handler, consume_handler);

	PRINT("Consuming interrupts producing\n");
	test_ztress(consume_handler, produce_handler);
}

/* Zero-copy API. Test is validating single producer, single consumer from
 * different priorities.
 */
ZTEST(ringbuffer_api, test_ringbuffer_zerocpy_stress)
{
	test_ringbuffer_stress(produce, consume);
}

/* Copy API. Test is validating single producer, single consumer from
 * different priorities.
 */
ZTEST(ringbuffer_api, test_ringbuffer_cpy_stress)
{
	test_ringbuffer_stress(produce_cpy, consume_cpy);
}
