/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(spi_context_test);

#include "spi_context.h"

struct test_data {
	struct spi_context ctx;
};

static struct test_data data = {
	SPI_CONTEXT_INIT_SYNC(data, ctx),
};

static struct spi_config cfg = {
	.operation = SPI_OP_MODE_CONTROLLER | SPI_WORD_SET(8),
};

static void transfer_done(struct k_timer *timer)
{
	spi_context_complete(&data.ctx, NULL, 0);
}

K_TIMER_DEFINE(transfer_timer, transfer_done, NULL);

static uint32_t wire_time_ms(size_t len, uint32_t frequency)
{
	return (uint32_t)((uint64_t)len * 8U * 1000U / frequency);
}

static int wait_for_transfer(size_t len, uint32_t frequency, bool complete)
{
	cfg.frequency = frequency;
	data.ctx.config = &cfg;
	data.ctx.max_count = len;

	if (complete) {
		k_timer_start(&transfer_timer, K_MSEC(wire_time_ms(len, frequency)), K_NO_WAIT);
	}

	return spi_context_wait_for_completion(&data.ctx);
}

static void spi_context_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&data.ctx.sync);
}

static void spi_context_after(void *fixture)
{
	ARG_UNUSED(fixture);

	k_timer_stop(&transfer_timer);
}

ZTEST(spi_context, test_wait_above_32bit_wrap)
{
	zassert_ok(wait_for_transfer(536871U, 10000000U, true));
}

ZTEST(spi_context, test_wait_above_two_32bit_wraps)
{
	zassert_ok(wait_for_transfer(1200000U, 40000000U, true));
}

ZTEST(spi_context, test_wait_times_out_after_wire_time)
{
	uint32_t min_ms =
		wire_time_ms(768000U, 10000000U) + CONFIG_SPI_COMPLETION_TIMEOUT_TOLERANCE;
	int64_t start = k_uptime_get();

	zassert_equal(wait_for_transfer(768000U, 10000000U, false), -ETIMEDOUT);
	zassert_true(k_uptime_get() - start >= min_ms);
}

ZTEST_SUITE(spi_context, NULL, NULL, spi_context_before, spi_context_after, NULL);
