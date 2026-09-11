/*
 * Copyright (c) 2026 Binho LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_device_registers.h>

/*
 * The core clock, not the kernel tick source.
 *
 * The generic branch of swdp_ll_pin.h defines CPU_CLOCK as
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, which on this SoC is the 1 MHz tick
 * source and not the CPU frequency. pin_delay_asm() burns *CPU* cycles, so
 * that value sizes MAX_SWJ_CLOCK() at (1 MHz / 2) / 4 = 125 kHz: every clock
 * a host can ask for lands above the threshold, sw_set_clock() pins
 * fast_clock with delay = 1, and swdp_set_clock() becomes inert.
 */
#define CPU_CLOCK               DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#define FAST_BITBANG_HW_SUPPORT 1

/*
 * PSOR and PCOR are write-1-to-set and write-1-to-clear, so a pin moves with a
 * single store and no read-modify-write, which is the point of this header.
 * PDDR carries the direction (1 = output) and PIDR the input buffer (1 =
 * disabled), so reading a pin needs both cleared and the turnaround on a shared
 * SWDIO stays correct.
 *
 * PIDR only exists on the MCX version of this GPIO block. Classic Kinetis parts
 * (and the MCXC variants that inherit their GPIO) have no such register, which
 * is why this header is selected per SoC family rather than by GPIO compatible.
 */

static ALWAYS_INLINE void swdp_ll_pin_input(void *const base, uint8_t pin)
{
	GPIO_Type *reg = base;

	reg->PIDR &= ~BIT(pin);
	reg->PDDR &= ~BIT(pin);
}

static ALWAYS_INLINE void swdp_ll_pin_output(void *const base, uint8_t pin)
{
	GPIO_Type *reg = base;

	reg->PDDR |= BIT(pin);
}

static ALWAYS_INLINE void swdp_ll_pin_set(void *const base, uint8_t pin)
{
	GPIO_Type *reg = base;

	reg->PSOR = BIT(pin);
}

static ALWAYS_INLINE void swdp_ll_pin_clr(void *const base, uint8_t pin)
{
	GPIO_Type *reg = base;

	reg->PCOR = BIT(pin);
}

static ALWAYS_INLINE uint32_t swdp_ll_pin_get(void *const base, uint8_t pin)
{
	GPIO_Type *reg = base;

	return (reg->PDIR >> pin) & 1UL;
}
