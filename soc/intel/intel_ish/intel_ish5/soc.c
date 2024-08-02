/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "soc.h"

#if defined(CONFIG_HPET_TIMER)
#include "sedi_driver_hpet.h"
#endif

static int intel_ish_init(void)
{
#if defined(CONFIG_HPET_TIMER)
	sedi_hpet_set_min_delay(HPET_CMP_MIN_DELAY);
#endif

	return 0;
}

SYS_INIT(intel_ish_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
