/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define EVENT_OVERHEAD_XTAL_US        1500
#define EVENT_OVERHEAD_PREEMPT_US     0    /* if <= min, then dynamic preempt */
#define EVENT_OVERHEAD_PREEMPT_MIN_US 0
#define EVENT_OVERHEAD_PREEMPT_MAX_US EVENT_OVERHEAD_XTAL_US
#define EVENT_OVERHEAD_START_US       200
#define EVENT_JITTER_US               16
