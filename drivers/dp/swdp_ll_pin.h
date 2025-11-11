/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>

static ALWAYS_INLINE void pin_delay_asm(uint32_t delay)
{
#if defined(CONFIG_CPU_CORTEX_M)
	__asm volatile (".syntax unified\n"
			".start_%=:\n"
			"subs %0, #1\n"
			"bne .start_%=\n"
			: "+l" (delay)
			:
			: "cc"
			);
#else
#warning "Pin delay is not defined"
#endif
}

#if defined(CONFIG_SOC_SERIES_NRF52X) || defined(CONFIG_SOC_SERIES_NRF53X)

#include "swdp_ll_pin_nrf.h"

#elif defined(CONFIG_SOC_FAMILY_STM32)

#include "swdp_ll_pin_stm32.h"

#else

#define CPU_CLOCK CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define FAST_BITBANG_HW_SUPPORT 0

static ALWAYS_INLINE void swdp_ll_pin_input(void *const base, uint8_t pin)
{
}

static ALWAYS_INLINE void swdp_ll_pin_output(void *const base, uint8_t pin)
{
}


static ALWAYS_INLINE void swdp_ll_pin_set(void *const base, uint8_t pin)
{
}

static ALWAYS_INLINE void swdp_ll_pin_clr(void *const base, uint8_t pin)
{
}

static ALWAYS_INLINE uint32_t swdp_ll_pin_get(void *const base, uint8_t pin)
{
	return 0UL;
}

#endif

#ifndef CPU_CLOCK
#error "CPU_CLOCK not defined by any soc specific driver"
#endif

#ifndef FAST_BITBANG_HW_SUPPORT
#error "FAST_BITBANG_HW_SUPPORT not defined by any soc specific driver"
#endif
