/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_lptmr

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <fsl_lptmr.h>
#include <zephyr/irq.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "No LPTMR instance enabled in devicetree");

/* Prescaler mapping */
#define LPTMR_PRESCALER_2     kLPTMR_Prescale_Glitch_0
#define LPTMR_PRESCALER_4     kLPTMR_Prescale_Glitch_1
#define LPTMR_PRESCALER_8     kLPTMR_Prescale_Glitch_2
#define LPTMR_PRESCALER_16    kLPTMR_Prescale_Glitch_3
#define LPTMR_PRESCALER_32    kLPTMR_Prescale_Glitch_4
#define LPTMR_PRESCALER_64    kLPTMR_Prescale_Glitch_5
#define LPTMR_PRESCALER_128   kLPTMR_Prescale_Glitch_6
#define LPTMR_PRESCALER_256   kLPTMR_Prescale_Glitch_7
#define LPTMR_PRESCALER_512   kLPTMR_Prescale_Glitch_8
#define LPTMR_PRESCALER_1024  kLPTMR_Prescale_Glitch_9
#define LPTMR_PRESCALER_2048  kLPTMR_Prescale_Glitch_10
#define LPTMR_PRESCALER_4096  kLPTMR_Prescale_Glitch_11
#define LPTMR_PRESCALER_8192  kLPTMR_Prescale_Glitch_12
#define LPTMR_PRESCALER_16384 kLPTMR_Prescale_Glitch_13
#define LPTMR_PRESCALER_32768 kLPTMR_Prescale_Glitch_14
#define LPTMR_PRESCALER_65536 kLPTMR_Prescale_Glitch_15
#define TO_LPTMR_PRESCALER(val) _DO_CONCAT(LPTMR_PRESCALER_, val)

/* Prescaler clock mapping */
#define TO_LPTMR_CLK_SEL(val) _DO_CONCAT(kLPTMR_PrescalerClock_, val)

/* Devicetree properties */
#define LPTMR_BASE ((LPTMR_Type *)(DT_INST_REG_ADDR(0)))
#define LPTMR_CLK_SOURCE TO_LPTMR_CLK_SEL(DT_INST_PROP(0, clk_source));
#define LPTMR_PRESCALER TO_LPTMR_PRESCALER(DT_INST_PROP(0, prescaler));
#define LPTMR_BYPASS_PRESCALER DT_INST_PROP(0, prescaler) == 1
#define LPTMR_IRQN DT_INST_IRQN(0)
#define LPTMR_IRQ_PRIORITY DT_INST_IRQ(0, priority)

/* Timer cycles per tick */
#define CYCLES_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/* 32 bit cycle counter */
static volatile uint32_t cycles;

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (idle && (ticks == K_TICKS_FOREVER)) {
		LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	}
}

void sys_clock_idle_exit(void)
{
	if (LPTMR_GetEnabledInterrupts(LPTMR_BASE) != kLPTMR_TimerInterruptEnable) {
		LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	}
}

void sys_clock_disable(void)
{
	LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_StopTimer(LPTMR_BASE);
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return LPTMR_GetCurrentTimerCount(LPTMR_BASE) + cycles;
}

static void mcux_lptmr_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	cycles += CYCLES_PER_TICK;

	sys_clock_announce(1);
	LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);
}

static int sys_clock_driver_init(void)
{
	lptmr_config_t config;


	LPTMR_GetDefaultConfig(&config);
	config.timerMode = kLPTMR_TimerModeTimeCounter;
	config.enableFreeRunning = false;
	config.prescalerClockSource = LPTMR_CLK_SOURCE;

#if LPTMR_BYPASS_PRESCALER
	config.bypassPrescaler = true;
#else /* LPTMR_BYPASS_PRESCALER */
	config.bypassPrescaler = false;
	config.value = LPTMR_PRESCALER;
#endif /* !LPTMR_BYPASS_PRESCALER */

	LPTMR_Init(LPTMR_BASE, &config);

	IRQ_CONNECT(LPTMR_IRQN, LPTMR_IRQ_PRIORITY, mcux_lptmr_timer_isr, NULL, 0);
	irq_enable(LPTMR_IRQN);

	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_SetTimerPeriod(LPTMR_BASE, CYCLES_PER_TICK);
	LPTMR_StartTimer(LPTMR_BASE);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
