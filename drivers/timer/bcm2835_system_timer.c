/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Broadcom BCM2835 free-running system timer (1 MHz). A compare-channel match
 * raises an interrupt. Channels 0 and 2 are reserved by the VideoCore firmware,
 * so channel 3 is used for the kernel tick. Supports both tickless and ticking
 * kernels.
 *
 * The compare is a 32-bit EQUALITY match on the counter's low word (ST_CLO): it
 * fires only when ST_CLO == Cn. A target the counter has already passed would
 * not match again until the low word wraps (~71 min at 1 MHz), so every compare
 * write is kept at least MIN_DELAY cycles ahead of the live counter.
 */

#define DT_DRV_COMPAT brcm_bcm2835_system_timer

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

#define TIMER_IRQ DT_INST_IRQN(0)
#define TIMER_CH  3

#define ST_CS_OFF      0x00 /* control/status   */
#define ST_CLO_OFF     0x04 /* counter low      */
#define ST_COMPARE_OFF (0x0c + TIMER_CH * 4)
#define ST_MATCH       BIT(TIMER_CH)

static mm_reg_t st_base;

#define CYC_PER_TICK ((uint32_t)k_ticks_to_cyc_floor32(1))

/* Largest tick count whose cycle span still fits the 32-bit compare. */
#define MAX_TICKS ((uint32_t)(UINT32_MAX / CYC_PER_TICK) - 2)

/*
 * Minimum cycles a freshly programmed compare must lead the live counter by,
 * covering the read-modify-write latency so the equality match is not missed.
 */
#define MIN_DELAY 16

/* ST_CLO value at the most recently announced tick boundary. */
static uint32_t last_cycle;
/* Ticks reported by the most recent sys_clock_elapsed() (tickless only). */
static uint32_t last_elapsed;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ;
#endif

/*
 * Program the compare register, stepping it forward in whole ticks until it is
 * at least MIN_DELAY ahead of the live counter. Stepping by a tick (rather than
 * to "now + MIN_DELAY") keeps the compare on a tick boundary so the period does
 * not drift; a late ISR then simply announces the extra elapsed ticks.
 */
static void set_compare(uint32_t target)
{
	while ((int32_t)(target - sys_read32(st_base + ST_CLO_OFF)) < (int32_t)MIN_DELAY) {
		target += CYC_PER_TICK;
	}
	sys_write32(target, st_base + ST_COMPARE_OFF);
}

static void bcm2835_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Acknowledge the match. */
	sys_write32(ST_MATCH, st_base + ST_CS_OFF);

	uint32_t now = sys_read32(st_base + ST_CLO_OFF);
	uint32_t ticks = (now - last_cycle) / CYC_PER_TICK;

	last_cycle += ticks * CYC_PER_TICK;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Re-arm for the next periodic tick. */
		set_compare(last_cycle + CYC_PER_TICK);
	}

	sys_clock_announce((int32_t)ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? (int32_t)MAX_TICKS : ticks;
	ticks = CLAMP(ticks, 0, (int32_t)MAX_TICKS);

	/*
	 * Fire `ticks` ticks past the position the kernel last observed via
	 * sys_clock_elapsed() (last_cycle + last_elapsed). The kernel already
	 * applies its own round-up, so the driver adds none of its own.
	 */
	set_compare(last_cycle + (last_elapsed + (uint32_t)ticks) * CYC_PER_TICK);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	last_elapsed = (sys_read32(st_base + ST_CLO_OFF) - last_cycle) / CYC_PER_TICK;
	return last_elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_read32(st_base + ST_CLO_OFF);
}

static int bcm2835_timer_init(void)
{
	/*
	 * Map the ST register block. The SoC selects KERNEL_DIRECT_MAP, so
	 * device_map() takes the identity-map fast path and works pre-SYS_INIT.
	 */
	device_map(&st_base, DT_INST_REG_ADDR(0), DT_INST_REG_SIZE(0), K_MEM_CACHE_NONE);

	IRQ_CONNECT(TIMER_IRQ, 0, bcm2835_timer_isr, NULL, 0);

	last_cycle = sys_read32(st_base + ST_CLO_OFF);
	set_compare(last_cycle + CYC_PER_TICK);
	sys_write32(ST_MATCH, st_base + ST_CS_OFF); /* clear any stale match */
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(bcm2835_timer_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
