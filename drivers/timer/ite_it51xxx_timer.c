/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_timer

#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(timer, LOG_LEVEL_ERR);

BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 32768,
	     "Hardware timer frequency is fixed at 32768Hz");

/* it51xxx timer registers definition */
static mm_reg_t timer_base = DT_REG_ADDR(DT_NODELABEL(timer));

/* 0x10, 0x18, 0x20, 0x28, 0x30, 0x38: External Timer 3-8 Control Register (n=0 to 5) */
#define TIMER_ETNCTRL(n)   (0x10 + ((n) * 8))
#define TIMER_ETCOMB       BIT(3)
#define TIMER_ETNRST       BIT(1)
#define TIMER_ETNEN        BIT(0)
/* 0x11, 0x19, 0x21, 0x29, 0x31, 0x39: External Timer 3-8 Prescaler Register (n=0 to 5) */
#define TIMER_ETNPSR(n)    (0x11 + ((n) * 8))
/* 0x14, 0x1c, 0x24, 0x2c, 0x34, 0x3c: External Timer 3-8 Counter Register (n=0 to 5) */
#define TIMER_ETNCNTLLR(n) (0x14 + ((n) * 8))
/* 0x48, 0x4c, 0x50, 0x54, 0x58, 0x5c: External Timer 3-8 Counter Observation Register (n=0 to 5) */
#define TIMER_ETNCNTOLR(n) (0x48 + ((n) * 4))

/*
 * 24-bit timers: external timer 3, 5, and 7
 * 32-bit timers: external timer 4, 6, and 8
 */
enum ext_timer_idx {
	EXT_TIMER_3 = 0, /* Event timer */
	EXT_TIMER_4,     /* Free run timer */
	EXT_TIMER_5,     /* Busy wait low timer */
	EXT_TIMER_6,     /* Busy wait high timer */
	EXT_TIMER_7,
	EXT_TIMER_8,
};

enum ext_clk_src_sel {
	EXT_PSR_32P768K = 0,
	EXT_PSR_1P024K,
	EXT_PSR_32,
	EXT_PSR_EC_CLK,
};

enum ext_timer_raw_cnt {
	EXT_NOT_RAW_CNT = 0,
	EXT_RAW_CNT,
};

enum ext_timer_int {
	EXT_WITHOUT_TIMER_INT = 0,
	EXT_WITH_TIMER_INT,
};

enum ext_timer_start {
	EXT_NOT_START_TIMER = 0,
	EXT_START_TIMER,
};

/* Event timer configurations */
#define EVENT_TIMER               EXT_TIMER_3
#define EVENT_TIMER_IRQ           DT_IRQ_BY_IDX(DT_NODELABEL(timer), 0, irq)
#define EVENT_TIMER_FLAG          DT_IRQ_BY_IDX(DT_NODELABEL(timer), 0, flags)
/* Event timer max count is 512 sec (base on clock source 32768Hz) */
#define EVENT_TIMER_MAX_CNT       0x00FFFFFFUL
/* Free run timer configurations */
#define FREE_RUN_TIMER            EXT_TIMER_4
#define FREE_RUN_TIMER_IRQ        DT_IRQ_BY_IDX(DT_NODELABEL(timer), 1, irq)
#define FREE_RUN_TIMER_FLAG       DT_IRQ_BY_IDX(DT_NODELABEL(timer), 1, flags)
/* Free run timer max count is 36.4 hr (base on clock source 32768Hz) */
#define FREE_RUN_TIMER_MAX_CNT    0xFFFFFFFFUL
/* Busy wait low timer configurations */
#define BUSY_WAIT_L_TIMER         EXT_TIMER_5
#define BUSY_WAIT_L_TIMER_IRQ     DT_INST_IRQ_BY_IDX(0, 2, irq)
#define BUSY_WAIT_L_TIMER_FLAG    DT_INST_IRQ_BY_IDX(0, 2, flags)
/* Busy wait high timer configurations */
#define BUSY_WAIT_H_TIMER         EXT_TIMER_6
#define BUSY_WAIT_H_TIMER_IRQ     DT_INST_IRQ_BY_IDX(0, 3, irq)
#define BUSY_WAIT_H_TIMER_FLAG    DT_INST_IRQ_BY_IDX(0, 3, flags)
/* Busy wait high timer max count is 7.78min (base on EC clock source 9.2MHz) */
#define BUSY_WAIT_TIMER_H_MAX_CNT 0xFFFFFFFFUL

#define MS_TO_COUNT(hz, ms) ((hz) * (ms) / 1000)
#define ETPSR_9200K         KHZ(9200)
#define ETPSR_32768         32768
#define ETPSR_1024          1024
#define ETPSR_32            32
#define EC_CLOCK            ETPSR_9200K
#define COUNT_1US           (EC_CLOCK / USEC_PER_SEC)

/*
 * One system (kernel) tick is as how much HW timer counts
 *
 * NOTE: Event and free run timer individually select the same clock source frequency, so they can
 *       use the same HW_CNT_PER_SYS_TICK to transform unit between HW count and system tick. If
 *       clock source frequency is different, then we should define another to transform.
 */
#define HW_CNT_PER_SYS_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Event timer max count is as how much system (kernel) tick */
#define EVEN_TIMER_MAX_CNT_SYS_TICK (EVENT_TIMER_MAX_CNT / HW_CNT_PER_SYS_TICK)

static struct k_spinlock lock;
/* Last HW count that we called sys_clock_announce() */
static volatile uint32_t last_announced_hw_cnt;
/* Last system (kernel) elapse and ticks */
static volatile uint32_t last_elapsed;
static volatile uint32_t last_ticks;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQ_BY_IDX(DT_NODELABEL(timer), 4, irq);
#endif

static uint32_t read_timer_obser(enum ext_timer_idx timer_idx)
{
	uint32_t obser;
	__unused uint8_t etnpsr;

	/* Workaround for observation register latch issue */
	obser = sys_read32(timer_base + TIMER_ETNCNTOLR(timer_idx));
	etnpsr = sys_read8(timer_base + TIMER_ETNPSR(timer_idx));
	obser = sys_read32(timer_base + TIMER_ETNCNTOLR(timer_idx));

	return obser;
}

static void ext_timer_disable(enum ext_timer_idx timer_idx)
{
	uint8_t etnctrl;

	/* Disable event timer */
	etnctrl = sys_read8(timer_base + TIMER_ETNCTRL(timer_idx));
	sys_write8(etnctrl & ~TIMER_ETNEN, timer_base + TIMER_ETNCTRL(timer_idx));
}

static void ext_timer_enable(enum ext_timer_idx timer_idx)
{
	uint8_t etnctrl;

	/* Enable and re-start event timer */
	etnctrl = sys_read8(timer_base + TIMER_ETNCTRL(timer_idx));
	sys_write8(etnctrl | TIMER_ETNRST | TIMER_ETNEN, timer_base + TIMER_ETNCTRL(timer_idx));
}

static void evt_timer_isr(const void *unused)
{
	ARG_UNUSED(unused);

	ext_timer_disable(EVENT_TIMER);
	/* W/C event timer interrupt status */
	ite_intc_isr_clear(EVENT_TIMER_IRQ);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * Get free run observer count from last time announced and transform unit to
		 * system tick
		 */
		uint32_t dticks = (~(read_timer_obser(FREE_RUN_TIMER))-last_announced_hw_cnt) /
				  HW_CNT_PER_SYS_TICK;
		last_announced_hw_cnt += (dticks * HW_CNT_PER_SYS_TICK);
		last_ticks += dticks;
		last_elapsed = 0;

		sys_clock_announce(dticks);
	} else {
		ext_timer_enable(EVENT_TIMER);
		/* Informs kernel that one system tick has elapsed */
		sys_clock_announce(MS_TO_COUNT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC, 1));
	}
}

static void free_run_timer_overflow_isr(const void *unused)
{
	ARG_UNUSED(unused);

	/* Read to clear terminal count flag */
	__unused uint8_t rc_tc = sys_read8(timer_base + TIMER_ETNCTRL(FREE_RUN_TIMER));
	/* TODO: to increment 32-bit "top half" here for software 64-bit timer emulation. */
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	uint32_t hw_cnt, next_cycs, now, dcycles;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return for non-tickless kernel system */
		return;
	}

	/* Critical section */
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Disable event timer */
	ext_timer_disable(EVENT_TIMER);

	if (ticks == K_TICKS_FOREVER) {
		/*
		 * If kernel doesn't have a timeout:
		 * 1.CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE = y (no future timer interrupts are expected),
		 *   kernel pass K_TICKS_FOREVER (0xFFFF FFFF FFFF FFFF), we handle this case in
		 *   here.
		 * 2.CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE = n (schedule timeout as far into the future
		 *   as possible), kernel pass INT_MAX (0x7FFF FFFF), we handle it in later else {}.
		 */
		k_spin_unlock(&lock, key);
		return;
	}
	/*
	 * If ticks <= 1 means the kernel wants the tick announced as soon as possible,
	 * ideally no more than one system tick in the future. So set event timer count
	 * to 1 HW tick.
	 */
	ticks = CLAMP(ticks, 1, (int32_t)EVEN_TIMER_MAX_CNT_SYS_TICK);
	next_cycs = (last_ticks + last_elapsed + ticks) * HW_CNT_PER_SYS_TICK;
	now = ~read_timer_obser(FREE_RUN_TIMER);
	if (unlikely(next_cycs <= now)) {
		hw_cnt = 1;
	} else {
		dcycles = next_cycs - now;
		hw_cnt = MIN(dcycles, EVENT_TIMER_MAX_CNT);
	}

	/* Set event timer 24-bit count */
	sys_write32(hw_cnt, timer_base + TIMER_ETNCNTLLR(EVENT_TIMER));

	/* W/C event timer interrupt status */
	ite_intc_isr_clear(EVENT_TIMER_IRQ);

	/* Enable event timer */
	ext_timer_enable(EVENT_TIMER);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for non-tickless kernel system */
		return 0;
	}

	/* Critical section */
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Get free run observer count from last time announced and transform unit to system tick */
	uint32_t dticks =
		(~(read_timer_obser(FREE_RUN_TIMER))-last_announced_hw_cnt) / HW_CNT_PER_SYS_TICK;

	last_elapsed = dticks;

	k_spin_unlock(&lock, key);

	return dticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t dticks = ~read_timer_obser(FREE_RUN_TIMER);

	return dticks;
}

static int timer_init(enum ext_timer_idx ext_timer, enum ext_clk_src_sel clock_source_sel,
		      enum ext_timer_raw_cnt raw, uint32_t ms, uint32_t irq_num, uint32_t irq_flag,
		      enum ext_timer_int with_int, enum ext_timer_start start)
{
	uint32_t hw_cnt;

	if (raw == EXT_RAW_CNT) {
		hw_cnt = ms;
	} else {
		if (clock_source_sel == EXT_PSR_32P768K) {
			hw_cnt = MS_TO_COUNT(ETPSR_32768, ms);
		} else if (clock_source_sel == EXT_PSR_1P024K) {
			hw_cnt = MS_TO_COUNT(ETPSR_1024, ms);
		} else if (clock_source_sel == EXT_PSR_32) {
			hw_cnt = MS_TO_COUNT(ETPSR_32, ms);
		} else if (clock_source_sel == EXT_PSR_EC_CLK) {
			hw_cnt = MS_TO_COUNT(ETPSR_9200K, ms);
		} else {
			LOG_ERR("Timer %d clock source error !", ext_timer);
			return -EINVAL;
		}
	}

	if (hw_cnt == 0) {
		LOG_ERR("Timer %d count shouldn't be 0 !", ext_timer);
		return -EINVAL;
	}

	/* First time enable */
	ext_timer_enable(ext_timer);
	ext_timer_disable(ext_timer);

	/* Set rising edge triggered of external timer x */
	ite_intc_irq_polarity_set(irq_num, irq_flag);

	/* Clear interrupt status of external timer x */
	ite_intc_isr_clear(irq_num);

	/* Set clock source of external timer */
	sys_write8(clock_source_sel, timer_base + TIMER_ETNPSR(ext_timer));

	/* Set count of external timer */
	sys_write32(hw_cnt, timer_base + TIMER_ETNCNTLLR(ext_timer));

	ext_timer_disable(ext_timer);
	if (start == EXT_START_TIMER) {
		ext_timer_enable(ext_timer);
	}

	if (with_int == EXT_WITH_TIMER_INT) {
		irq_enable(irq_num);
	} else {
		irq_disable(irq_num);
	}

	return 0;
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	uint32_t start = read_timer_obser(BUSY_WAIT_H_TIMER);

	if (!usec_to_wait) {
		return;
	}

	/* Decrease 1us here to calibrate our access registers latency */
	usec_to_wait--;

	for (;;) {
		if ((read_timer_obser(BUSY_WAIT_H_TIMER) - start) >= usec_to_wait) {
			break;
		}
	}
}
#endif

static int sys_clock_driver_init(void)
{
	int ret;

	/* Enable 32-bit free run timer overflow interrupt */
	IRQ_CONNECT(FREE_RUN_TIMER_IRQ, 0, free_run_timer_overflow_isr, NULL, FREE_RUN_TIMER_FLAG);
	/* Set 32-bit timer4 for free run*/
	ret = timer_init(FREE_RUN_TIMER, EXT_PSR_32P768K, EXT_RAW_CNT, FREE_RUN_TIMER_MAX_CNT,
			 FREE_RUN_TIMER_IRQ, FREE_RUN_TIMER_FLAG, EXT_WITH_TIMER_INT,
			 EXT_START_TIMER);
	if (ret < 0) {
		LOG_ERR("Init free run timer failed");
		return ret;
	}

	/* Set 24-bit timer3 for timeout event */
	IRQ_CONNECT(EVENT_TIMER_IRQ, 0, evt_timer_isr, NULL, EVENT_TIMER_FLAG);
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ret = timer_init(EVENT_TIMER, EXT_PSR_32P768K, EXT_RAW_CNT, EVENT_TIMER_MAX_CNT,
				 EVENT_TIMER_IRQ, EVENT_TIMER_FLAG, EXT_WITH_TIMER_INT,
				 EXT_NOT_START_TIMER);
	} else {
		/* Start a event timer in one system tick */
		ret = timer_init(EVENT_TIMER, EXT_PSR_32P768K, EXT_NOT_RAW_CNT,
				 MAX((1 * HW_CNT_PER_SYS_TICK), 1), EVENT_TIMER_IRQ,
				 EVENT_TIMER_FLAG, EXT_WITH_TIMER_INT, EXT_START_TIMER);
	}
	if (ret < 0) {
		LOG_ERR("Init event timer failed");
		return ret;
	}

	if (IS_ENABLED(CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT)) {
		/* Set timer5 and timer6 combinational mode for busy wait */
		sys_write8(TIMER_ETCOMB, timer_base + TIMER_ETNCTRL(BUSY_WAIT_L_TIMER));

		/*
		 * Set 32-bit timer6 to count-- every 1us
		 * NOTE: When the combinational mode. the counter observation value of timer 6 will
		 *       in incremental order.
		 */
		ret = timer_init(BUSY_WAIT_H_TIMER, EXT_PSR_EC_CLK, EXT_RAW_CNT,
				 BUSY_WAIT_TIMER_H_MAX_CNT, BUSY_WAIT_H_TIMER_IRQ,
				 BUSY_WAIT_H_TIMER_FLAG, EXT_WITHOUT_TIMER_INT, EXT_START_TIMER);
		if (ret < 0) {
			LOG_ERR("Init busy wait high timer failed");
			return ret;
		}

		/*
		 * Set 24-bit timer5 to overflow every 1us
		 * NOTE: When the timer5 count down to overflow in combinational mode, timer6
		 *       counter will automatically decrease one count and timer5 will
		 *       automatically re-start counting down from COUNT_1US. Timer5 clock
		 *       source is EC_CLOCK, so the time period from COUNT_1US to overflow is
		 *       (1 / EC_CLOCK) * (EC_CLOCK / USEC_PER_SEC) = 1us.
		 */
		ret = timer_init(BUSY_WAIT_L_TIMER, EXT_PSR_EC_CLK, EXT_RAW_CNT, COUNT_1US,
				 BUSY_WAIT_L_TIMER_IRQ, BUSY_WAIT_L_TIMER_FLAG,
				 EXT_WITHOUT_TIMER_INT, EXT_START_TIMER);
		if (ret < 0) {
			LOG_ERR("Init busy wait low timer failed");
			return ret;
		}
	}

	return 0;
}
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
