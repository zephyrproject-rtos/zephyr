/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/debug/watchpoint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

#include <errno.h>
#include <stdint.h>

struct hit_record {
	void *pc;
	enum k_watchpoint_timing timing;
	bool rearm_required;
};

static volatile uint8_t monitored_value;
static struct hit_record hit;
static atomic_t hit_count;

static void watchpoint_hit(const struct k_watchpoint *wp,
			   const struct k_watchpoint_event *event, void *arg)
{
	ARG_UNUSED(wp);
	ARG_UNUSED(arg);

	hit.pc = event->pc;
	hit.timing = event->timing;
	hit.rearm_required = event->rearm_required;
	atomic_inc(&hit_count);
}

static K_WATCHPOINT_DEFINE(wp, (void *)&monitored_value,
			   sizeof(monitored_value), K_WATCHPOINT_WRITE,
			   watchpoint_hit, NULL);

static const char *timing_name(enum k_watchpoint_timing timing)
{
	switch (timing) {
	case K_WATCHPOINT_TIMING_BEFORE:
		return "before";
	case K_WATCHPOINT_TIMING_AFTER:
		return "after";
	default:
		return "unknown";
	}
}

int main(void)
{
	int ret = k_watchpoint_add(&wp);

	if (ret != 0) {
		printk("Failed to arm watchpoint (%d)\n", ret);
		return ret;
	}

	monitored_value = 1U;

	if (atomic_get(&hit_count) != 1) {
		printk("Watchpoint did not fire exactly once\n");
		(void)k_watchpoint_remove(&wp);
		return -EIO;
	}

	ret = k_watchpoint_remove(&wp);
	if (ret != 0) {
		printk("Failed to disarm watchpoint (%d)\n", ret);
		return ret;
	}

	printk("Watchpoint hit: pc=%p timing=%s rearm_required=%s\n",
	       hit.pc, timing_name(hit.timing),
	       hit.rearm_required ? "yes" : "no");
	printk("Watchpoint sample complete\n");

	return 0;
}
