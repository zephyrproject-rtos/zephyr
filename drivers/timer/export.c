/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/llext/symbol.h>
#include <limits.h>

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
EXPORT_SYMBOL(sys_clock_cycle_get_64);
#endif
