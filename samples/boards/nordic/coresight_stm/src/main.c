/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_LOG_FRONTEND_STMESP
#include <zephyr/logging/log_frontend_stmesp.h>
#endif

LOG_MODULE_REGISTER(app);

#define TEST_LOG(rpt, item)                                                                        \
	({                                                                                         \
		uint32_t key = irq_lock();                                                         \
		uint32_t t = k_cycle_get_32();                                                     \
		for (uint32_t i = 0; i < rpt; i++) {                                               \
			__DEBRACKET item;                                                          \
		}                                                                                  \
		t = k_cycle_get_32() - t;                                                          \
		irq_unlock(key);                                                                   \
		k_msleep(400);                                                                     \
		t;                                                                                 \
	})

static char *core_name = "unknown";

static void get_core_name(void)
{
	if (strstr(CONFIG_BOARD_TARGET, "cpuapp")) {
		core_name = "app";
	} else if (strstr(CONFIG_BOARD_TARGET, "cpurad")) {
		core_name = "rad";
	} else if (strstr(CONFIG_BOARD_TARGET, "cpuppr")) {
		core_name = "ppr";
	} else if (strstr(CONFIG_BOARD_TARGET, "cpuflpr")) {
		core_name = "flpr";
	}
}

static uint32_t t_to_ns(uint32_t t, uint32_t rpt, uint32_t freq)
{
	return (uint32_t)(((uint64_t)t * 1000000000) / (uint64_t)(rpt * freq));
}

static void timing_report(uint32_t t, uint32_t rpt, const char *str)
{
	uint32_t ns = t_to_ns(t, rpt, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	LOG_RAW("%s: Timing for %s: %d.%dus\n", core_name, str, ns / 1000, (ns % 1000) / 10);
}

int main(void)
{
	uint32_t t;
	uint32_t delta;
	uint32_t rpt = 10;
	uint32_t t0, t1, t2, t3, t_s;
	char str[] = "test string";

	get_core_name();

	t = k_cycle_get_32();
	delta = k_cycle_get_32() - t;

	t0 = TEST_LOG(rpt, (LOG_INF("test no arguments")));
	t0 -= delta;

	t1 = TEST_LOG(rpt, (LOG_INF("test with one argument %d", 100)));
	t1 -= delta;

	t2 = TEST_LOG(rpt, (LOG_INF("test with two arguments %d %d", 100, 10)));
	t2 -= delta;

	t3 = TEST_LOG(rpt, (LOG_INF("test with three arguments %d %d %d", 100, 10, 1)));
	t3 -= delta;

	t_s = TEST_LOG(rpt, (LOG_INF("test with string %s", str)));
	t_s -= delta;

#ifdef CONFIG_LOG_FRONTEND_STMESP
	uint32_t rpt_tp = 20;
	uint32_t t_tp, t_tpd;

	t_tp = TEST_LOG(rpt_tp, (log_frontend_stmesp_tp(5)));
	t_tp -= delta;

	t_tpd = TEST_LOG(rpt_tp, (log_frontend_stmesp_tp_d32(6, 10)));
	t_tpd -= delta;
#endif

	timing_report(t0, rpt, "log message with 0 arguments");
	timing_report(t1, rpt, "log message with 1 argument");
	timing_report(t2, rpt, "log message with 2 arguments");
	timing_report(t3, rpt, "log message with 3 arguments");
	timing_report(t_s, rpt, "log_message with string");

#ifdef CONFIG_LOG_FRONTEND_STMESP
	timing_report(t_tp, rpt_tp, "tracepoint");
	timing_report(t_tpd, rpt_tp, "tracepoint_d32");
#endif

	/* Needed in coverage run to separate STM logs from printk() */
	k_msleep(400);
	return 0;
}
