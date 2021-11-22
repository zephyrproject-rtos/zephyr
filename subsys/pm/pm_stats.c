/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pm_stats.h"

#include <autoconf.h>
#include <init.h>
#include <kernel_structs.h>
#include <stats/stats.h>
#include <sys/printk.h>

struct pm_cpu_timing {
	uint32_t timer_start;
	uint32_t timer_end;
};

static struct pm_cpu_timing pm_cpu_timings[CONFIG_MP_NUM_CPUS];

STATS_SECT_START(pm_cpu_stats)
STATS_SECT_ENTRY32(state_count)
STATS_SECT_ENTRY32(state_last_cycles)
STATS_SECT_ENTRY32(state_total_cycles)
STATS_SECT_END;

STATS_NAME_START(pm_cpu_stats)
STATS_NAME(pm_cpu_stats, state_count)
STATS_NAME(pm_cpu_stats, state_last_cycles)
STATS_NAME(pm_cpu_stats, state_total_cycles)
STATS_NAME_END(pm_cpu_stats);

#define PM_STAT_NAME_LEN sizeof("pm_cpu_XXX_state_X_stats")
static char pm_cpu_stat_names[CONFIG_MP_NUM_CPUS][PM_STATE_COUNT][PM_STAT_NAME_LEN];
static struct stats_pm_cpu_stats pm_cpu_stats[CONFIG_MP_NUM_CPUS][PM_STATE_COUNT];

static int pm_stats_init(const struct device *unused)
{
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		for (int j = 0; j < PM_STATE_COUNT; j++) {
			snprintk(pm_cpu_stat_names[i][j], PM_STAT_NAME_LEN,
				 "pm_cpu_%03d_state_%1d_stats", i, j);
			stats_init(&(pm_cpu_stats[i][j].s_hdr), STATS_SIZE_32, 3,
				   STATS_NAME_INIT_PARMS(pm_cpu_stats));
			stats_register(pm_cpu_stat_names[i][j], &(pm_cpu_stats[i][j].s_hdr));
		}
	}
	return 0;
}

SYS_INIT(pm_stats_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

void pm_stats_start(void)
{
	pm_cpu_timings[_current_cpu->id].timer_start = k_cycle_get_32();
}

void pm_stats_stop(void)
{
	pm_cpu_timings[_current_cpu->id].timer_end = k_cycle_get_32();
}

void pm_stats_update(enum pm_state state)
{
	uint8_t cpu = _current_cpu->id;
	uint32_t time_total =
		pm_cpu_timings[cpu].timer_end -
		pm_cpu_timings[cpu].timer_start;

	STATS_INC(pm_cpu_stats[cpu][state], state_count);
	STATS_INCN(pm_cpu_stats[cpu][state], state_total_cycles, time_total);
	STATS_SET(pm_cpu_stats[cpu][state], state_last_cycles, time_total);
}
