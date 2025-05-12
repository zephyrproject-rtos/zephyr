/* Copyright (C) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel.h>

#include <ti/driverlib/dl_timerg.h>
#include <ti/driverlib/dl_timer.h>

#define DT_DRV_COMPAT ti_mspm0_timer_sysclock

#define MSPM0_TMR_PRESCALE	DT_PROP(DT_INST_PARENT(0), clk_prescaler)
#define MSPM0_TMR_IRQN		DT_IRQN(DT_INST_PARENT(0))
#define MSPM0_TMR_IRQ_PRIO	DT_IRQ(DT_INST_PARENT(0), priority)
#define MSPM0_TMR_BASE 		((GPTIMER_Regs *)(DT_REG_ADDR(DT_INST_PARENT(0))))
#define MSPM0_TMR_CLK		(DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(0), 0, bus) & \
				 MSPM0_CLOCK_SEL_MASK)
#define MSPM0_CLK_DIV(div) 	DT_CAT(DL_TIMER_CLOCK_DIVIDE_, div)
#define MSPM0_TMR_CLK_DIV	MSPM0_CLK_DIV(DT_PROP(DT_INST_PARENT(0), clk_div))

/* Timer cycles per tick */
#define CYC_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

static uint32_t cycles;

static void mspm0_timer_isr(void *arg)
{
	uint32_t status;
	ARG_UNUSED(arg);

	status = DL_Timer_getPendingInterrupt(MSPM0_TMR_BASE);
	if ((status & DL_TIMER_IIDX_LOAD) == 0) {
		return;
	}

	cycles += CYC_PER_TICK;
	sys_clock_announce(1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (idle && (ticks == K_TICKS_FOREVER)) {
		DL_Timer_disableInterrupt(MSPM0_TMR_BASE,
					  DL_TIMER_INTERRUPT_LOAD_EVENT);
	}
}

void sys_clock_idle_exit(void)
{
	if (DL_Timer_getEnabledInterruptStatus(MSPM0_TMR_BASE,
					 DL_TIMER_INTERRUPT_LOAD_EVENT) == 0) {
		DL_Timer_enableInterrupt(MSPM0_TMR_BASE,
					 DL_TIMER_INTERRUPT_LOAD_EVENT);
	}
}

void sys_clock_disable(void)
{
	DL_Timer_disableInterrupt(MSPM0_TMR_BASE,
				  DL_TIMER_INTERRUPT_LOAD_EVENT);
	DL_Timer_stopCounter(MSPM0_TMR_BASE);
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return DL_Timer_getTimerCount(MSPM0_TMR_BASE) + cycles;
}

static int mspm0_sysclock_init(void)
{
	DL_Timer_TimerConfig tim_config = {
			.period = CYC_PER_TICK,
			.timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP,
			.startTimer = DL_TIMER_START,
			};

	DL_Timer_ClockConfig clk_config = {
		.clockSel = MSPM0_TMR_CLK,
		.divideRatio = MSPM0_TMR_CLK_DIV,
		.prescale = MSPM0_TMR_PRESCALE,
		};

	DL_Timer_reset(MSPM0_TMR_BASE);
	DL_Timer_enablePower(MSPM0_TMR_BASE);

	delay_cycles(16);
	DL_Timer_setClockConfig(MSPM0_TMR_BASE, &clk_config);
	DL_Timer_initTimerMode(MSPM0_TMR_BASE, &tim_config);
	DL_Timer_setCounterRepeatMode(MSPM0_TMR_BASE,
				      DL_TIMER_REPEAT_MODE_ENABLED);

	IRQ_CONNECT(MSPM0_TMR_IRQN, MSPM0_TMR_IRQ_PRIO, mspm0_timer_isr, 0, 0);
	irq_enable(MSPM0_TMR_IRQN);

	DL_Timer_clearInterruptStatus(MSPM0_TMR_BASE,
				      DL_TIMER_INTERRUPT_LOAD_EVENT);
	DL_Timer_enableInterrupt(MSPM0_TMR_BASE,
				 DL_TIMER_INTERRUPT_LOAD_EVENT);

	return 0;
}

SYS_INIT(mspm0_sysclock_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
