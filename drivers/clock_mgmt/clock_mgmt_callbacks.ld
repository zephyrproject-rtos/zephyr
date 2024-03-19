/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_mgmt_linker.h"

clock_callbacks_start = .;
/* Link all used clock management callback structures here */
DT_FOREACH_CLOCK_ID(CLOCK_CALLBACK_LD_SECTION)
clock_callbacks_end = .;
