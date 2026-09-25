/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#if CONFIG_SOC_SERIES_TC3X
#define STM_TIM0       0x10
#define STM_CMP0       0x30
#define STM_CMCON      0x38
#define STM_ICR        0x3c
#define STM_ISCR       0x40
#define STM_TIM0SV     0x50
#define STM_CAPSV      0x54
#define STM_OCS        0xe8
#define STM_ICR_CMP0OS BIT(2)
#define STM_IRQ_IDX    0
#define CBS_OSTATE     0xf0000480
#elif CONFIG_SOC_SERIES_TC4X
#define STM_ABS             0x20
#define STM_OCS             0x04
#define STM_CMP0            0x120
#define STM_CMCON           0x128
#define STM_ICR             0x12c
#define STM_ISCR            0x130
#define STM_ICR_CMP0OS      BIT(1)
#define STM_IRQ_IDX         1
#define CBS_OSTATE          0xfa18006c
#endif
#define STM_ICR_CMP0EN         BIT(0)
#define STM_ISCR_CMP0IRR       BIT(0)
#define STM_CMCON_MSIZE0_MASK  GENMASK(4, 0)
#define STM_CMCON_MSTART0_MASK GENMASK(12, 8)
#define STM_OCS_SUS_MASK       GENMASK(27, 24)
#define STM_OCS_SUS_P          BIT(28)
#define CBS_OSTATE_OEN         BIT(0)
/* OCDS suspend control: SUS = 2 (suspend), SUS_P = 1 (write protect) */
#define STM_OCS_FREEZE         (FIELD_PREP(STM_OCS_SUS_MASK, 2) | STM_OCS_SUS_P)

#define STM_IRQ DT_IRQN_BY_IDX(DT_NODELABEL(stm), STM_IRQ_IDX)

BUILD_ASSERT(IS_ENABLED(CONFIG_SOC_SERIES_TC3X) || IS_ENABLED(CONFIG_SOC_SERIES_TC4X),
	     "aurix_stm requires CONFIG_SOC_SERIES_TC3X or CONFIG_SOC_SERIES_TC4X");

/* STM register block base */
#define STM_BASE ((mem_addr_t)DT_REG_ADDR(DT_NODELABEL(stm)))
/* Test value */
#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = STM_IRQ;
#endif

/*
 * The STM is a free-running 64-bit counter with a 32-bit CMP0 comparator that
 * fires on an equality match of the low 32 bits (MSIZE0 = 31, MSTART0 = 0). A
 * target written after the counter has passed it is missed until the low word
 * wraps, so this is the exact-match backend: the generic core rewrites CMP0
 * through its verify loop instead of the driver applying a minimum-delay floor.
 * The counter runs at the fstm clock, which is the kernel system clock rate, so
 * no rate override is needed.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH    64
/* CMP0 is a 32-bit comparator: a single arm reaches at most 2^32 cycles. */
#define TIMER_CORE_ALARM_MAX_CYCLES UINT32_MAX
/* Keep the cheap single-read 32-bit cycle getter (see below). */
#define TIMER_CORE_HAVE_CYCLE_GET_32

static uint64_t timer_driver_cycle_get(void)
{
#if CONFIG_SOC_SERIES_TC3X
	/* Reading TIM0SV latches STM[63:32] into CAPSV, so read it first. */
	uint32_t lo = sys_read32(STM_BASE + STM_TIM0SV);
	uint32_t hi = sys_read32(STM_BASE + STM_CAPSV);

	return (uint64_t)lo | ((uint64_t)hi << 32);
#elif CONFIG_SOC_SERIES_TC4X
	return sys_read64(STM_BASE + STM_ABS);
#else
	return 0;
#endif
}

static void timer_driver_set_compare(uint64_t cycles)
{
	sys_write32((uint32_t)cycles, STM_BASE + STM_CMP0);
}

#include "system_timer_generic.h"

uint32_t sys_clock_cycle_get_32(void)
{
#if CONFIG_SOC_SERIES_TC3X
	return sys_read32(STM_BASE + STM_TIM0);
#elif CONFIG_SOC_SERIES_TC4X
	return (uint32_t)sys_read64(STM_BASE + STM_ABS);
#else
	return 0;
#endif
}

static void set_compare_irq(bool enabled)
{
	uint32_t icr = sys_read32(STM_BASE + STM_ICR);

	icr = enabled ? (icr | STM_ICR_CMP0EN) : (icr & ~STM_ICR_CMP0EN);
	sys_write32(icr, STM_BASE + STM_ICR);
}

void sys_clock_disable(void)
{
	set_compare_irq(false);
}

static void sys_clock_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Clear the compare match flag, then account and rearm. */
	sys_write32(STM_ISCR_CMP0IRR, STM_BASE + STM_ISCR);
	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(STM_IRQ, 1, sys_clock_isr, NULL, 0);

#if DT_PROP(DT_NODELABEL(stm), freeze)
	/* Set debug freeze if selected and debugger connected */
	if (sys_read32(CBS_OSTATE) & CBS_OSTATE_OEN) {
		sys_write32(STM_OCS_FREEZE, STM_BASE + STM_OCS);
	}
#endif
	/* Set compare window: MSIZE0 = 31 (compare CMP0[31:0]), MSTART0 = 0 */
	sys_write32(FIELD_PREP(STM_CMCON_MSIZE0_MASK, 31) | FIELD_PREP(STM_CMCON_MSTART0_MASK, 0),
		    STM_BASE + STM_CMCON);
	/* Route compare match to service request STMIR0 */
	sys_write32(sys_read32(STM_BASE + STM_ICR) & ~STM_ICR_CMP0OS, STM_BASE + STM_ICR);
	/* Clear irq */
	sys_write32(STM_ISCR_CMP0IRR, STM_BASE + STM_ISCR);

	/* Seed the announce baseline and arm the first compare. */
	timer_core_init();

	set_compare_irq(true);
	irq_enable(STM_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
