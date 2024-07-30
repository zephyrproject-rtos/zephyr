/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pm_stats.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/stats/stats.h>
#include <zephyr/sys/printk.h>

STATS_SECT_START(pm_stats)
STATS_SECT_ENTRY32(state_count)
STATS_SECT_ENTRY32(state_last_cycles)
STATS_SECT_ENTRY32(state_total_cycles)
STATS_SECT_END;

STATS_NAME_START(pm_stats)
STATS_NAME(pm_stats, state_count)
STATS_NAME(pm_stats, state_last_cycles)
STATS_NAME(pm_stats, state_total_cycles)
STATS_NAME_END(pm_stats);

static STATS_SECT_DECL(pm_stats) stats[CONFIG_MP_MAX_NUM_CPUS][PM_STATE_COUNT];

#define PM_STAT_NAME_LEN sizeof("pm_cpu_XXX_state_X_stats")
static char names[CONFIG_MP_MAX_NUM_CPUS][PM_STATE_COUNT][PM_STAT_NAME_LEN];
static uint32_t time_start[CONFIG_MP_MAX_NUM_CPUS];
static uint32_t time_stop[CONFIG_MP_MAX_NUM_CPUS];

static int pm_stats_init(void)
{

	unsigned int num_cpus = arch_num_cpus();

	for (uint8_t i = 0U; i < num_cpus; i++) {
		for (uint8_t j = 0U; j < PM_STATE_COUNT; j++) {
			snprintk(names[i][j], PM_STAT_NAME_LEN,
				 "pm_cpu_%03d_state_%1d_stats", i, j);
			stats_init(&(stats[i][j].s_hdr), STATS_SIZE_32, 3U,
				   STATS_NAME_INIT_PARMS(pm_stats));
			stats_register(names[i][j], &(stats[i][j].s_hdr));
		}
	}

	return 0;
}

SYS_INIT(pm_stats_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void pm_stats_start(void)
{
	time_start[_current_cpu->id] = k_cycle_get_32();
}

void pm_stats_stop(void)
{
	time_stop[_current_cpu->id] = k_cycle_get_32();
}

void pm_stats_update(enum pm_state state)
{
	uint8_t cpu = _current_cpu->id;
	uint32_t time_total = time_stop[cpu] - time_start[cpu];

	STATS_INC(stats[cpu][state], state_count);
	STATS_INCN(stats[cpu][state], state_total_cycles, time_total);
	STATS_SET(stats[cpu][state], state_last_cycles, time_total);
}
