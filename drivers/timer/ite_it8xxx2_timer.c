/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_timer

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>
#include <soc.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/clock.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(timer, LOG_LEVEL_ERR);

#define COUNT_1US (EC_FREQ / USEC_PER_SEC - 1)

BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 32768,
	     "ITE RTOS timer HW frequency is fixed at 32768Hz");

/* Event timer configurations */
#define EVENT_TIMER		EXT_TIMER_3
#define EVENT_TIMER_IRQ		DT_INST_IRQ_BY_IDX(0, 0, irq)
#define EVENT_TIMER_FLAG	DT_INST_IRQ_BY_IDX(0, 0, flags)
/* Event timer max count is 512 sec (base on clock source 32768Hz) */
#define EVENT_TIMER_MAX_CNT	0x00FFFFFFUL

/* Busy wait low timer configurations */
#define BUSY_WAIT_L_TIMER	EXT_TIMER_5
#define BUSY_WAIT_L_TIMER_IRQ	DT_INST_IRQ_BY_IDX(0, 2, irq)
#define BUSY_WAIT_L_TIMER_FLAG	DT_INST_IRQ_BY_IDX(0, 2, flags)

/* Busy wait high timer configurations */
#define BUSY_WAIT_H_TIMER	EXT_TIMER_6
#define BUSY_WAIT_H_TIMER_IRQ	DT_INST_IRQ_BY_IDX(0, 3, irq)
#define BUSY_WAIT_H_TIMER_FLAG	DT_INST_IRQ_BY_IDX(0, 3, flags)
/* Busy wait high timer max count is 71.58min (base on clock source 1MHz) */
#define BUSY_WAIT_TIMER_H_MAX_CNT 0xFFFFFFFFUL

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQ_BY_IDX(DT_NODELABEL(timer), 5, irq);
#endif

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M
/*
 * One shot timer configurations
 *
 * NOTE: Timer1/2 register address isn't regular like timer3/4/5/6/7/8, and
 *       timer1 is used for printing watchdog warning message. So now we use
 *       timer2 only one shot to wake up chip and change pll.
 */
#define WDT_BASE		DT_REG_ADDR(DT_NODELABEL(twd0))
#define WDT_REG			(struct wdt_it8xxx2_regs *)(WDT_BASE)
#define ONE_SHOT_TIMER_IRQ	DT_IRQ_BY_IDX(DT_NODELABEL(twd0), 1, irq)
#define ONE_SHOT_TIMER_FLAG	DT_IRQ_BY_IDX(DT_NODELABEL(twd0), 1, flags)
#endif

#define MS_TO_COUNT(hz, ms)	((hz) * (ms) / 1000)

/* Timer count threshold to prevent SoC from entering idle mode */
#define IDLE_BLOCK_TIMER_TICKS	k_us_to_cyc_ceil32(150)

enum ext_timer_raw_cnt {
	EXT_NOT_RAW_CNT = 0,
	EXT_RAW_CNT,
};

enum ext_timer_init {
	EXT_NOT_FIRST_TIME_ENABLE = 0,
	EXT_FIRST_TIME_ENABLE,
};

enum ext_timer_int {
	EXT_WITHOUT_TIMER_INT = 0,
	EXT_WITH_TIMER_INT,
};

enum ext_timer_start {
	EXT_NOT_START_TIMER = 0,
	EXT_START_TIMER,
};

#ifdef CONFIG_SOC_IT8XXX2_PLL_FLASH_48M
static void timer_5ms_one_shot_isr(const void *unused)
{
	ARG_UNUSED(unused);

	/*
	 * We are here because we have completed changing PLL sequence,
	 * so disabled one shot timer interrupt.
	 */
	irq_disable(ONE_SHOT_TIMER_IRQ);
}

/*
 * This timer is used to wake up chip from sleep mode to complete
 * changing PLL sequence.
 */
void timer_5ms_one_shot(void)
{
	struct wdt_it8xxx2_regs *const timer2_reg = WDT_REG;
	uint32_t hw_cnt;

	/* Initialize interrupt handler of one shot timer */
	IRQ_CONNECT(ONE_SHOT_TIMER_IRQ, 0, timer_5ms_one_shot_isr, NULL,
			ONE_SHOT_TIMER_FLAG);

	/* Set rising edge triggered of one shot timer */
	ite_intc_irq_polarity_set(ONE_SHOT_TIMER_IRQ, ONE_SHOT_TIMER_FLAG);

	/* Clear interrupt status of one shot timer */
	ite_intc_isr_clear(ONE_SHOT_TIMER_IRQ);

	/* Set clock source of one shot timer */
	timer2_reg->ET2PSR = EXT_PSR_32P768K;

	/*
	 * Set count of one shot timer,
	 * and after write ET2CNTLLR timer will start
	 */
	hw_cnt = MS_TO_COUNT(32768, 5/*ms*/);
	timer2_reg->ET2CNTLH2R = (uint8_t)((hw_cnt >> 16) & 0xff);
	timer2_reg->ET2CNTLHR = (uint8_t)((hw_cnt >> 8) & 0xff);
	timer2_reg->ET2CNTLLR = (uint8_t)(hw_cnt & 0xff);

	irq_enable(ONE_SHOT_TIMER_IRQ);
}
#endif /* CONFIG_SOC_IT8XXX2_PLL_FLASH_48M */

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	uint32_t start = IT8XXX2_EXT_CNTOX(BUSY_WAIT_H_TIMER);

	if (!usec_to_wait) {
		return;
	}

	/* Decrease 1us here to calibrate our access registers latency */
	usec_to_wait--;

	for (;;) {
		if ((IT8XXX2_EXT_CNTOX(BUSY_WAIT_H_TIMER) - start) >= usec_to_wait) {
			break;
		}
	}
}
#endif

static void evt_timer_enable(void)
{
	/* Enable and re-start event timer */
	IT8XXX2_EXT_CTRLX(EVENT_TIMER) |= (IT8XXX2_EXT_ETXEN |
					   IT8XXX2_EXT_ETXRST);
}

/*
 * The counter and the alarm are two different timers here. The cycle domain is
 * the free-running 32-bit timer, counting down from 0xffffffff and read back
 * inverted, while the wakeup comes from the separate 24-bit event timer, a
 * one-shot countdown. That makes it a RELOAD backend whose range is bounded by
 * the event timer rather than by the counter. Both select the same clock, so
 * the core's cycles are the event timer's without conversion.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 32
#define TIMER_CORE_ALARM_MAX_CYCLES EVENT_TIMER_MAX_CNT

static inline uint32_t timer_driver_cycle_get(void)
{
	/*
	 * NOTE: Timer is counting down from 0xffffffff. In not combined
	 *       mode, the observer count value is the same as count, so after
	 *       NOT count operation we can get counting up value; In
	 *       combined mode, the observer count value is the same as NOT
	 *       count operation.
	 */
	return ~(IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER));
}

static void timer_driver_set_reload(uint32_t cycles)
{
	/* Disable event timer */
	IT8XXX2_EXT_CTRLX(EVENT_TIMER) &= ~IT8XXX2_EXT_ETXEN;

	/* Set event timer 24-bit count */
	IT8XXX2_EXT_CNTX(EVENT_TIMER) = cycles;

	/* W/C event timer interrupt status */
	ite_intc_isr_clear(EVENT_TIMER_IRQ);

	evt_timer_enable();
}

#include "system_timer_generic.h"

static void evt_timer_isr(const void *unused)
{
	ARG_UNUSED(unused);

	/* Disable event timer */
	IT8XXX2_EXT_CTRLX(EVENT_TIMER) &= ~IT8XXX2_EXT_ETXEN;
	/* W/C event timer interrupt status */
	ite_intc_isr_clear(EVENT_TIMER_IRQ);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* The event timer is one-shot, so a ticked kernel re-arms it
		 * here: the core leaves a reload backend alone, expecting
		 * hardware that reloads itself.
		 */
		evt_timer_enable();
	}

	timer_core_announce();
}

static void free_run_timer_overflow_isr(const void *unused)
{
	ARG_UNUSED(unused);

	/* Read to clear terminal count flag */
	__unused uint8_t rc_tc = IT8XXX2_EXT_CTRLX(FREE_RUN_TIMER);

	/*
	 * TODO: to increment 32-bit "top half" here for software 64-bit
	 * timer emulation.
	 */
}

static int timer_init(enum ext_timer_idx ext_timer,
			enum ext_clk_src_sel clock_source_sel,
			enum ext_timer_raw_cnt raw,
			uint32_t ms,
			enum ext_timer_init first_time_enable,
			uint32_t irq_num,
			uint32_t irq_flag,
			enum ext_timer_int with_int,
			enum ext_timer_start start)
{
	uint32_t hw_cnt;

	if (raw == EXT_RAW_CNT) {
		hw_cnt = ms;
	} else {
		if (clock_source_sel == EXT_PSR_32P768K) {
			hw_cnt = MS_TO_COUNT(32768, ms);
		} else if (clock_source_sel == EXT_PSR_1P024K) {
			hw_cnt = MS_TO_COUNT(1024, ms);
		} else if (clock_source_sel == EXT_PSR_32) {
			hw_cnt = MS_TO_COUNT(32, ms);
		} else if (clock_source_sel == EXT_PSR_EC_CLK) {
			hw_cnt = MS_TO_COUNT(EC_FREQ, ms);
		} else {
			LOG_ERR("Timer %d clock source error !", ext_timer);
			return -1;
		}
	}

	if (hw_cnt == 0) {
		LOG_ERR("Timer %d count shouldn't be 0 !", ext_timer);
		return -1;
	}

	if (first_time_enable == EXT_FIRST_TIME_ENABLE) {
		/* Enable and re-start external timer x */
		IT8XXX2_EXT_CTRLX(ext_timer) |= (IT8XXX2_EXT_ETXEN |
						 IT8XXX2_EXT_ETXRST);
		/* Disable external timer x */
		IT8XXX2_EXT_CTRLX(ext_timer) &= ~IT8XXX2_EXT_ETXEN;
	}

	/* Set rising edge triggered of external timer x */
	ite_intc_irq_polarity_set(irq_num, irq_flag);

	/* Clear interrupt status of external timer x */
	ite_intc_isr_clear(irq_num);

	/* Set clock source of external timer x */
	IT8XXX2_EXT_PSRX(ext_timer) = clock_source_sel;

	/* Set count of external timer x */
	IT8XXX2_EXT_CNTX(ext_timer) = hw_cnt;

	/* Disable external timer x */
	IT8XXX2_EXT_CTRLX(ext_timer) &= ~IT8XXX2_EXT_ETXEN;
	if (start == EXT_START_TIMER) {
		/* Enable and re-start external timer x */
		IT8XXX2_EXT_CTRLX(ext_timer) |= (IT8XXX2_EXT_ETXEN |
						 IT8XXX2_EXT_ETXRST);
	}

	if (with_int == EXT_WITH_TIMER_INT) {
		irq_enable(irq_num);
	} else {
		irq_disable(irq_num);
	}

	return 0;
}

bool ite_it8xxx2_timer_block_idle(void)
{
	return (IT8XXX2_EXT_CNTOX(EVENT_TIMER) < IDLE_BLOCK_TIMER_TICKS) ||
	       (IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER) < IDLE_BLOCK_TIMER_TICKS);
}

#ifdef CONFIG_PM
static uint64_t cyc_deep_sleep_total;
static uint32_t cyc_enter_deep_sleep;

void ite_ec_clock_capture_low_freq_timer(void)
{
	cyc_enter_deep_sleep = ~(IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER));
}

void ite_ec_clock_compensate_system_timer(void)
{
	uint32_t now = ~(IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER));
	uint32_t cyc_elapsed_in_deep = now - cyc_enter_deep_sleep;

	cyc_deep_sleep_total += cyc_elapsed_in_deep;
}

uint64_t ite_ec_clock_get_sleep_ticks(void)
{
	return k_cyc_to_ticks_floor64(cyc_deep_sleep_total);
}
#endif /* CONFIG_PM */

static int sys_clock_driver_init(void)
{
	int ret;


	/* Enable 32-bit free run timer overflow interrupt */
	IRQ_CONNECT(FREE_RUN_TIMER_IRQ, 0, free_run_timer_overflow_isr, NULL,
		    FREE_RUN_TIMER_FLAG);
	/* Set 32-bit timer4 for free run*/
	ret = timer_init(FREE_RUN_TIMER, EXT_PSR_32P768K, EXT_RAW_CNT,
			 FREE_RUN_TIMER_MAX_CNT, EXT_FIRST_TIME_ENABLE,
			 FREE_RUN_TIMER_IRQ, FREE_RUN_TIMER_FLAG,
			 EXT_WITH_TIMER_INT, EXT_START_TIMER);
	if (ret < 0) {
		LOG_ERR("Init free run timer failed");
		return ret;
	}

	/* Set 24-bit timer3 for timeout event */
	IRQ_CONNECT(EVENT_TIMER_IRQ, 0, evt_timer_isr, NULL, EVENT_TIMER_FLAG);
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ret = timer_init(EVENT_TIMER, EXT_PSR_32P768K, EXT_RAW_CNT,
				 EVENT_TIMER_MAX_CNT, EXT_FIRST_TIME_ENABLE,
				 EVENT_TIMER_IRQ, EVENT_TIMER_FLAG,
				 EXT_WITH_TIMER_INT, EXT_NOT_START_TIMER);
	} else {
		/* Start a event timer in one system tick */
		ret = timer_init(EVENT_TIMER, EXT_PSR_32P768K, EXT_RAW_CNT,
				 TIMER_CORE_CYC_PER_TICK,
				 EXT_FIRST_TIME_ENABLE, EVENT_TIMER_IRQ,
				 EVENT_TIMER_FLAG, EXT_WITH_TIMER_INT,
				 EXT_START_TIMER);
	}
	if (ret < 0) {
		LOG_ERR("Init event timer failed");
		return ret;
	}

	/* Seed the announce baseline and arm the first tick. */
	timer_core_init();

	if (IS_ENABLED(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)) {
		/* Set timer5 and timer6 combinational mode for busy wait */
		IT8XXX2_EXT_CTRLX(BUSY_WAIT_L_TIMER) |= IT8XXX2_EXT_ETXCOMB;

		/* Set 32-bit timer6 to count-- every 1us */
		ret = timer_init(BUSY_WAIT_H_TIMER, EXT_PSR_EC_CLK, EXT_RAW_CNT,
				 BUSY_WAIT_TIMER_H_MAX_CNT, EXT_FIRST_TIME_ENABLE,
				 BUSY_WAIT_H_TIMER_IRQ, BUSY_WAIT_H_TIMER_FLAG,
				 EXT_WITHOUT_TIMER_INT, EXT_START_TIMER);
		if (ret < 0) {
			LOG_ERR("Init busy wait high timer failed");
			return ret;
		}

		/*
		 * Set 24-bit timer5 to overflow every 1us
		 * NOTE: When the timer5 count down to overflow in combinational
		 *       mode, timer6 counter will automatically decrease one count
		 *       and timer5 will automatically re-start counting down
		 *       from COUNT_1US. Timer5 clock source is EC_FREQ, so the
		 *       time period from COUNT_1US to overflow is
		 *       (1 / EC_FREQ) * (EC_FREQ / USEC_PER_SEC) = 1us.
		 */
		ret = timer_init(BUSY_WAIT_L_TIMER, EXT_PSR_EC_CLK, EXT_RAW_CNT,
				 COUNT_1US, EXT_FIRST_TIME_ENABLE,
				 BUSY_WAIT_L_TIMER_IRQ, BUSY_WAIT_L_TIMER_FLAG,
				 EXT_WITHOUT_TIMER_INT, EXT_START_TIMER);
		if (ret < 0) {
			LOG_ERR("Init busy wait low timer failed");
			return ret;
		}
	}

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
