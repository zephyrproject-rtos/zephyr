/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE
#include <zephyr/device.h>
#include <zephyr/random/random.h>

#ifdef CONFIG_HPET_TIMER
#include "sedi_driver_hpet.h"

#define HPET_USE_CUSTOM_REG_ACCESS_FUNCS

/* COUNTER_CLK_PERIOD (CLK_PERIOD_REG) is in picoseconds (1e-12 sec) */
#define HPET_COUNTER_CLK_PERIOD		(1000000000000ULL)

#define HPET_CMP_MIN_DELAY		(5)

__pinned_func
static inline void hpet_timer_comparator_set(uint64_t next)
{
	sedi_hpet_set_comparator(HPET_0, next);
}

#endif  /*CONFIG_HPET_TIMER */

#endif  /* !_ASMLANGUAGE */

/* ISH specific DMA channel direction */
#define IMR_TO_MEMORY (DMA_CHANNEL_DIRECTION_PRIV_START)
#define MEMORY_TO_IMR (DMA_CHANNEL_DIRECTION_PRIV_START + 1)

#endif /* __SOC_H_ */
