/* phil_task.c - dining philosophers */

/*
 * Copyright (c) 2011-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include "phil.h"

/**
 *
 * @brief Routine to start dining philosopher demo
 *
 */

void phil_demo(void)
{
	task_group_start(DP_PHI);
	task_group_start(DP_MON);

}
