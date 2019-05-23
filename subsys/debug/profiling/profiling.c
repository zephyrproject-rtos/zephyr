/*
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <zephyr/types.h>
#include <logging/log.h>
#include <profiling.h>

#include "profiling_priv.h"

LOG_MODULE_REGISTER(profiling, CONFIG_PROFILING_LOG_LEVEL);

int dbg_prof_init(void)
{
	return 0;
}

int dbg_prof_task_init(struct dbg_prof_task *task)
{
	DBG_PROF_RST_CYCCNT;
	task->count = 0;

	return 0;
}

int dbg_prof_task_start(struct dbg_prof_task *task)
{
	/* Reset backend counter. */
	DBG_PROF_RST_CYCCNT;

	if (task->count) {
		LOG_INF("restarted %s: %d ticks",
			task->name, (unsigned int)task->count);

	} else {
		LOG_INF("started %s", task->name);
	}

	return 0;
}

int dbg_prof_task_stop(struct dbg_prof_task *task)
{
	/* Read backend counter and append value to count. */
	task->count += DWT->CYCCNT;

	LOG_INF("stopped %s: %d ticks",
		task->name, (unsigned int)task->count);

	return 0;
}

int dbg_prof_task_update(struct dbg_prof_task *task)
{
	u32_t cur = DWT->CYCCNT;

	LOG_INF("%s: %d ticks", task->name, (unsigned int)task->count + cur);

	return 0;
}

int dbg_prof_task_reset(struct dbg_prof_task *task)
{
	/* Reset backend counter. */
	DBG_PROF_RST_CYCCNT;
	task->count = 0;

	LOG_INF("reset %s", task->name);

	return 0;
}
