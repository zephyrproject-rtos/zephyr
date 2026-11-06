/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* LIFO operations, for comparison against the FIFO of the same shape. */

#include "bench.h"

ZTEST_BENCHMARK_SUITE(lifo, NULL, NULL);

static K_LIFO_DEFINE(lifo);

static struct {
	intptr_t reserved;
	uint32_t payload;
} item;

static void lifo_fill(void)
{
	k_lifo_put(&lifo, &item);
}

static void lifo_drain(void)
{
	(void)k_lifo_get(&lifo, K_NO_WAIT);
}

ZTEST_BENCHMARK(lifo, put, BENCH_SAMPLES, NULL, lifo_drain)
{
	k_lifo_put(&lifo, &item);
}

ZTEST_BENCHMARK(lifo, get, BENCH_SAMPLES, lifo_fill, NULL)
{
	(void)k_lifo_get(&lifo, K_NO_WAIT);
}
