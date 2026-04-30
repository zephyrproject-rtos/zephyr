/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>

/* Ring buffer performance benchmarks. Each benchmark performs one full
 * put followed by one full get per sample, so the ring buffer is balanced
 * across iterations and never overflows or underflows.
 */
#define PERF_BUF_SIZE 16
#define PERF_SAMPLES  1000

static uint8_t perf_buf[PERF_BUF_SIZE];
static uint8_t perf_indata[PERF_BUF_SIZE];
static uint8_t perf_outdata[PERF_BUF_SIZE];
static struct ring_buf perf_rbuf;

static void perf_setup(void)
{
	ring_buf_init(&perf_rbuf, sizeof(perf_buf), perf_buf);
}

ZTEST_BENCHMARK_SUITE(ringbuffer_perf, NULL, NULL);

ZTEST_BENCHMARK(ringbuffer_perf, put_get_1byte, PERF_SAMPLES, perf_setup, NULL)
{
	ring_buf_put(&perf_rbuf, perf_indata, 1);
	ring_buf_get(&perf_rbuf, perf_outdata, 1);
}

ZTEST_BENCHMARK(ringbuffer_perf, put_get_4byte, PERF_SAMPLES, perf_setup, NULL)
{
	ring_buf_put(&perf_rbuf, perf_indata, 4);
	ring_buf_get(&perf_rbuf, perf_outdata, 4);
}

ZTEST_BENCHMARK(ringbuffer_perf, put_ptr_commit_1byte, PERF_SAMPLES, perf_setup, NULL)
{
	uint8_t *ptr;

	ring_buf_put_ptr(&perf_rbuf, &ptr);
	ring_buf_commit(&perf_rbuf, 1);
	ring_buf_get(&perf_rbuf, perf_outdata, 1);
}

ZTEST_BENCHMARK(ringbuffer_perf, put_ptr_commit_5byte, PERF_SAMPLES, perf_setup, NULL)
{
	uint8_t *ptr;

	ring_buf_put_ptr(&perf_rbuf, &ptr);
	ring_buf_commit(&perf_rbuf, 5);
	ring_buf_get(&perf_rbuf, perf_outdata, 5);
}

ZTEST_BENCHMARK(ringbuffer_perf, get_ptr_consume_5byte, PERF_SAMPLES, perf_setup, NULL)
{
	uint8_t *ptr;

	ring_buf_put(&perf_rbuf, perf_indata, 5);
	ring_buf_get_ptr(&perf_rbuf, &ptr);
	ring_buf_consume(&perf_rbuf, 5);
}
