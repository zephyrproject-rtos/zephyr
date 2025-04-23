/*
 * Copyright (c) 2025 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT		ti_tms570_rti

#define REG_RTI			DT_INST_REG_ADDR_BY_IDX(0, 0)
#define RTI_CLK_FREQ		DT_PROP(DT_NODELABEL(clk_rticlk), clock_frequency)

#define RTI_INTFLAG_COMP0	BIT(0)
#define RTI_INTENA_COMP0	BIT(0)

#define RTI_INTCLR_ALL		(BIT(18) | BIT(17) | BIT(16) | \
				 BIT(3) | BIT(2) | BIT(1) | BIT(0))

#define RTIGCTRL		(REG_RTI + 0x00)
#define RTITBCTRL		(REG_RTI + 0x04)
#define RTICAPCTRL		(REG_RTI + 0x08)
#define RTICOMPCTRL		(REG_RTI + 0x0C)
#define RTISETINTENA		(REG_RTI + 0x80)
#define RTICLEARINTENA		(REG_RTI + 0x84)
#define RTIINTFLAG		(REG_RTI + 0x88)
#define RTIFRC0			(REG_RTI + 0x10)
#define RTIUC0			(REG_RTI + 0x14)
#define RTICPUC0		(REG_RTI + 0x18)
#define RTICAFRC0		(REG_RTI + 0x20)
#define RTICAUC0		(REG_RTI + 0x24)
#define RTICOMP0		(REG_RTI + 0x50)
#define RTIUDCP0		(REG_RTI + 0x54)

#define RTIGCTRL_COS_OFFSET	(15)
#define CNT1EN			BIT(1)
#define CNT0EN			BIT(0)

static struct k_spinlock ticks_lock;
static volatile uint64_t ticks;

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_read32(RTIFRC0);
}

static void rti_compare0_isr(const void *arg)
{
	ARG_UNUSED(arg);

	K_SPINLOCK(&ticks_lock) {
		ticks += k_ticks_to_cyc_floor32(1);
	}

	sys_write32(RTI_INTFLAG_COMP0, RTIINTFLAG);
	sys_clock_announce(1);
}

static int rti_timer_init(void)
{
	/**
	 * TODO Counter block 0 can be synchronized with Network Time (NTU),
	 * we can make use of that.
	 */

	/* disable counters and interrupts */
	sys_write32(1 << RTIGCTRL_COS_OFFSET, RTIGCTRL);
	sys_write32(RTI_INTCLR_ALL, RTICLEARINTENA);

	/**
	 * We use counter 0 and compare register 0.
	 */

	/* default compare control and capture control */
	sys_write32(0, RTICOMPCTRL);
	sys_write32(0, RTICAPCTRL);

	/* Initialize counter 0 */
	sys_write32(0, RTIUC0);
	sys_write32(0, RTIFRC0);
	sys_write32(0, RTITBCTRL);

	/**
	 * Set up free running counter interrupt cycle.
	 * UCx    - up-counter or prescale counter - driven by RTICLK
	 * CPUCx  - compare up-counter, it acts like a prescaler over UCx
	 * FRCx   - when CPUCx value matches UCx, this reg is incremented by one
	 * COMPx  - this is compared with FRCx, a match generates an interrupt
	 * UDCPx  - UDCPx is added to COMPx when a match occues (COMPx matches FRCx),
	 *         so that we can generate periodic interrupts1
	 */
	sys_write32(RTI_CLK_FREQ / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC - 1U, RTICPUC0);
	/* free running counter, compare match period = 10 ms */
	sys_write32(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC, RTICOMP0);
	sys_write32(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC, RTIUDCP0);

	/* Clear all pending interrupts */
	sys_write32(RTI_INTCLR_ALL, RTIINTFLAG);

	/* connect irq */
	IRQ_CONNECT(IRQ_RTI_COMPARE_0, IRQ_RTI_COMPARE_0, rti_compare0_isr, NULL, 0);
	irq_enable(IRQ_RTI_COMPARE_0);

	/* Enable interrupt */
	sys_write32(RTI_INTENA_COMP0, RTISETINTENA);
	/* Enable timer */
	sys_write32(sys_read32(RTIGCTRL) | CNT1EN | CNT0EN, RTIGCTRL);

	return 0;
}

SYS_INIT(rti_timer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
