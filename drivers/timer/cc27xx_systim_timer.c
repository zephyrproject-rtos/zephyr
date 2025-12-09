/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc27xx_systim_timer

/*
 * TI SimpleLink CC27XX timer driver based on SYSTIM
 */

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/exception.h>

#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_systim.h>
#include <inc/hw_rtc.h>
#include <inc/hw_evtsvt.h>

/* Kernel tick period in microseconds (same timebase as systim) */
#define TICK_PERIOD_SYS (USEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Max number of systim ticks into the future
 *
 * Under the hood, the kernel timer uses the SysTimer whose events trigger
 * immediately if the compare value is less than 2^22 systimer ticks in the past
 * (4.194sec at 1us resolution). Therefore, the max number of SysTimer ticks you
 * can schedule into the future is 2^32 - 2^22 - 1 ticks (~= 4290 sec at 1us
 * resolution).
 */
#define SYSTIM_TIMEOUT_MAX 0xFFBFFFFFU

/* Set systim interrupt to lowest priority */
#define SYSTIM_ISR_PRIORITY IRQ_PRIO_LOWEST

/* Bit mask for the non-overlapping bits between RTC.TIME524M and SYSTIM.TIME1U */
#define RTC_TI_CC27XX_TOP_19_BITS_MASK  0xFFFFE000

static struct k_spinlock lock;

/* Keep track of systim counter at previous announcement to the kernel */
static uint32_t last_systim_count;

static void systim_isr(const void *arg);
static int sys_clock_driver_init(void);
static uint32_t sys_clock_elapsed_ticks(uint32_t current, uint32_t last);


/*
 * Set system clock timeout.
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	k_spinlock_key_t key = k_spin_lock(&lock);
	/* If timeout is necessary */
	if (ticks != K_TICKS_FOREVER) {
		/* Get current value as early as possible */
		uint32_t nowTick = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);

		/* Round down to nearest multiple of TICK_PERIOD_SYS.
		 * That is, round down to the last tick
		 */
		nowTick -= nowTick % TICK_PERIOD_SYS;

		uint32_t timeout = ticks * TICK_PERIOD_SYS;

		if (timeout > SYSTIM_TIMEOUT_MAX) {
			timeout = SYSTIM_TIMEOUT_MAX;
			/* Make sure timeout is a multiple of TICK_PERIOD_SYS */
			timeout -= timeout % TICK_PERIOD_SYS;
		}
		/* This should wrap around */
		HWREG(SYSTIM_BASE + SYSTIM_O_CH0CC) = nowTick + timeout;
	}
	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	/* Get current value as early as possible */
	uint32_t current_systim_count = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);

	int32_t elapsed_ticks = sys_clock_elapsed_ticks(current_systim_count, last_systim_count);

	k_spin_unlock(&lock, key);
	return elapsed_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
}

uint64_t sys_clock_cycle_get_64(void)
{
	/*
	 * NOTE: This function does not implement true 64-bit cycle count, only 51 bit.
	 * However, since it would take 71.4 years for the 51 bits to overflow, it
	 * is deemed acceptable.
	 */

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t low;
	uint64_t high;

	/*
	 * We combine both RTC and SYSTIM to get 51 bit cycle count.
	 * RTC is a 67-bit timer, of which we can read the first 51 bits. The SYSTIM is a
	 * 34 bit-timer. The RTC and SYSTIM are synchronized through hardware, however the RTC
	 * is only updated every ~30us, which is too rare for it to be used for Zephyr's system
	 * clock. What this means is that reading RTC.TIME1U to get a resolution of 1 us could be
	 * up to 30 us behind the actual cycle count. Therefore we use SYSTIM.TIME1U for these
	 * least significant bits that are updated more often than 30 us,
	 * since SYSTIM is updated immediately on count.
	 * We use RTC.TIME524M to get the most significant bits.
	 */

	high = HWREG(RTC_BASE + RTC_O_TIME524M);
	low  = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);

	/*
	 * The 13 most significant bits of TIME1U and the 13 least significant bits of TIME524M
	 * "overlaps". There is a possibility that the 13 bits in low are incremented, and we
	 * call this function before the RTC is updated. Normally, if the overlapping bits in
	 * TIME1U are numerically higher than those in TIME524U, no action is needed. However, if
	 * the increment of TIME1U makes the counter overflow, this needs to be accounted for.
	 * We check the overlapping bits, if those in "low" are numerically higher than those in
	 * "high", no action is needed. If they are numerically lower, then this means TIME1U has
	 * overflowed and the RTC is not yet updated. We do not want to wait for the RTC to
	 * update, so we manually increment the high part by add 1 to the first non-overlapping
	 * bit (bit 13). We can never have the opposite, where those in "high" are numerically
	 * higher than those in "low", since the RTC is always updated to the current value of
	 * the more frequently updated SYSTIM timer.
	 */
	if ((high & 0x1FFF) > (low  >> 19)) {

		high += 1<<13;

	}
	/*
	 * We mask out TIME524M[12:0] bits since they overlap, and shift the first
	 * valid bit (bit 13) to position 32 in the resulting 64 bit cycle count. We do this
	 * by left shifting the masked valued 32-13 = 19 positions.
	 */

	high = (high & RTC_TI_CC27XX_TOP_19_BITS_MASK) << 19;

	k_spin_unlock(&lock, key);

	return high | low;
}

void systim_isr(const void *arg)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	/* Get current value as early as possible */
	uint32_t current_systim_count = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
	uint32_t elapsed_ticks = sys_clock_elapsed_ticks(current_systim_count, last_systim_count);
	last_systim_count = current_systim_count;
	k_spin_unlock(&lock, key);
	sys_clock_announce(elapsed_ticks);


	/* Do not re-arm systim. Zephyr will do so through sys_clock_set_timeout */
}

static int sys_clock_driver_init(void)
{
	uint32_t nowTick;

	/* Get current value as early as possible */
	nowTick = HWREG(SYSTIM_BASE + SYSTIM_O_TIME1U);
	last_systim_count = nowTick;

	/* Clear any pending interrupts on SysTimer channel 0 */
	HWREG(SYSTIM_BASE + SYSTIM_O_ICLR) = SYSTIM_ICLR_EV0_CLR;

	/*
	 * Configure SysTimer channel 0 to compare mode with timer
	 * resolution of 1 us.
	 */
	HWREG(SYSTIM_BASE + SYSTIM_O_CH0CFG) = 0;

	/* Make SysTimer halt on CPU debug halt */
	HWREG(SYSTIM_BASE + SYSTIM_O_EMU) = SYSTIM_EMU_HALT_STOP;

	HWREG(EVTSVT_BASE + EVTSVT_O_CPUIRQ16SEL) = EVTSVT_CPUIRQ16SEL_PUBID_SYSTIM0;

	/*
	 * Set IMASK for channel 0. IMASK is used by the power driver to know
	 * which systimer channels are active.
	 */
	HWREG(SYSTIM_BASE + SYSTIM_O_IMSET) = SYSTIM_IMSET_EV0_SET;

	/* This should wrap around and set a maximum timeout */
	HWREG(SYSTIM_BASE + SYSTIM_O_CH0CC) = nowTick + SYSTIM_TIMEOUT_MAX;

	/* Take configurable interrupt IRQ16 for systimer */
	IRQ_CONNECT(CPUIRQ16_IRQn, SYSTIM_ISR_PRIORITY, systim_isr, 0, 0);
	irq_enable(CPUIRQ16_IRQn);

	return 0;
}

static uint32_t sys_clock_elapsed_ticks(uint32_t current, uint32_t last)
{
	if (current >= last) {
		return (current / TICK_PERIOD_SYS) - (last / TICK_PERIOD_SYS);
	} else {
		return ((0xFFFFFFFF - last) / TICK_PERIOD_SYS) +
		(current / TICK_PERIOD_SYS);
	}
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
