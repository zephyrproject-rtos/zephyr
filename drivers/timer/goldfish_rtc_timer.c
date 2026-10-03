/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define GOLDFISH_RTC_NODE DT_CHOSEN(zephyr_system_timer)
#define GOLDFISH_RTC_BASE DT_REG_ADDR(GOLDFISH_RTC_NODE)
#define GOLDFISH_RTC_IRQ  DT_IRQN(GOLDFISH_RTC_NODE)

#define GOLDFISH_RTC_TIME_LOW        0x00U
#define GOLDFISH_RTC_TIME_HIGH       0x04U
#define GOLDFISH_RTC_ALARM_LOW       0x08U
#define GOLDFISH_RTC_ALARM_HIGH      0x0cU
#define GOLDFISH_RTC_IRQ_ENABLED     0x10U
#define GOLDFISH_RTC_CLEAR_INTERRUPT 0x1cU
#define GOLDFISH_RTC_MMIO_SIZE       0x20U

#define GOLDFISH_RTC_BIG_ENDIAN DT_PROP(GOLDFISH_RTC_NODE, big_endian)
#define GOLDFISH_RTC_COUNT_HZ   1000000000ULL
#define GOLDFISH_RTC_COUNTS_PER_TICK \
	(GOLDFISH_RTC_COUNT_HZ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define GOLDFISH_RTC_COUNTS_PER_CYCLE \
	(GOLDFISH_RTC_COUNT_HZ / CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)

BUILD_ASSERT(DT_REG_SIZE(GOLDFISH_RTC_NODE) >= GOLDFISH_RTC_MMIO_SIZE,
	     "Goldfish RTC register block is too small");
BUILD_ASSERT((GOLDFISH_RTC_COUNT_HZ %
	      CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) == 0U,
	     "system cycle frequency must divide Goldfish RTC frequency");
BUILD_ASSERT(GOLDFISH_RTC_COUNTS_PER_TICK > 0U,
	     "kernel tick frequency exceeds Goldfish RTC frequency");

static struct k_spinlock goldfish_rtc_lock;
static uint64_t goldfish_rtc_count_base;
static uint64_t goldfish_rtc_last_count;

static uint32_t goldfish_rtc_read32(uint32_t offset)
{
	uint32_t value = sys_read32(GOLDFISH_RTC_BASE + offset);

	if (GOLDFISH_RTC_BIG_ENDIAN) {
		return sys_be32_to_cpu(value);
	}

	return sys_le32_to_cpu(value);
}

static void goldfish_rtc_write32(uint32_t value, uint32_t offset)
{
	if (GOLDFISH_RTC_BIG_ENDIAN) {
		value = sys_cpu_to_be32(value);
	} else {
		value = sys_cpu_to_le32(value);
	}

	sys_write32(value, GOLDFISH_RTC_BASE + offset);
}

static uint64_t goldfish_rtc_count(void)
{
	k_spinlock_key_t key = k_spin_lock(&goldfish_rtc_lock);
	uint32_t low;
	uint32_t high;

	/* Reading TIME_LOW latches TIME_HIGH. */
	low = goldfish_rtc_read32(GOLDFISH_RTC_TIME_LOW);
	high = goldfish_rtc_read32(GOLDFISH_RTC_TIME_HIGH);

	k_spin_unlock(&goldfish_rtc_lock, key);

	return ((uint64_t)high << 32) | low;
}

static void goldfish_rtc_set_alarm(uint64_t deadline)
{
	/* Writing ALARM_LOW commits and arms the complete alarm value. */
	goldfish_rtc_write32((uint32_t)(deadline >> 32),
			     GOLDFISH_RTC_ALARM_HIGH);
	goldfish_rtc_write32((uint32_t)deadline, GOLDFISH_RTC_ALARM_LOW);
}

static void goldfish_rtc_isr(const void *arg)
{
	k_spinlock_key_t key = sys_clock_lock();
	uint64_t now;
	uint32_t ticks;

	ARG_UNUSED(arg);

	goldfish_rtc_write32(1U, GOLDFISH_RTC_CLEAR_INTERRUPT);

	now = goldfish_rtc_count();
	ticks = (uint32_t)((now - goldfish_rtc_last_count) /
			   GOLDFISH_RTC_COUNTS_PER_TICK);
	goldfish_rtc_last_count +=
		(uint64_t)ticks * GOLDFISH_RTC_COUNTS_PER_TICK;

	goldfish_rtc_set_alarm(goldfish_rtc_last_count +
			       GOLDFISH_RTC_COUNTS_PER_TICK);

	sys_clock_announce_locked(ticks, key);
}

uint32_t sys_clock_elapsed(void)
{
	return 0U;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return (goldfish_rtc_count() - goldfish_rtc_count_base) /
		GOLDFISH_RTC_COUNTS_PER_CYCLE;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)sys_clock_cycle_get_64();
}

static int goldfish_rtc_init(void)
{
	IRQ_CONNECT(GOLDFISH_RTC_IRQ, 0, goldfish_rtc_isr, NULL, 0);

	goldfish_rtc_last_count = goldfish_rtc_count();
	goldfish_rtc_count_base = goldfish_rtc_last_count;
	goldfish_rtc_set_alarm(goldfish_rtc_last_count +
			       GOLDFISH_RTC_COUNTS_PER_TICK);

	irq_enable(GOLDFISH_RTC_IRQ);
	goldfish_rtc_write32(1U, GOLDFISH_RTC_IRQ_ENABLED);

	return 0;
}

SYS_INIT(goldfish_rtc_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
