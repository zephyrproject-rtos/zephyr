/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AMD Xilinx AXI Timer driver for Zephyr system clock
 *
 * Uses timer 1 as a free-running clocksource and timer 0 as the
 * clockevent: periodic (auto-reload) when tickless kernel support is
 * disabled, or one-shot (reprogrammed on each interrupt) when tickless
 * operation is enabled.
 *
 * If more than one amd,xps-timer-1.00.a node is enabled, only the first
 * (DT_INST(0, ...)) is used as the Zephyr system clock; any others are
 * left untouched by this driver.
 */

#include <inttypes.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(amd_axi_timer);

#define DT_DRV_COMPAT amd_xps_timer_1_00_a

/* Use the first enabled AXI Timer node as the Zephyr system clock. */
#define TIMER_NODE DT_INST(0, DT_DRV_COMPAT)
#define TIMER_BASE ((mem_addr_t)DT_REG_ADDR(TIMER_NODE))

/* Register definitions */
#define XTC_TCSR_OFFSET 0 /* Control/Status register */
#define XTC_TLR_OFFSET  4 /* Load register */
#define XTC_TCR_OFFSET  8 /* Timer counter register */

/* Control status register mask */
#define XTC_CSR_INT_OCCURRED_MASK BIT(8)
#define XTC_CSR_ENABLE_TMR_MASK   BIT(7)
#define XTC_CSR_ENABLE_INT_MASK   BIT(6)
#define XTC_CSR_LOAD_MASK         BIT(5)
#define XTC_CSR_AUTO_RELOAD_MASK  BIT(4)
#define XTC_CSR_DOWN_COUNT_MASK   BIT(1)

/* Offset of second timer */
#define TIMER_REG_OFFSET 0x10

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(TIMER_NODE);
#endif

static uint32_t clocksource_offset;
static uint32_t clockevent_offset;

static inline uint32_t amd_axi_timer_read32(uint32_t timer_offset, uint32_t offset)
{
	mem_addr_t reg = TIMER_BASE + timer_offset + offset;

	LOG_DBG("%s: 0x%" PRIxPTR " (timer_offset = 0x%x, offset = 0x%x)", __func__, reg,
		timer_offset, offset);

	return sys_read32(reg);
}

static inline void amd_axi_timer_write32(uint32_t timer_offset, uint32_t value, uint32_t offset)
{
	mem_addr_t reg = TIMER_BASE + timer_offset + offset;

	LOG_DBG("%s: 0x%" PRIxPTR " (timer_offset = 0x%x, offset = 0x%x)", __func__, reg,
		timer_offset, offset);

	sys_write32(value, reg);
}

static void amd_axi_timer_clear_interrupt(void)
{
	uint32_t tcsr = amd_axi_timer_read32(clockevent_offset, XTC_TCSR_OFFSET);

	amd_axi_timer_write32(clockevent_offset, tcsr | XTC_CSR_INT_OCCURRED_MASK, XTC_TCSR_OFFSET);
}

static uint32_t amd_axi_timer_read_hw_cycle_count(void)
{
	return amd_axi_timer_read32(clocksource_offset, XTC_TCR_OFFSET);
}

static void amd_axi_timer_clockevent_program(uint32_t load_cycles)
{
	const uint32_t off = clockevent_offset;
	uint32_t tcsr;

	tcsr = amd_axi_timer_read32(off, XTC_TCSR_OFFSET);
	tcsr &= ~(uint32_t)XTC_CSR_ENABLE_TMR_MASK;
	amd_axi_timer_write32(off, tcsr, XTC_TCSR_OFFSET);

	amd_axi_timer_clear_interrupt();

	amd_axi_timer_write32(off, load_cycles, XTC_TLR_OFFSET);

	/* One-shot down counter: no auto-reload (tickless reprogramming). */
	tcsr = XTC_CSR_ENABLE_INT_MASK | XTC_CSR_DOWN_COUNT_MASK;
	amd_axi_timer_write32(off, tcsr | XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	amd_axi_timer_write32(off, tcsr | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);
}

/*
 * Generic tickless timer core backend:
 * - timer 1: free-running counter (timer_driver_cycle_get)
 * - timer 0: one-shot down counter (timer_driver_set_reload)
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 32

static uint32_t timer_driver_cycle_get(void)
{
	return amd_axi_timer_read_hw_cycle_count();
}

static void timer_driver_set_reload(uint32_t cycles)
{
	amd_axi_timer_clockevent_program(cycles);
}

#include "system_timer_generic.h"

static void amd_axi_timer_isr(const void *arg)
{
	const k_spinlock_key_t key = sys_clock_lock();

	ARG_UNUSED(arg);

	amd_axi_timer_clear_interrupt();
	timer_core_announce_from(key);
}

static void amd_axi_timer_initialize(void)
{
	for (uint8_t counter_number = 0; counter_number < 2; counter_number++) {
		uint32_t reg_offset = counter_number * TIMER_REG_OFFSET;

		amd_axi_timer_write32(reg_offset, 0, XTC_TLR_OFFSET);
		amd_axi_timer_write32(reg_offset, XTC_CSR_INT_OCCURRED_MASK | XTC_CSR_LOAD_MASK,
				      XTC_TCSR_OFFSET);
		amd_axi_timer_write32(reg_offset, 0, XTC_TCSR_OFFSET);
	}
}

static inline void amd_axi_timer_set_options(uint8_t timer_offset, uint32_t options)
{
	amd_axi_timer_write32(timer_offset, options, XTC_TCSR_OFFSET);
}

static void amd_axi_timer_start_clocksource(void)
{
	uint32_t tcsr = amd_axi_timer_read32(clocksource_offset, XTC_TCSR_OFFSET);

	amd_axi_timer_write32(clocksource_offset, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	amd_axi_timer_write32(clocksource_offset, tcsr | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);
}

static void amd_axi_timer_start_periodic_clockevent(void)
{
	uint32_t tcsr;

	amd_axi_timer_write32(clockevent_offset, TIMER_CORE_CYC_PER_TICK, XTC_TLR_OFFSET);
	amd_axi_timer_set_options(clockevent_offset, XTC_CSR_ENABLE_INT_MASK |
							     XTC_CSR_AUTO_RELOAD_MASK |
							     XTC_CSR_DOWN_COUNT_MASK);

	tcsr = amd_axi_timer_read32(clockevent_offset, XTC_TCSR_OFFSET);
	amd_axi_timer_write32(clockevent_offset, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	amd_axi_timer_write32(clockevent_offset, tcsr | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);

	amd_axi_timer_start_clocksource();
}

static int sys_clock_driver_init(void)
{
	LOG_DBG("Timer init at base 0x%" PRIxPTR, (mem_addr_t)TIMER_BASE);

	amd_axi_timer_initialize();

	clockevent_offset = 0;
	clocksource_offset = TIMER_REG_OFFSET;

	amd_axi_timer_set_options(clocksource_offset, XTC_CSR_AUTO_RELOAD_MASK);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		amd_axi_timer_start_clocksource();
	} else {
		amd_axi_timer_start_periodic_clockevent();
	}

	IRQ_CONNECT(DT_IRQN(TIMER_NODE), DT_IRQ(TIMER_NODE, priority), amd_axi_timer_isr, NULL, 0);
	irq_enable(DT_IRQN(TIMER_NODE));

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

BUILD_ASSERT(!DT_PROP(TIMER_NODE, xlnx_one_timer_only),
	     "AMD AXI Timer one-timer-only mode is not supported as the "
	     "Zephyr system clock; a second counter is required");
BUILD_ASSERT(DT_PROP_BY_PHANDLE(TIMER_NODE, clocks, clock_frequency) >=
		     CONFIG_SYS_CLOCK_TICKS_PER_SEC,
	     "Timer clock frequency must be greater than the system tick frequency");
BUILD_ASSERT((DT_PROP_BY_PHANDLE(TIMER_NODE, clocks, clock_frequency) %
	      CONFIG_SYS_CLOCK_TICKS_PER_SEC) == 0,
	     "Timer clock frequency is not divisible by the system tick frequency");
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC ==
		     DT_PROP_BY_PHANDLE(TIMER_NODE, clocks, clock_frequency),
	     "CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC must equal the AMD AXI Timer "
	     "clock frequency for correct cycle-to-time conversion");
