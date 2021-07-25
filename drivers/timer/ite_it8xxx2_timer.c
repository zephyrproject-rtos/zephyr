/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_timer

#include <drivers/timer/system_timer.h>
#include <dt-bindings/interrupt-controller/ite-intc.h>
#include <soc.h>
#include <spinlock.h>
#include <sys_clock.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(timer, LOG_LEVEL_ERR);

/* Control external timer3~8 */
#define IT8XXX2_EXT_TIMER_BASE	(DT_INST_REG_ADDR(0))/*0x00F01F10*/
#define IT8XXX2_EXT_CTRLX(n)	ECREG(IT8XXX2_EXT_TIMER_BASE + (n << 3))
#define IT8XXX2_EXT_PSRX(n)	ECREG(IT8XXX2_EXT_TIMER_BASE + 0x01 + (n << 3))
#define IT8XXX2_EXT_CNTX(n)	ECREG_u32(IT8XXX2_EXT_TIMER_BASE + 0x04 + \
						(n << 3))
#define IT8XXX2_EXT_CNTOX(n)	ECREG_u32(IT8XXX2_EXT_TIMER_BASE + 0x38 + \
						(n << 2))

/* Event timer configurations */
#define EVENT_TIMER		EXT_TIMER_3
#define EVENT_TIMER_IRQ		DT_INST_IRQ_BY_IDX(0, 0, irq)
#define EVENT_TIMER_FLAG	DT_INST_IRQ_BY_IDX(0, 0, flags)
/* Event timer max count is 512 sec (base on clock source 32768Hz) */
#define EVENT_TIMER_MAX_CNT	0x00FFFFFFUL

/* Free run timer configurations */
#define FREE_RUN_TIMER		EXT_TIMER_4
#define FREE_RUN_TIMER_IRQ	DT_INST_IRQ_BY_IDX(0, 1, irq)
#define FREE_RUN_TIMER_FLAG	DT_INST_IRQ_BY_IDX(0, 1, flags)
/* Free run timer max count is 36.4 hr (base on clock source 32768Hz) */
#define FREE_RUN_TIMER_MAX_CNT	0xFFFFFFFFUL

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
/*
 * One system (kernel) tick is as how much HW timer counts
 *
 * NOTE: Event and free run timer individually select the same clock source
 *       frequency, so they can use the same HW_CNT_PER_SYS_TICK to tranform
 *       unit between HW count and system tick. If clock source frequency is
 *       different, then we should define another to tranform.
 */
#define HW_CNT_PER_SYS_TICK	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC \
				 / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
/* Event timer max count is as how much system (kernal) tick */
#define EVEN_TIMER_MAX_CNT_SYS_TICK	(EVENT_TIMER_MAX_CNT \
					/ HW_CNT_PER_SYS_TICK)

enum ext_clk_src_sel {
	EXT_PSR_32P768K = 0,
	EXT_PSR_1P024K,
	EXT_PSR_32,
	EXT_PSR_8M,
};

/*
 * 24-bit timers: external timer 3, 5, and 7
 * 32-bit timers: external timer 4, 6, and 8
 */
enum ext_timer_idx {
	EXT_TIMER_3 = 0,	/* Event timer */
	EXT_TIMER_4,		/* Free run timer */
	EXT_TIMER_5,
	EXT_TIMER_6,
	EXT_TIMER_7,
	EXT_TIMER_8,
};

static struct k_spinlock lock;
/* Last HW count that we called sys_clock_announce() */
static volatile uint32_t last_announced_hw_cnt;

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
	ite_intc_irq_priority_set(ONE_SHOT_TIMER_IRQ, 0, ONE_SHOT_TIMER_FLAG);

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

static void evt_timer_isr(const void *unused)
{
	ARG_UNUSED(unused);

	/* Disable event timer */
	IT8XXX2_EXT_CTRLX(EVENT_TIMER) &= ~IT8XXX2_EXT_ETXEN;
	/* W/C event timer interrupt status */
	ite_intc_isr_clear(EVENT_TIMER_IRQ);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * Get free run observer count from last time announced and
		 * trnaform unit to system tick
		 */
		uint32_t dticks = (~(IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER)) -
				   last_announced_hw_cnt) / HW_CNT_PER_SYS_TICK;
		last_announced_hw_cnt += (dticks * HW_CNT_PER_SYS_TICK);

		sys_clock_announce(dticks);
	} else {
		/* Enable and re-start event timer */
		IT8XXX2_EXT_CTRLX(EVENT_TIMER) |= (IT8XXX2_EXT_ETXEN |
						   IT8XXX2_EXT_ETXRST);

		/* Informs kernel that one system tick has elapsed */
		sys_clock_announce(1);
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	uint32_t hw_cnt;

	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return for non-tickless kernel system */
		return;
	}

	/* Critical section */
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* NOTE: To reduce cpu effort, we don't add critical section here */
	if (ticks == K_TICKS_FOREVER) {
		hw_cnt = EVENT_TIMER_MAX_CNT;
	} else if (ticks <= 1) {
		/*
		 * Ticks <= 1 means the kernel wants the tick announced
		 * as soon as possible, ideally no more than one system tick
		 * in the future. So set event timer count to 1 system tick or
		 * at least 1 hw count.
		 */
		hw_cnt = MAX((1 * HW_CNT_PER_SYS_TICK), 1);
	} else {
		if (ticks > EVEN_TIMER_MAX_CNT_SYS_TICK)
			/*
			 * Set event timer count to EVENT_TIMER_MAX_CNT, after
			 * interrupt fired the remaining time will be set again
			 * by sys_clock_announce().
			 */
			hw_cnt = EVENT_TIMER_MAX_CNT;
		else
			/*
			 * Set event timer count to system tick or at least
			 * 1 hw count
			 */
			hw_cnt = MAX((ticks * HW_CNT_PER_SYS_TICK), 1);
	}

	/* Set event timer 24-bit count */
	IT8XXX2_EXT_CNTX(EVENT_TIMER) = hw_cnt;

	/* Enable and re-start event timer */
	IT8XXX2_EXT_CTRLX(EVENT_TIMER) |= (IT8XXX2_EXT_ETXEN |
					   IT8XXX2_EXT_ETXRST);

	/* W/C event timer interrupt status */
	ite_intc_isr_clear(EVENT_TIMER_IRQ);

	k_spin_unlock(&lock, key);

	LOG_DBG("timeout is 0x%x, set hw count 0x%x", ticks, hw_cnt);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for non-tickless kernel system */
		return 0;
	}

	/* Critical section */
	k_spinlock_key_t key = k_spin_lock(&lock);
	/*
	 * Get free run observer count from last time announced and trnaform
	 * unit to system tick
	 */
	uint32_t dticks = (~(IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER)) -
				last_announced_hw_cnt) / HW_CNT_PER_SYS_TICK;
	k_spin_unlock(&lock, key);

	return dticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	/*
	 * Get free run observer count and trnaform unit to system tick
	 *
	 * NOTE: Timer is counting down from 0xffffffff. In not combined
	 *       mode, the observer count value is the same as count, so after
	 *       NOT count operation we can get counting up value; In
	 *       combined mode, the observer count value is the same as NOT
	 *       count operation.
	 */
	uint32_t dticks = (~(IT8XXX2_EXT_CNTOX(FREE_RUN_TIMER)))
				/ HW_CNT_PER_SYS_TICK;

	return dticks;
}

static int timer_init(enum ext_timer_idx ext_timer,
			enum ext_clk_src_sel clock_source_sel,
			uint32_t raw,
			uint32_t ms,
			uint8_t first_time_enable,
			uint32_t irq_num,
			uint32_t irq_flag,
			uint8_t with_int,
			uint8_t start)
{
	uint32_t hw_cnt;

	if (raw) {
		hw_cnt = ms;
	} else {
		if (clock_source_sel == EXT_PSR_32P768K)
			hw_cnt = MS_TO_COUNT(32768, ms);
		else if (clock_source_sel == EXT_PSR_1P024K)
			hw_cnt = MS_TO_COUNT(1024, ms);
		else if (clock_source_sel == EXT_PSR_32)
			hw_cnt = MS_TO_COUNT(32, ms);
		else if (clock_source_sel == EXT_PSR_8M)
			hw_cnt = 8000 * ms;
		else {
			LOG_ERR("Timer %d clock source error !", ext_timer);
			return -1;
		}
	}

	if (hw_cnt == 0) {
		LOG_ERR("Timer %d count shouldn't be 0 !", ext_timer);
		return -1;
	}

	if (first_time_enable) {
		/* Enable and re-start external timer x */
		IT8XXX2_EXT_CTRLX(ext_timer) |= (IT8XXX2_EXT_ETXEN |
						 IT8XXX2_EXT_ETXRST);
		/* Disable external timer x */
		IT8XXX2_EXT_CTRLX(ext_timer) &= ~IT8XXX2_EXT_ETXEN;
	}

	/* Set rising edge triggered of external timer x */
	ite_intc_irq_priority_set(irq_num, 0, irq_flag);

	/* Clear interrupt status of external timer x */
	ite_intc_isr_clear(irq_num);

	/* Set clock source of external timer x */
	IT8XXX2_EXT_PSRX(ext_timer) = clock_source_sel;

	/* Set count of external timer x */
	IT8XXX2_EXT_CNTX(ext_timer) = hw_cnt;

	/* Disable external timer x */
	IT8XXX2_EXT_CTRLX(ext_timer) &= ~IT8XXX2_EXT_ETXEN;
	if (start)
		/* Enable and re-start external timer x */
		IT8XXX2_EXT_CTRLX(ext_timer) |= (IT8XXX2_EXT_ETXEN |
						 IT8XXX2_EXT_ETXRST);

	if (with_int)
		irq_enable(irq_num);
	else
		irq_disable(irq_num);

	return 0;
}

int sys_clock_driver_init(const struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	/* Set 32-bit timer4 for free run*/
	ret = timer_init(FREE_RUN_TIMER, EXT_PSR_32P768K, TRUE,
			 FREE_RUN_TIMER_MAX_CNT, TRUE, FREE_RUN_TIMER_IRQ,
			 FREE_RUN_TIMER_FLAG, FALSE, TRUE);
	if (ret < 0) {
		LOG_ERR("Init free run timer failed");
		return ret;
	}

	/* Set 24-bit timer3 for timeout event */
	IRQ_CONNECT(EVENT_TIMER_IRQ, 0, evt_timer_isr, NULL, EVENT_TIMER_FLAG);
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ret = timer_init(EVENT_TIMER, EXT_PSR_32P768K, TRUE,
				 EVENT_TIMER_MAX_CNT, TRUE, EVENT_TIMER_IRQ,
				 EVENT_TIMER_FLAG, TRUE, FALSE);
	} else {
		/* Start a event timer in one system tick */
		ret = timer_init(EVENT_TIMER, EXT_PSR_32P768K, TRUE,
				 MAX((1 * HW_CNT_PER_SYS_TICK), 1), TRUE,
				 EVENT_TIMER_IRQ, EVENT_TIMER_FLAG,
				 TRUE, TRUE);
	}
	if (ret < 0) {
		LOG_ERR("Init event timer failed");
		return ret;
	}

	return 0;
}
