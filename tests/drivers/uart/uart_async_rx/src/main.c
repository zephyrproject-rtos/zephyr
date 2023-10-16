/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/serial/uart_async_rx.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

static void mem_fill(uint8_t *buf, uint8_t init, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = init + i;
	}
}

static bool mem_check(uint8_t *buf, uint8_t init, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (buf[i] != init + i) {
			return false;
		}
	}

	return true;
}

ZTEST(uart_async_rx, test_rx)
{
	int err;
	uint8_t buf[40];
	static const int buf_cnt = 4;
	size_t aloc_len;
	size_t claim_len;
	uint8_t *claim_buf;
	uint8_t *aloc_buf;
	struct uart_async_rx async_rx;
	const struct uart_async_rx_config config = {
		.buffer = buf,
		.length = sizeof(buf),
		.buf_cnt = buf_cnt
	};

	err = uart_async_rx_init(&async_rx, &config);
	zassert_equal(err, 0);

	aloc_len = uart_async_rx_get_buf_len(&async_rx);
	aloc_buf = uart_async_rx_buf_req(&async_rx);

	mem_fill(aloc_buf, 0, aloc_len - 2);

	/* No data to read. */
	claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 1);
	zassert_equal(claim_len, 0);

	/* Simulate partial write */
	uart_async_rx_on_rdy(&async_rx, aloc_buf, aloc_len - 4);

	/* There is at least 1 byte available */
	claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 1);
	zassert_equal(claim_len, 1);
	zassert_equal(claim_buf, aloc_buf);
	zassert_true(mem_check(claim_buf, 0, 1));

	/* All received data is available */
	claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 100);
	zassert_equal(claim_len, aloc_len - 4);
	zassert_equal(claim_buf, aloc_buf);
	zassert_true(mem_check(claim_buf, 0, aloc_len - 4));

	/* Simulate 2 bytes received to the same buffer. */
	uart_async_rx_on_rdy(&async_rx, aloc_buf, 2);

	/* Indicate and of the current buffer. */
	uart_async_rx_on_buf_rel(&async_rx, aloc_buf);

	/* Claim all data received so far */
	claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 100);
	zassert_equal(claim_len, aloc_len - 2);
	zassert_equal(claim_buf, aloc_buf);
	zassert_true(mem_check(claim_buf, 0, aloc_len - 2));

	/* Consume first 2 bytes. */
	uart_async_rx_data_consume(&async_rx, 2);

	/* Now claim will return buffer taking into account that first 2 bytes are
	 * consumed.
	 */
	claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 100);
	zassert_equal(claim_len, aloc_len - 4);
	zassert_equal(claim_buf, &aloc_buf[2]);
	zassert_true(mem_check(claim_buf, 2, aloc_len - 4));

	/* Consume rest of data. Get indication that it was end of the buffer. */
	uart_async_rx_data_consume(&async_rx, aloc_len - 4);
}

ZTEST(uart_async_rx, test_rx_late_consume)
{
	int err;
	uint8_t buf[40] __aligned(4);
	static const int buf_cnt = 4;
	size_t aloc_len;
	size_t claim_len;
	uint8_t *claim_buf;
	uint8_t *aloc_buf;
	struct uart_async_rx async_rx;
	const struct uart_async_rx_config config = {
		.buffer = buf,
		.length = sizeof(buf),
		.buf_cnt = buf_cnt
	};

	err = uart_async_rx_init(&async_rx, &config);
	zassert_equal(err, 0);

	aloc_len = uart_async_rx_get_buf_len(&async_rx);
	for (int i = 0; i < buf_cnt; i++) {
		aloc_buf = uart_async_rx_buf_req(&async_rx);

		aloc_buf[0] = (uint8_t)i;
		uart_async_rx_on_rdy(&async_rx, aloc_buf, 1);
		uart_async_rx_on_buf_rel(&async_rx, aloc_buf);
	}

	for (int i = 0; i < buf_cnt; i++) {
		claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 100);
		zassert_equal(claim_len, 1);
		zassert_equal(claim_buf[0], (uint8_t)i);

		uart_async_rx_data_consume(&async_rx, 1);
	}

	claim_len = uart_async_rx_data_claim(&async_rx, &claim_buf, 100);
	zassert_equal(claim_len, 0);
}

struct test_async_rx {
	struct uart_async_rx async_rx;
	atomic_t pending_req;
	atomic_t total_pending_req;
	bool in_chunks;
	uint8_t exp_consume;
	uint32_t byte_cnt;
	uint8_t curr_len;
	uint8_t *curr_buf;
	uint8_t *next_buf;
	struct k_spinlock lock;
};

static bool producer_no_chunks(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct test_async_rx *test_data = (struct test_async_rx *)user_data;
	struct uart_async_rx *async_rx = &test_data->async_rx;
	uint32_t r = sys_rand32_get();
	uint32_t len = MAX(1, MIN(uart_async_rx_get_buf_len(async_rx), r & 0x7));

	if (test_data->curr_buf) {

		for (int i = 0; i < len; i++) {
			test_data->curr_buf[i] = (uint8_t)test_data->byte_cnt;
			test_data->byte_cnt++;
		}
		uart_async_rx_on_rdy(async_rx, test_data->curr_buf, len);
		uart_async_rx_on_buf_rel(async_rx, test_data->curr_buf);
		test_data->curr_buf = test_data->next_buf;
		test_data->next_buf = NULL;

		uint8_t *buf = uart_async_rx_buf_req(async_rx);

		if (buf) {
			if (test_data->curr_buf == NULL) {
				test_data->curr_buf = buf;
			} else {
				test_data->next_buf = buf;
			}
		} else {
			atomic_inc(&test_data->pending_req);
			atomic_inc(&test_data->total_pending_req);
		}
	}

	return true;
}

static bool consumer(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct test_async_rx *test_data = (struct test_async_rx *)user_data;
	struct uart_async_rx *async_rx = &test_data->async_rx;
	uint32_t r = sys_rand32_get();
	uint32_t rpt = MAX(1, r & 0x7);

	r >>= 3;

	for (uint32_t i = 0; i < rpt; i++) {
		size_t claim_len = MAX(1, r & 0x7);
		size_t len;
		uint8_t *buf;

		r >>= 3;
		len = uart_async_rx_data_claim(async_rx, &buf, claim_len);

		if (len == 0) {
			return true;
		}

		for (int j = 0; j < len; j++) {
			zassert_equal(buf[j], test_data->exp_consume,
					"%02x (exp:%02x) len:%d, total:%d",
					buf[j], test_data->exp_consume, len, test_data->byte_cnt);
			test_data->exp_consume++;
		}

		uart_async_rx_data_consume(async_rx, len);

		if (test_data->pending_req) {
			buf = uart_async_rx_buf_req(async_rx);
			if (buf) {
				atomic_dec(&test_data->pending_req);
			}
			k_spinlock_key_t key = k_spin_lock(&test_data->lock);

			if (test_data->curr_buf == NULL) {
				test_data->curr_buf = buf;
			} else if (test_data->next_buf == NULL) {
				test_data->next_buf = buf;
			} else {
				zassert_true(false);
			}
			k_spin_unlock(&test_data->lock, key);
		}
	}

	return true;
}

static bool producer_in_chunks(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct test_async_rx *test_data = (struct test_async_rx *)user_data;
	struct uart_async_rx *async_rx = &test_data->async_rx;
	uint32_t r = sys_rand32_get();
	uint32_t rem = uart_async_rx_get_buf_len(async_rx) - test_data->curr_len;
	uint32_t len = MAX(1, MIN(uart_async_rx_get_buf_len(async_rx), r & 0x7));

	len = MIN(rem, len);

	if (test_data->curr_buf) {
		for (int i = 0; i < len; i++) {
			test_data->curr_buf[test_data->curr_len + i] = (uint8_t)test_data->byte_cnt;
			test_data->byte_cnt++;
		}
		uart_async_rx_on_rdy(async_rx, test_data->curr_buf, len);
		test_data->curr_len += len;

		if ((test_data->curr_len == uart_async_rx_get_buf_len(async_rx)) || (r & BIT(31))) {
			test_data->curr_len = 0;
			uart_async_rx_on_buf_rel(async_rx, test_data->curr_buf);

			test_data->curr_buf = test_data->next_buf;
			test_data->next_buf = NULL;

			uint8_t *buf = uart_async_rx_buf_req(async_rx);

			if (buf) {
				if (test_data->curr_buf == NULL) {
					test_data->curr_buf = buf;
				} else {
					test_data->next_buf = buf;
				}
			} else {
				atomic_inc(&test_data->pending_req);
			}
		}
	}

	return true;
}

static void stress_test(bool in_chunks)
{
	int err;
	uint8_t buf[40];
	static const int buf_cnt = 4;
	int preempt = 1000;
	int timeout = 5000;
	struct test_async_rx test_data;
	const struct uart_async_rx_config config = {
		.buffer = buf,
		.length = sizeof(buf),
		.buf_cnt = buf_cnt
	};

	memset(&test_data, 0, sizeof(test_data));

	err = uart_async_rx_init(&test_data.async_rx, &config);
	zassert_equal(err, 0);

	test_data.in_chunks = in_chunks;
	test_data.curr_buf = uart_async_rx_buf_req(&test_data.async_rx);

	ztress_set_timeout(K_MSEC(timeout));

	ZTRESS_EXECUTE(ZTRESS_THREAD(in_chunks ? producer_in_chunks : producer_no_chunks,
				&test_data, 0, 0, Z_TIMEOUT_TICKS(20)),
		       ZTRESS_THREAD(consumer, &test_data, 0, preempt, Z_TIMEOUT_TICKS(20)));

	TC_PRINT("total bytes: %d\n", test_data.byte_cnt);
	ztress_set_timeout(K_NO_WAIT);
}

ZTEST(uart_async_rx, test_rx_ztress_no_chunks)
{
	stress_test(false);
}

ZTEST(uart_async_rx, test_rx_ztress_with_chunks)
{
	stress_test(true);
}

ZTEST_SUITE(uart_async_rx, NULL, NULL, NULL, NULL, NULL);
