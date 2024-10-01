/* Copyright 2023 The ChromiumOS Authors
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/spinlock.h>
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>

#define OSTIMER64_BASE DT_REG_ADDR(DT_NODELABEL(ostimer64))
#define OSTIMER_BASE DT_REG_ADDR(DT_NODELABEL(ostimer0))

/*
 * This device has a LOT of timer hardware.  There are SIX
 * instantiated devices, with THREE different interfaces!  Not
 * including the three Xtensa CCOUNT timers!
 *
 * In practice only "ostimer0" is used as an interrupt source by the
 * original SOF code, and the "ostimer64" and "platform" timers
 * reflect the same underlying clock (though they're different
 * counters with different values).  There is also a "ptimer" device,
 * which is unused by SOF and not exercised by this driver.
 *
 * The driver architecture itself is sort of a hybrid of what other
 * Zephyr drivers use: there is no (or at least no documented)
 * comparator facility.  The "ostimer64" is used as the system clock,
 * which is a 13 MHz 64 bit up-counter.  But timeout interrupts are
 * delivered by ostimers[0], which is a 32 bit (!) down-counter (!!)
 * running at twice (!!!) the rate: 26MHz.  Testing shows they're
 * slaved the same underlying clock -- they don't skew relative to
 * each other.
 */
struct mtk_ostimer {
	unsigned int con;
	unsigned int rst;
	unsigned int cur;
	unsigned int irq_ack;
};

struct mtk_ostimer64 {
	unsigned int con;
	unsigned int init_l;
	unsigned int init_h;
	unsigned int cur_l;
	unsigned int cur_h;
	unsigned int tval_h;
	unsigned int irq_ack;
};

#define OSTIMER64 (*(volatile struct mtk_ostimer64 *)OSTIMER64_BASE)

#define OSTIMERS ((volatile struct mtk_ostimer *)OSTIMER_BASE)

#define OSTIMER_CON_ENABLE BIT(0)
#define OSTIMER_CON_CLKSRC_MASK 0x30
#define OSTIMER_CON_CLKSRC_32K  0x00 /*  32768 Hz */
#define OSTIMER_CON_CLKSRC_26M  0x10 /*  26 MHz */
#define OSTIMER_CON_CLKSRC_BCLK 0x20 /*  CPU speed, 720 MHz */
#define OSTIMER_CON_CLKSRC_PCLK 0x30 /*  ~312 MHz experimentally */

#define OSTIMER_IRQ_ACK_ENABLE BIT(4) /*  read = status, write = enable */
#define OSTIMER_IRQ_ACK_CLEAR  BIT(5)

#define OST64_HZ 13000000U
#define OST_HZ 26000000U
#define OST64_PER_TICK (OST64_HZ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define OST_PER_TICK (OST_HZ / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((0xffffffffU - OST_PER_TICK) / OST_PER_TICK)
#define CYC64_MAX (0xffffffff - OST64_PER_TICK)

static struct k_spinlock lock;

static uint64_t last_announce;

uint32_t sys_clock_cycle_get_32(void)
{
	return OSTIMER64.cur_l;
}

uint64_t sys_clock_cycle_get_64(void)
{
	uint32_t l, h0, h1;

	do {
		h0 = OSTIMER64.cur_h;
		l  = OSTIMER64.cur_l;
		h1 = OSTIMER64.cur_h;
	} while (h0 != h1);

	return (((uint64_t)h0) << 32) | l;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	/* Compute desired expiration time */
	uint64_t now = sys_clock_cycle_get_64();
	uint64_t end = now + CLAMP(ticks - 1, 0, MAX_TICKS) * OST64_PER_TICK;
	uint32_t dt = (uint32_t)MIN(end - last_announce, CYC64_MAX);

	/* Round up to tick boundary */
	dt = ((dt + OST64_PER_TICK - 1) / OST64_PER_TICK) * OST64_PER_TICK;

	/* Convert to "fast" OSTIMER[0] cycles! */
	uint32_t cyc = 2 * (dt - (uint32_t)(now - last_announce));

	/* Writes to RST need to be done when the device is disabled,
	 * and automatically reset CUR (which reads zero while disabled)
	 */
	OSTIMERS[0].con &= ~OSTIMER_CON_ENABLE;
	OSTIMERS[0].rst = cyc;
	OSTIMERS[0].irq_ack |= OSTIMER_IRQ_ACK_CLEAR;
	OSTIMERS[0].irq_ack |= OSTIMER_IRQ_ACK_ENABLE;
	OSTIMERS[0].con |= OSTIMER_CON_ENABLE;
}

uint32_t sys_clock_elapsed(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret;

	ret = (uint32_t)((sys_clock_cycle_get_64() - last_announce)
			 / OST64_PER_TICK);
	k_spin_unlock(&lock, key);
	return ret;
}

static void timer_isr(__maybe_unused void *arg)
{
	/* Note: no locking.  As it happens, on MT8195/8186/8188 all
	 * Zephyr-usable interrupts are delivered at the same level.
	 * So we can't be preempted and there's actually no need to
	 * take a spinlock here.  But ideally we should verify/detect
	 * this instead of trusting blindly; this is fragile if future
	 * devices add nested interrupts.
	 */
	uint64_t dcyc = sys_clock_cycle_get_64() - last_announce;
	uint64_t ticks = dcyc / OST64_PER_TICK;

	/* Leave the device disabled after clearing the interrupt,
	 * sys_clock_set_timeout() is responsible for turning it back
	 * on.
	 */
	OSTIMERS[0].irq_ack |=  OSTIMER_IRQ_ACK_CLEAR;
	OSTIMERS[0].con     &= ~OSTIMER_CON_ENABLE;
	OSTIMERS[0].irq_ack &= ~OSTIMER_IRQ_ACK_ENABLE;

	last_announce += ticks * OST64_PER_TICK;
	sys_clock_announce(ticks);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		sys_clock_set_timeout(1, false);
	}
}

static int mtk_adsp_timer_init(void)
{
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(ostimer0)), 0, timer_isr, 0, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(ostimer0)));

	/* Disable all timers */
	for (int i = 0; i < 4; i++) {
		OSTIMERS[i].con &= ~OSTIMER_CON_ENABLE;
		OSTIMERS[i].irq_ack |= OSTIMER_IRQ_ACK_CLEAR;
		OSTIMERS[i].irq_ack &= ~OSTIMER_IRQ_ACK_ENABLE;
	}

	/* Set them up to use the same clock.  Note that OSTIMER64 has
	 * a built-in divide by two (or it's configurable and I don't
	 * know the register) and exposes a 13 MHz counter!
	 */
	OSTIMERS[0].con = ((OSTIMERS[0].con & ~OSTIMER_CON_CLKSRC_MASK)
			   | OSTIMER_CON_CLKSRC_26M);
	OSTIMERS[0].con |= OSTIMER_CON_ENABLE;

	/* Clock is free running and survives reset, doesn't start at zero */
	last_announce = sys_clock_cycle_get_64();

	return 0;
}

SYS_INIT(mtk_adsp_timer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
