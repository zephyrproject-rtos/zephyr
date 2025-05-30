/*
 * Copyright (c) 2018 Intel Corporation.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_PM_PM_STATS_H_
#define ZEPHYR_SUBSYS_PM_PM_STATS_H_

#include <zephyr/pm/state.h>

void pm_stats_start(void);
void pm_stats_stop(void);
void pm_stats_update(enum pm_state state);

#endif /* ZEPHYR_SUBSYS_PM_PM_STATS_H_ */
