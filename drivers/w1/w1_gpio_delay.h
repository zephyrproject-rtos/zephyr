/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for software driven 1-Wire using GPIO lines
 *
 * This driver implements an 1-Wire interface by driving two GPIO lines under
 * software control. The GPIO pins used must be preconfigured to to a suitable
 * state, i.e. the DQ pin as open-colector/open-drain with a pull-up resistor
 * (possibly as an external component attached to the pin). When the DQ pin
 * is read it must return the state of the physical hardware line, not just the
 * last state written to it for output.
 */

#include <device.h>
#include <errno.h>
#include <gpio.h>
#include <misc/printk.h>
#include <w1.h>
#include "w1_internal.h"

#ifdef CONFIG_SOC_FAMILY_NRF

#include "soc/nrfx_coredep.h"

/**
 * Time delay for all the w1_ functions
 *
 * If nRF51x soc used we are using built in 16Mhz timer instead of built in
 * ticks which are too slow for microsecond timimgs
 *
 * @param context [description]
 * @param delay   delay in microseconds
 */
static inline void w1_delay(u32_t delay)
{
	/* 6us is compensation */
	u32_t adjusted_delay = delay > 6 ? delay - 6 : 0;

	nrfx_coredep_delay_us(adjusted_delay);
};

#else

#define NS_TO_SYS_CLOCK_HW_CYCLES(us) \
	((u64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC * (us) / USEC_PER_SEC)

/**
 * Time delay for all the w1_ function
 *
 * @param context [description]
 * @param delay   delay in microseconds
 */
static inline void w1_delay(u32_t delay)
{
	u32_t start = k_cycle_get_32();
	unsigned int cycles_to_wait = NS_TO_SYS_CLOCK_HW_CYCLES(delay);

	while (k_cycle_get_32() - start < cycles_to_wait) {
	}
};
#endif
