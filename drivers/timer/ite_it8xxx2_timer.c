/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/timer/system_timer.h>
#include <soc.h>
#include <sys/printk.h>

/**
 * Macro Define
 */
#define EXT_TIMER_BASE		(DT_REG_ADDR_BY_IDX(DT_NODELABEL(timer), 0))
#define EXT_CTL_B		(EXT_TIMER_BASE + 0x10)
#define EXT_PSC_B		(EXT_TIMER_BASE + 0x11)
#define EXT_LLR_B		(EXT_TIMER_BASE + 0x14)
#define EXT_LHR_B		(EXT_TIMER_BASE + 0x15)
#define EXT_LH2R_B		(EXT_TIMER_BASE + 0x16)
#define EXT_LH3R_B		(EXT_TIMER_BASE + 0x17)
#define EXT_CNTO_B		(EXT_TIMER_BASE + 0x48)

#define CTIMER_HW_TIMER_INDEX	EXT_TIMER_3
#define ETIMER_HW_TIMER_INDEX	EXT_TIMER_5
#define RTIMER_HW_TIMER_INDEX	EXT_TIMER_7
#define CYC_PER_TICK		(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC \
					/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS		((0x00ffffffu - CYC_PER_TICK) / CYC_PER_TICK)

#define MAX_TIMER_NUM			8
#define REG_ADDR_OFFSET(idx)		(idx * MAX_TIMER_NUM)
#define IDX_SHIFT(idx, rsh, lsh)	((idx >> rsh) << lsh)

enum _EXT_TIMER_PRESCALE_TYPE_ {
	ET_PSR_32K,
	ET_PSR_1K,
	ET_PSR_32,
	ET_PSR_8M,
};
enum _EXT_TIMER_IDX_ {
	EXT_TIMER_3 = 0,	/* ctimer */
	EXT_TIMER_4,		/* ctimer */
	EXT_TIMER_5,		/* etimer */
	EXT_TIMER_6,		/* NULL */
	EXT_TIMER_7,		/* rtimer */
	EXT_TIMER_8		/* NULL */
};

/* Be careful of overflow issue */
#define MILLI_SEC_TO_COUNT(hz, ms) ((hz) * (ms) / 1000)
#define MICRO_SEC_TO_COUNT(hz, us) ((hz) * (us) / 1000000)

/**
 * ITE timer control api
 */
static void ite_timer_reload(uint8_t idx, uint32_t cnt)
{
	/* timer_start */
	sys_set_bit((EXT_CTL_B + REG_ADDR_OFFSET(idx)), 0);
	sys_write8(((cnt >> 24) & 0xFF), (EXT_LH3R_B + REG_ADDR_OFFSET(idx)));
	sys_write8(((cnt >> 16) & 0xFF), (EXT_LH2R_B + REG_ADDR_OFFSET(idx)));
	sys_write8(((cnt >> 8) & 0xFF), (EXT_LHR_B + REG_ADDR_OFFSET(idx)));
	sys_write8(((cnt >> 0) & 0xFF), (EXT_LLR_B + REG_ADDR_OFFSET(idx)));
}

/* The following function:
 * disable, enable, check_flag, clear_flag, wait,
 * can be used only for Timer #3 ~ #7
 */
static void ite_timer_disable(uint8_t idx)
{
	CLEAR_MASK(IER19, (BIT(3 + (idx))));
}

static void ite_timer_enable(uint8_t idx)
{
	SET_MASK(IER19, (BIT(3 + (idx))));
}

static void ite_timer_clear_flag(uint8_t idx)
{
	ISR19 = BIT(3 + (idx));
}

/**
 * timer_init()
 */
static int timer_init(uint8_t idx, uint8_t psr, uint8_t initial_state,
	       uint8_t enable_isr, uint32_t cnt)
{
	/* Setup Triggered Mode -> Rising-Edge Trig. */
	if (idx != EXT_TIMER_8) {
		IELMR19 |= BIT(3 + idx);
		IPOLR19 &= (~(BIT(3 + idx)));
	} else {
		IELMR10 |= BIT(0);
		IPOLR10 &= (~(BIT(0)));
	}

	/* Setup prescaler */
	sys_write8(psr, (EXT_PSC_B + REG_ADDR_OFFSET(idx)));

	/* Reload counter */
	ite_timer_reload(idx, cnt);

	/* Start counting or not */
	if (initial_state) {
		/* timer restart */
		/* timer_stop */
		sys_clear_bit((EXT_CTL_B + REG_ADDR_OFFSET(idx)), 0);

		/* timer_start */
		sys_set_bit((EXT_CTL_B + REG_ADDR_OFFSET(idx)), 0);
	} else {
		/* timer_stop */
		sys_clear_bit((EXT_CTL_B + REG_ADDR_OFFSET(idx)), 0);
	}

	/* Enable ISR or not & Clear flag */
	if (idx != EXT_TIMER_8) {
		if (enable_isr) {
			ite_timer_enable(idx);
		} else {
			ite_timer_disable(idx);
		}
		ite_timer_clear_flag(idx);
	} else {
		if (enable_isr) {
			SET_MASK(IER10, BIT(0));
		} else {
			CLEAR_MASK(IER10, BIT(0));
		}
		ISR10 = BIT(0);
	}
	return 0;
}

static int timer_init_ms(uint8_t idx, uint8_t psr, uint8_t initial_state,
		  uint8_t enable_isr, uint32_t u32MilliSec)
{
	uint32_t cnt;

	if (psr == ET_PSR_32K) {
		cnt = MILLI_SEC_TO_COUNT(32768, u32MilliSec);
	} else if (psr == ET_PSR_1K) {
		cnt = MILLI_SEC_TO_COUNT(1024, u32MilliSec);
	} else if (psr == ET_PSR_32) {
		cnt = MILLI_SEC_TO_COUNT(32, u32MilliSec);
	} else if (psr == ET_PSR_8M) {
		cnt = u32MilliSec * 8000; /* fixed overflow issue */
	} else {
		return -1;
	}

	/* 24-bits only */
	if (cnt >> 24) {
		return -2;
	}
	return timer_init(idx, psr, initial_state, enable_isr, cnt);
}

static void timer_init_combine(uint8_t idx, uint8_t bEnable)
{
	if (bEnable) {
		sys_set_bit((EXT_CTL_B + IDX_SHIFT(idx, 1, (1 + 3))), 3);
	} else {
		sys_clear_bit((EXT_CTL_B + IDX_SHIFT(idx, 1, (1 + 3))), 3);
	}
}

static uint32_t get_timer_combine_count(uint8_t idx)
{
	return sys_read32(EXT_CNTO_B + ((IDX_SHIFT(idx, 1, 1) + 1) * 4));
}

static void timer_count_reset(uint8_t idx, uint32_t cnt)
{
	/* Reload counter */
	ite_timer_reload(idx, cnt);

	/* Start counting or not */
	/* timer_stop */
	sys_clear_bit((EXT_CTL_B + REG_ADDR_OFFSET(idx)), 0);
	/* timer_start */
	sys_set_bit((EXT_CTL_B + REG_ADDR_OFFSET(idx)), 0);
}

static struct k_spinlock lock;
static volatile uint32_t accumulated_cycle_count;
static void timer_isr(const void *unused)
{
	ARG_UNUSED(unused);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* timer_stop */
	sys_clear_bit((EXT_CTL_B + ((ETIMER_HW_TIMER_INDEX) * MAX_TIMER_NUM)),
				0);
	uint32_t dticks = (get_timer_combine_count(CTIMER_HW_TIMER_INDEX)
				- accumulated_cycle_count) / CYC_PER_TICK;
	accumulated_cycle_count += dticks * CYC_PER_TICK;
	k_spin_unlock(&lock, key);
	z_clock_announce(dticks);
}

int z_clock_driver_init(const struct device *device)
{
	timer_init_combine(CTIMER_HW_TIMER_INDEX, TRUE);
	timer_init(CTIMER_HW_TIMER_INDEX, ET_PSR_32K, TRUE, FALSE, 0);
	irq_connect_dynamic(DT_IRQ_BY_IDX(DT_NODELABEL(timer), 5, irq),
				0, timer_isr, NULL,
				DT_IRQ_BY_IDX(DT_NODELABEL(timer), 5, flags));
	timer_init_ms(ETIMER_HW_TIMER_INDEX, ET_PSR_32K, FALSE, TRUE, 0);
	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	k_spinlock_key_t key = k_spin_lock(&lock);

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks, (int32_t)MAX_TICKS), 1);
	timer_count_reset(ETIMER_HW_TIMER_INDEX, ticks * CYC_PER_TICK);
	k_spin_unlock(&lock, key);
}

uint32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t ret = (get_timer_combine_count(CTIMER_HW_TIMER_INDEX)
			- accumulated_cycle_count) / CYC_PER_TICK;
	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t z_timer_cycle_get_32(void)
{
	return get_timer_combine_count(CTIMER_HW_TIMER_INDEX);
}
