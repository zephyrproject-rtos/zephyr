/*
 * zyclictest.c: Zephyr cyclictest
 *
 * Copyright (c) 2025 Andreas Klinger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zyclictest, LOG_LEVEL_INF);

#include <zephyr/posix/unistd.h>
#include <zephyr/shell/shell.h>

#include <zephyr/sys/atomic.h>

#ifndef CONFIG_NATIVE_LIBC
extern void getopt_init(void);
#endif

#define ZYC_MAX_HIST 1000

#define ZYC_INT 0
#define ZYC_THR 1

#define MICRO_SEC 1000000ULL

static struct {
	uint64_t start_ticks;
	int64_t thr_ticks;
	int64_t interval_us;
	uint64_t start_cycle;
	uint64_t irq_cycle;
	uint64_t thr_cycle;
	uint32_t loops;
	int thr_prio;
	int max_lat[2];
	int cnt_ovl[2];
	int lat[2][ZYC_MAX_HIST];
	int quiet_mode;
	atomic_t cnt_lat;
	const struct shell *sh;
} zyc_data;

static struct k_timer zyc_timer;

static atomic_t zyc_running;
static K_KERNEL_STACK_DEFINE(zyc_stack_area, 1024);
static k_tid_t zyc_tid;
static struct k_thread zyc_ctl;

static uint64_t zyc_cyc_sec;

static void zyclictest_thr(void *d0, void *d1, void *d2)
{
	int status, i;
	uint64_t clat[2];

	LOG_DBG("Zyclictest thread started ticks/sec: %d hw-cycles: %llu",
		CONFIG_SYS_CLOCK_TICKS_PER_SEC, zyc_cyc_sec);

	while (atomic_get(&zyc_running)) {
		status = k_timer_status_sync(&zyc_timer);
		if (status <= 0) {
			k_sleep(K_USEC(zyc_data.interval_us / 4));
			continue;
		}
		zyc_data.thr_cycle = k_cycle_get_64();

		clat[ZYC_INT] =
			(zyc_data.irq_cycle - zyc_data.start_cycle) * MICRO_SEC / zyc_cyc_sec;
		clat[ZYC_THR] =
			(zyc_data.thr_cycle - zyc_data.start_cycle) * MICRO_SEC / zyc_cyc_sec;

		zyc_data.start_cycle += zyc_cyc_sec * zyc_data.interval_us / MICRO_SEC;

		atomic_inc(&zyc_data.cnt_lat);
		if (zyc_data.loops && atomic_get(&zyc_data.cnt_lat) >= zyc_data.loops) {
			atomic_set(&zyc_running, 0);
		}

		for (i = 0; i < 2; i++) {
			if (clat[i] > zyc_data.max_lat[i]) {
				zyc_data.max_lat[i] = clat[i];
			}
			if (clat[i] < ZYC_MAX_HIST) {
				zyc_data.lat[i][clat[i]]++;
			} else {
				zyc_data.cnt_ovl[i]++;
			}
		}
		LOG_DBG("sta: %d, irq-lat: %llu thr-lat: %llu", status, clat[ZYC_INT],
			clat[ZYC_THR]);
	}

	shell_print(zyc_data.sh, "Finished thread with count: %ld", atomic_get(&zyc_data.cnt_lat));

	LOG_DBG("Bye-bye from the zyclictest thread");
}

static void zyclictest_handler(struct k_timer *timer_id)
{
	zyc_data.irq_cycle = k_cycle_get_64();
}

static int zyclictest_init(void)
{
	int ret;

	if (zyc_tid) {
		LOG_WRN("zyclictest already running");
		return -EBUSY;
	}

	zyc_cyc_sec = sys_clock_hw_cycles_per_sec();
	zyc_data.start_cycle = k_cycle_get_64() + zyc_cyc_sec * zyc_data.interval_us / MICRO_SEC;

	k_timer_init(&zyc_timer, zyclictest_handler, NULL);
	k_timer_start(&zyc_timer, K_USEC(zyc_data.interval_us), K_USEC(zyc_data.interval_us));

	atomic_set(&zyc_running, 1);
	zyc_tid =
		k_thread_create(&zyc_ctl, zyc_stack_area, K_KERNEL_STACK_SIZEOF(zyc_stack_area),
				zyclictest_thr, NULL, NULL, NULL, zyc_data.thr_prio, 0, K_NO_WAIT);

	if (zyc_tid != &zyc_ctl) {
		LOG_ERR("Error while creating zyclictest thread");
		return -ENOMEM;
	}

	ret = k_thread_name_set(zyc_tid, "zyclictest");
	if (ret) {
		LOG_ERR("Error while setting zyclictest thread name: %d", ret);
	}

	return 0;
}

static int zyclictest_exit(void)
{
	int ret;

	atomic_set(&zyc_running, 0);

	ret = k_thread_join(zyc_tid, K_FOREVER);
	if (ret) {
		LOG_ERR("Error while terminating zyclictest thread: %d", ret);
	}

	zyc_tid = NULL;

	k_timer_stop(&zyc_timer);

	return 0;
}

static int zyclictest_print_summary(const struct shell *sh)
{
	int max_lat;
	int min_int_lat, min_thr_lat;
	uint64_t sum_int_lat = 0, sum_thr_lat = 0;
	int avg_int_lat, avg_thr_lat;
	long cnt;

	max_lat = MIN(zyc_data.max_lat[ZYC_THR], ZYC_MAX_HIST - 1);
	min_int_lat = min_thr_lat = max_lat;
	for (int i = 0; i <= max_lat; i++) {
		if (i < min_int_lat && zyc_data.lat[ZYC_INT][i] > 0) {
			min_int_lat = i;
		}
		if (i < min_thr_lat && zyc_data.lat[ZYC_THR][i] > 0) {
			min_thr_lat = i;
		}
		sum_int_lat += zyc_data.lat[ZYC_INT][i] * i;
		sum_thr_lat += zyc_data.lat[ZYC_THR][i] * i;
	}

	cnt = atomic_get(&zyc_data.cnt_lat);

	avg_int_lat = (sum_int_lat + cnt / 2) / cnt;
	avg_thr_lat = (sum_thr_lat + cnt / 2) / cnt;

	shell_print(sh, "            \t   IRQ\tThread");
	shell_print(sh, "Min-Latency:\t%6d\t%6d us", min_int_lat, min_thr_lat);
	shell_print(sh, "Avg-Latency:\t%6d\t%6d us", avg_int_lat, avg_thr_lat);
	shell_print(sh, "Max-Latency:\t%6d\t%6d us", zyc_data.max_lat[ZYC_INT],
		    zyc_data.max_lat[ZYC_THR]);
	shell_print(sh, "Overflow:   \t%6d\t%6d", zyc_data.cnt_ovl[ZYC_INT],
		    zyc_data.cnt_ovl[ZYC_THR]);

	if (!zyc_data.quiet_mode) {
		shell_print(sh, "\nHistogram:");

		for (int i = 0; i <= max_lat; i++) {
			shell_print(sh, "%3d         \t%6d\t%6d", i, zyc_data.lat[ZYC_INT][i],
				    zyc_data.lat[ZYC_THR][i]);
		}
	}

	return 0;
}

static int cmd_zyclictest_start(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int rv;

#ifndef CONFIG_NATIVE_LIBC
	getopt_init();
#endif

	zyc_data.sh = sh;
	zyc_data.thr_prio = -CONFIG_NUM_COOP_PRIORITIES;
	zyc_data.interval_us = 1000LL;
	atomic_set(&zyc_data.cnt_lat, 0);
	zyc_data.loops = 0;
	memset(zyc_data.max_lat, 0, sizeof(zyc_data.max_lat));
	memset(zyc_data.cnt_ovl, 0, sizeof(zyc_data.cnt_ovl));
	memset(zyc_data.lat, 0, sizeof(zyc_data.lat));

	while ((rv = getopt(argc, argv, "i:l:p:")) != -1) {
		switch (rv) {
		case 'i':
			ret = 0;
			zyc_data.interval_us = shell_strtoull(optarg, 0, &ret);
			if (ret) {
				shell_error(sh, "invalid interval '%s' ret: %d", optarg, ret);
				return -EINVAL;
			}
			break;
		case 'l':
			ret = 0;
			zyc_data.loops = shell_strtoul(optarg, 0, &ret);
			if (ret) {
				shell_error(sh, "invalid loop '%s' ret: %d", optarg, ret);
				return -EINVAL;
			}
			break;
		case 'p':
			ret = 0;
			zyc_data.thr_prio = shell_strtol(optarg, 0, &ret);
			if ((ret) || (zyc_data.thr_prio < -CONFIG_NUM_COOP_PRIORITIES) ||
			    (zyc_data.thr_prio > CONFIG_NUM_PREEMPT_PRIORITIES)) {
				shell_error(sh, "invalid priority '%s' ret: %d", optarg, ret);
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	shell_print(sh, "Cycle interval: %llu us", zyc_data.interval_us);
	shell_print(sh, "Priority: %d", zyc_data.thr_prio);

	ret = zyclictest_init();
	if (ret) {
		LOG_ERR("Error while initializing zyclictest: %d", ret);
	}

	return 0;
}

static int cmd_zyclictest_stop(const struct shell *sh, size_t argc, char **argv)
{
	int rv;
	int cancel = 0;

	zyc_data.quiet_mode = 0;

#ifndef CONFIG_NATIVE_LIBC
	getopt_init();
#endif

	while ((rv = getopt(argc, argv, "cq")) != -1) {
		switch (rv) {
		case 'c':
			cancel = 1;
			break;
		case 'q':
			zyc_data.quiet_mode = 1;
			break;
		default:
			return -EINVAL;
		}
	}

	if (!cancel && zyc_data.loops && atomic_get(&zyc_running)) {
		shell_print(sh, "Zyclictest still running: %ld / %u",
			    atomic_get(&zyc_data.cnt_lat), zyc_data.loops);
		return 1;
	}

	zyclictest_exit();

	zyclictest_print_summary(sh);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_zyclictest,
	SHELL_CMD_ARG(start, NULL, SHELL_HELP("Start Zyclictest",
		      "[-i <interval-us>] [-l <loops>] [-p <prio>]"),
		      cmd_zyclictest_start, 1, 6),
	SHELL_CMD_ARG(stop, NULL, SHELL_HELP("Stop Zyclictest",
		      "[-c] [-q]"),
		      cmd_zyclictest_stop, 1, 2),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(zyclictest, &sub_zyclictest, "Zephyr cyclictest latency measurement", NULL);
