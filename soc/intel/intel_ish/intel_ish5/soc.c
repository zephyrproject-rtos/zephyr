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

void soc_early_init_hook(void)
{
#if defined(CONFIG_HPET_TIMER)
	sedi_hpet_set_min_delay(HPET_CMP_MIN_DELAY);
#endif
}
