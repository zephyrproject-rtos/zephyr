/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#define SCHED_COOP 0
#define SCHED_PREMPT 1

extern int sched_get_priority_min(int policy);
extern int sched_get_priority_max(int policy);
extern int sched_yield(void);

