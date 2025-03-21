/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/debug/cpu_load.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_backend.h>

#include <zephyr/drivers/counter.h>

#define DELTA 30

ZTEST(cpu_load, test_load)
{
	int load;
	uint32_t t_ms = 100;

	if (CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0) {
		cpu_load_log_control(false);
	}

	/* Reset the measurement */
	(void)cpu_load_get(true);
	k_busy_wait(t_ms * USEC_PER_MSEC);

	/* Measurement is not reset. */
	load = cpu_load_get(false);
	/* Result in per mille */
	zassert_within(load, 1000, DELTA);

	k_msleep(t_ms);
	load = cpu_load_get(false);
	zassert_within(load, 500, DELTA);

	/* Reset the measurement */
	(void)cpu_load_get(true);
	k_msleep(t_ms);
	load = cpu_load_get(false);
	zassert_within(load, 0, DELTA);
}

#if CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0
static int cpu_load_src_id;
static atomic_t log_cnt;

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	ARG_UNUSED(backend);
	const void *source = msg->log.hdr.source;
	int source_id = log_const_source_id((const struct log_source_const_data *)source);

	if (source_id == cpu_load_src_id) {
		atomic_inc(&log_cnt);
	}
}

static void init(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

const struct log_backend_api mock_log_backend_api = {
	.process = process,
	.init = init
};

LOG_BACKEND_DEFINE(dummy, mock_log_backend_api, false, NULL);

ZTEST(cpu_load, test_periodic_report)
{
	log_backend_enable(&dummy, NULL, LOG_LEVEL_INF);
	cpu_load_log_control(true);

	cpu_load_src_id = log_source_id_get(STRINGIFY(cpu_load));
	atomic_set(&log_cnt, 0);
	k_msleep(3 * CONFIG_CPU_LOAD_LOG_PERIODICALLY);
	zassert_within(log_cnt, 3, 1);

	cpu_load_log_control(false);
	k_msleep(1);
	atomic_set(&log_cnt, 0);
	k_msleep(3 * CONFIG_CPU_LOAD_LOG_PERIODICALLY);
	zassert_equal(log_cnt, 0);

	cpu_load_log_control(true);
	k_msleep(3 * CONFIG_CPU_LOAD_LOG_PERIODICALLY);
	zassert_within(log_cnt, 3, 1);

	cpu_load_log_control(false);
	log_backend_disable(&dummy);
}
#endif /* CONFIG_CPU_LOAD_LOG_PERIODICALLY > 0 */

ZTEST_SUITE(cpu_load, NULL, NULL, NULL, NULL, NULL);
