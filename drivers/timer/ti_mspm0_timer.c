/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_system_timer

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>

#include <soc.h>

#define MSPM0_TMR_BASE     DT_REG_ADDR(DT_INST_PARENT(0))
#define MSPM0_TMR_IRQ_NUM  DT_IRQN(DT_INST_PARENT(0))
#define MSPM0_TMR_IRQ_PRIO DT_IRQ(DT_INST_PARENT(0), priority)
#define MSPM0_TMR_CLOCK    DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(0), 0, clk)
#define MSPM0_TMR_CLK_DIV  (DT_PROP(DT_INST_PARENT(0), ti_clk_div) - 1U)

/* Dividing the clock frequency by the CLK_DIV */
#define MSPM0_TMR_CYC_PER_SEC (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / (MSPM0_TMR_CLK_DIV + 1))

/* Scaling up the counter value, kernel expects this */
#define MSPM0_TMR_CYC_SCALED(cyc) ((cyc) * (MSPM0_TMR_CLK_DIV + 1))
#define MSPM0_TMR_CYCLES_MAX      UINT32_MAX

/*
 * MSPM0 timer register offset
 */

/* Power/Reset/Clock registers */
#define MSPM0_TMR_REG_PWREN  0x800
#define MSPM0_TMR_REG_RSTCTL 0x804
#define MSPM0_TMR_REG_CLKSEL 0x1008
#define MSPM0_TMR_REG_CPS    0x110c
#define MSPM0_TMR_REG_CLKDIV 0x1000

/* Counter control registers */
#define MSPM0_TMR_REG_CTR    0x1800
#define MSPM0_TMR_REG_CTRCTL 0x1804
#define MSPM0_TMR_REG_LOAD   0x1808

/* Compare and compare control registers */
#define MSPM0_TMR_REG_CC_0 0x1810

/* CPU interrupt registers */
#define MSPM0_TMR_REG_CPU_INT_IIDX  0x1020
#define MSPM0_TMR_REG_CPU_INT_IMASK 0x1028
#define MSPM0_TMR_REG_CPU_INT_ICLR  0x1048

/*
 * Register mask and bit field values
 */

/* Reset control */
#define MSPM0_TMR_RSTCTL_UNLOCK_KEY      0xb1000000U
#define MSPM0_TMR_RSTCTL_CLEAR_RESETSTKY BIT(1)
#define MSPM0_TMR_RSTCTL_ASSERT_RESET    BIT(0)

/* Power Enable */
#define MSPM0_TMR_PWREN_UNLOCK_KEY 0x26000000U
#define MSPM0_TMR_PWREN_ENABLE     BIT(0)

/* Clock Select */
#define MSPM0_TMR_CLKSEL_MASK MSPM0_CLOCK_PERIPH_REG_MASK(MSPM0_TMR_CLOCK)

/* Counter control */
#define MSPM0_TMR_CTRCTL_COUNT_UP_MASK GENMASK(5, 4)
#define MSPM0_TMR_CTRCTL_REPEAT_MASK   GENMASK(3, 1)
#define MSPM0_TMR_CTRCTL_ENABLE_MASK   BIT(0)
#define MSPM0_TMR_CTRCTL_DISABLE_MASK  MSPM0_TMR_CTRCTL_ENABLE_MASK
#define MSPM0_TMR_CTRCTL_COUNT_UP      FIELD_PREP(MSPM0_TMR_CTRCTL_COUNT_UP_MASK, 0x2)
#define MSPM0_TMR_CTRCTL_REPEAT        FIELD_PREP(MSPM0_TMR_CTRCTL_REPEAT_MASK, 0x1)
#define MSPM0_TMR_CTRCTL_ENABLE        BIT(0)
#define MSPM0_TMR_CTRCTL_DISABLE       0x0

/* Interrupts (Events) */
#define MSPM0_TMR_CPU_INT_IIDX_COMPARE_UP   0x9
#define MSPM0_TMR_CPU_INT_COMPARE_UP        BIT(8)
#define MSPM0_TMR_INTERRUPT_COMPARE_ENABLE  MSPM0_TMR_CPU_INT_COMPARE_UP
#define MSPM0_TMR_INTERRUPT_COMPARE_DISABLE 0
#define MSPM0_TMR_CPU_INT_COMPARE_UP_MASK   MSPM0_TMR_CPU_INT_COMPARE_UP

/* Helper macro for BASE_ADDR + REG_OFF */
#define REG_ADDR(reg) (MSPM0_TMR_BASE + MSPM0_TMR_REG_##reg)

/* Helper function to read modify write the register*/
static inline void mspm0_timer_reg_update(uint32_t mask, uint32_t data, uint32_t reg_addr)
{
	uint32_t tmp;

	tmp = sys_read32(reg_addr);
	tmp &= ~mask;
	tmp |= (data & mask);
	sys_write32(tmp, reg_addr);
}

static inline void mspm0_compare_irq_enable(void)
{
	mspm0_timer_reg_update(MSPM0_TMR_CPU_INT_COMPARE_UP_MASK,
			       MSPM0_TMR_INTERRUPT_COMPARE_ENABLE, REG_ADDR(CPU_INT_IMASK));
}

static inline void mspm0_compare_irq_disable(void)
{
	mspm0_timer_reg_update(MSPM0_TMR_CPU_INT_COMPARE_UP_MASK,
			       MSPM0_TMR_INTERRUPT_COMPARE_DISABLE, REG_ADDR(CPU_INT_IMASK));
}

/*
 * TIMG12 is a 32-bit free running up counter, using compare channel 0
 * interrupt is fired when CTR equals CC_0, so the COMPARE_EXACT
 * backend is used.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH  32
#define TIMER_CORE_CYCLES_PER_SEC MSPM0_TMR_CYC_PER_SEC

/* Providing sys_clock_cycle_get_32() so that kernel gets the scaled up clock cycle */
#define TIMER_CORE_HAVE_CYCLE_GET_32

static uint32_t timer_driver_cycle_get(void)
{
	return sys_read32(REG_ADDR(CTR));
}

static void timer_driver_set_compare(uint32_t cycles)
{
	sys_write32(cycles, REG_ADDR(CC_0));
}

#include "system_timer_generic.h"

static void mspm0_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t pending_irq;

	/* Pending interrupt is cleared on the read to IIDX */
	pending_irq = sys_read32(REG_ADDR(CPU_INT_IIDX));
	if (pending_irq != MSPM0_TMR_CPU_INT_IIDX_COMPARE_UP) {
		return;
	}

	k_spinlock_key_t key = sys_clock_lock();

	timer_core_announce_from(key);
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* Scale the counter cycle back */
	return MSPM0_TMR_CYC_SCALED(timer_core_cycle_get());
}

/* Disable the interrupt and the counter */
void sys_clock_disable(void)
{
	mspm0_compare_irq_disable();
	mspm0_timer_reg_update(MSPM0_TMR_CTRCTL_DISABLE_MASK, MSPM0_TMR_CTRCTL_DISABLE,
			       REG_ADDR(CTRCTL));
}

void sys_clock_idle_enter(uint32_t ticks)
{
	if (ticks != SYS_CLOCK_IDLE_FOREVER) {
		sys_clock_set_timeout(ticks, false);
		return;
	}
	mspm0_compare_irq_disable();
}

void sys_clock_idle_exit(void)
{
	mspm0_compare_irq_enable();
}

static int mspm0_timer_init(void)
{
	/* Reset and power on the timer */
	sys_write32(MSPM0_TMR_RSTCTL_UNLOCK_KEY | MSPM0_TMR_RSTCTL_CLEAR_RESETSTKY |
			    MSPM0_TMR_RSTCTL_ASSERT_RESET,
		    REG_ADDR(RSTCTL));
	sys_write32(MSPM0_TMR_PWREN_UNLOCK_KEY | MSPM0_TMR_PWREN_ENABLE, REG_ADDR(PWREN));
	msp_delay_peripheral_startup();

	/* Clock configuration */
	mspm0_timer_reg_update(MSPM0_TMR_CLKSEL_MASK, MSPM0_TMR_CLOCK, REG_ADDR(CLKSEL));
	sys_write32(MSPM0_TMR_CLK_DIV, REG_ADDR(CLKDIV));

	/* Initialize the counter to its maximum value for free-running operation. */
	sys_write32(MSPM0_TMR_CYCLES_MAX, REG_ADDR(LOAD));

	/* Start the timer */
	sys_write32(MSPM0_TMR_CTRCTL_COUNT_UP | MSPM0_TMR_CTRCTL_REPEAT | MSPM0_TMR_CTRCTL_ENABLE,
		    REG_ADDR(CTRCTL));

	IRQ_CONNECT(MSPM0_TMR_IRQ_NUM, MSPM0_TMR_IRQ_PRIO, mspm0_timer_isr, 0, 0);
	timer_core_init();

	/* Clear the interrupt */
	sys_write32(MSPM0_TMR_CPU_INT_COMPARE_UP, REG_ADDR(CPU_INT_ICLR));

	/* Enable the interrupt */
	mspm0_compare_irq_enable();
	irq_enable(MSPM0_TMR_IRQ_NUM);

	return 0;
}

SYS_INIT(mspm0_timer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
