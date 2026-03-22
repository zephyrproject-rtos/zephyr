/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_BENCHMARK_SUITE(k_pipe, NULL, NULL);

K_PIPE_DEFINE(pipe, 128, 4);
static uint8_t data[128];

static void reset(void)
{
	k_pipe_reset(&pipe);
}

static void fill(void)
{
	int rc;

	rc = k_pipe_write(&pipe, data, sizeof(data), K_NO_WAIT);
	__ASSERT(rc == sizeof(data), "Failed to write to pipe: %d", rc);
}

ZTEST_BENCHMARK(k_pipe, buffered_write, 1000, reset, NULL)
{
	k_pipe_write(&pipe, data, 1, K_NO_WAIT);
}

ZTEST_BENCHMARK(k_pipe, buffered_read, 1000, fill, reset)
{
	k_pipe_read(&pipe, data, 1, K_NO_WAIT);
}

K_PIPE_DEFINE(dma_r, 0, 4);
static void reader_thread(void *p1, void *p2, void *p3)
{
	uint8_t rx;

	while (1) {
		k_pipe_read(&dma_r, &rx, sizeof(rx), K_FOREVER);
	}
}

K_THREAD_DEFINE(reader, 1024, reader_thread, NULL, NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY, 0, 0);

ZTEST_BENCHMARK(k_pipe, threaded_transfer_write, 1000, k_yield, k_yield)
{
	k_pipe_write(&dma_r, data, 1, K_FOREVER);
}

K_PIPE_DEFINE(dma_w, 0, 4);
static void writer_thread(void *p1, void *p2, void *p3)
{
	uint8_t tx = 0;

	while (1) {
		k_pipe_write(&dma_w, &tx, sizeof(tx), K_FOREVER);
	}
}

K_THREAD_DEFINE(writer, 1024, writer_thread, NULL, NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY, 0, 0);

ZTEST_BENCHMARK(k_pipe, threaded_transfer_read, 1000, k_yield, k_yield)
{
	k_pipe_read(&dma_w, data, 1, K_FOREVER);
}
