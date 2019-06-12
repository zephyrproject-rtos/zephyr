/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2019 Microchip Technology Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>

BUILD_ASSERT_MSG(!IS_ENABLED(CONFIG_SMP), "XEC RTOS timer doesn't support SMP");
BUILD_ASSERT_MSG(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 32768,
		 "XEC RTOS timer HW frequency is fixed at 32768");

#define DEBUG_RTOS_TIMER 0

#if DEBUG_RTOS_TIMER != 0
/* Enable feature to halt timer on JTAG/SWD CPU halt */
#define TIMER_START_VAL (MCHP_RTMR_CTRL_BLK_EN | MCHP_RTMR_CTRL_START \
			 | MCHP_RTMR_CTRL_HW_HALT_EN)
#else
#define TIMER_START_VAL (MCHP_RTMR_CTRL_BLK_EN | MCHP_RTMR_CTRL_START)
#endif

/*
 * Overview:
 *
 * This driver enables the Microchip XEC 32KHz based RTOS timer as the Zephyr
 * system timer. It supports both legacy ("tickful") mode as well as
 * TICKLESS_KERNEL. The XEC RTOS timer is a down counter with a fixed
 * frequency of 32768 Hz. The driver is based upon the Intel local APIC
 * timer driver.
 * Configuration:
 *
 * CONFIG_MCHP_XEC_RTOS_TIMER=y
 *
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=<hz> must be set to 32768.
 *
 * To reduce truncation errors from accumalating due to conversion
 * to/from time, ticks, and HW cycles set ticks per second equal to
 * the frequency. With tickless kernel mode enabled the kernel will not
 * program a periodic timer at this fast rate.
 * CONFIG_SYS_CLOCK_TICKS_PER_SEC=32768
 */

#define CYCLES_PER_TICK \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MOD31_MASK	0x7FFFFFFFUL

/* RTOS timer is 32 bit */
#define TIMER_MAX	0xFFFFFFFFU

#define TIMER_COUNT_MASK	0x7FFFFFFFUL

/* max number of ticks we can load into the timer in one shot */
#define MAX_TICKS (TIMER_MAX / CYCLES_PER_TICK)

/*
 * The spinlock protects all access to the RTMR registers, as well as
 * 'total_cycles', 'last_announcement', and 'cached_icr'.
 *
 * One important invariant that must be observed: `total_cycles` + `cached_icr`
 * is always an integral multiple of CYCLE_PER_TICK; this is, timer interrupts
 * are only ever scheduled to occur at tick boundaries.
 */

static struct k_spinlock lock;
static u32_t total_cycles;
static u32_t cached_icr = CYCLES_PER_TICK;

/*
 * read the timer count register and mask value to strip bit[31].
 */
static INLINE u32_t tcount(void)
{
	return RTMR_REGS->CNT & TIMER_COUNT_MASK;
}

/*
 * Kernel converts some u32_t return values to s32_t.
 * Implement modular addition (a + b) mod 0x80000000 to
 * insure sum does not have bit[31] set.
 * Modulus is a power of 2 allowing us to use masking.
 */
static u32_t mod_add(u32_t a, u32_t b)
{
	return (a + b) & MOD31_MASK;
}

static u32_t mod_sub(u32_t a, u32_t b)
{
	return ((a & MOD31_MASK) - (b & MOD31_MASK)) & MOD31_MASK;
}

/*
 * Restart RTOS timer handling the case where the timer is currently running.
 * This sequence has been vetted by the RTOS timer HW designer.
 * 1. Write 0 to CONTROL register which asynchronously stops the timer and
 *    clears all its registers.
 * 2. Set CONTROL block enable bit to ungate clocks to the block.
 * 3. Write new count down value to PRELOAD register.
 * 4. Write CONTROL to start timer in one-shot mode.
 * We added a check after the timer is stopped for active interrupt status.
 * If interrupt status is active we clear it in the aggregator and NVIC.
 * This handles the scenario in z_clock_set_timeout where the timer expires
 * while inside the spin lock. We don't want a spurious interrupt. We can't
 * use the method the local APIC driver uses because this timer takes 1 32KHz
 * clock period to load the new count value into its COUNT register.
 * The pending interrupt would fire and the ISR would observed COUNT==0.
 */
static void timer_restart(u32_t val)
{
	RTMR_REGS->CTRL = 0;
	RTMR_REGS->CTRL = MCHP_RTMR_CTRL_BLK_EN;
	if (GIRQ23_REGS->SRC & MCHP_RTMR_GIRQ_VAL) {
		GIRQ23_REGS->SRC = MCHP_RTMR_GIRQ_VAL;
		NVIC_ClearPendingIRQ(RTMR_IRQn);
	}
	RTMR_REGS->PRLD = val;
	RTMR_REGS->CTRL = TIMER_START_VAL;
}

#ifdef CONFIG_TICKLESS_KERNEL

static u32_t last_announcement;	/* last time we called z_clock_announce() */

/*
 * Request a timeout n Zephyr ticks in the future from now.
 * Requested number of ticks in the future of n <= 1 means the kernel wants
 * the tick announced as soon as possible, ideally no more than one tick
 * in the future.
 *
 * Per comment below we don't clear RTMR pending interrupt.
 * RTMR counter register is read-only and is loaded from the preload
 * register by a 0->1 transition of the control register start bit.
 * Writing a new value to preload only takes effect once the count
 * register reaches 0.
 */
void z_clock_set_timeout(s32_t n, bool idle)
{
	ARG_UNUSED(idle);

	u32_t ccr, temp;
	int   full_ticks;	/* number of complete ticks we'll wait */
	u32_t full_cycles;	/* full_ticks represented as cycles */
	u32_t partial_cycles;	/* number of cycles to first tick boundary */

	if (n < 1) {
		full_ticks = 0;
	} else if ((n == K_FOREVER) || (n > MAX_TICKS)) {
		full_ticks = MAX_TICKS - 1;
	} else {
		full_ticks = n - 1;
	}

	full_cycles = full_ticks * CYCLES_PER_TICK;

	/*
	 * There's a wee race condition here. The timer may expire while
	 * we're busy reprogramming it; an interrupt will be queued at the
	 * NVIC and the ISR will be called too early, roughly right
	 * after we unlock, and not because the count we just programmed has
	 * counted down. We can detect this situation only by using one-shot
	 * mode. The counter will be 0 for a "real" interrupt and non-zero
	 * if we have restarted the timer here.
	 */

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = tcount();
	temp = mod_sub(cached_icr, ccr);
	total_cycles = mod_add(total_cycles, temp);
	partial_cycles = CYCLES_PER_TICK - (total_cycles % CYCLES_PER_TICK);
	cached_icr = full_cycles + partial_cycles;
	timer_restart(cached_icr);

	k_spin_unlock(&lock, key);
}

/*
 * Return the number of Zephyr ticks elapsed from last call to
 * z_clock_announce in the ISR.
 */
u32_t z_clock_elapsed(void)
{
	u32_t ccr, temp;
	u32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);
	ccr = tcount();
	ticks = mod_sub(total_cycles, last_announcement);
	temp = mod_sub(cached_icr, ccr);
	ticks = mod_add(ticks, temp);
	k_spin_unlock(&lock, key);
	ticks /= CYCLES_PER_TICK;

	return ticks;
}

static void xec_rtos_timer_isr(void *arg)
{
	ARG_UNUSED(arg);

	u32_t cycles;
	s32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);

	GIRQ23_REGS->SRC = MCHP_RTMR_GIRQ_VAL;

	/* Restart the timer as early as possible to minimize drift... */
	timer_restart(MAX_TICKS * CYCLES_PER_TICK);

	cycles = cached_icr;
	cached_icr = MAX_TICKS * CYCLES_PER_TICK;

	total_cycles = mod_add(total_cycles, cycles);
	ticks = mod_sub(total_cycles, last_announcement);
	ticks /= CYCLES_PER_TICK;
	last_announcement = total_cycles;
	k_spin_unlock(&lock, key);
	z_clock_announce(ticks);
	GPIO_CTRL_REGS->CTRL_0015 = 0x10240ul;
}

#else

/* Non-tickless kernel build. */

static void xec_rtos_timer_isr(void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	GIRQ23_REGS->SRC = MCHP_RTMR_GIRQ_VAL;
	total_cycles = mod_add(total_cycles, CYCLES_PER_TICK);
	timer_restart(cached_icr);
	k_spin_unlock(&lock, key);

	z_clock_announce(1);
}

u32_t z_clock_elapsed(void)
{
	return 0U;
}

#endif /* CONFIG_TICKLESS_KERNEL */

/*
 * Warning RTOS timer resolution is 30.5 us.
 */
u32_t z_timer_cycle_get_32(void)
{
	u32_t ret, temp;
	u32_t ccr;

	k_spinlock_key_t key = k_spin_lock(&lock);
	ccr = tcount();
	temp = mod_sub(cached_icr, ccr);
	ret = mod_add(total_cycles, temp);
	k_spin_unlock(&lock, key);

	return ret;
}

int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	mchp_pcr_periph_slp_ctrl(PCR_RTMR, MCHP_PCR_SLEEP_DIS);

	RTMR_REGS->CTRL = 0U;
	GIRQ23_REGS->SRC = MCHP_RTMR_GIRQ_VAL;
	NVIC_ClearPendingIRQ(RTMR_IRQn);

	/* load timer and start */
	cached_icr = MAX_TICKS;
	timer_restart(MAX_TICKS);

	IRQ_CONNECT(RTMR_IRQn, 0, xec_rtos_timer_isr, 0, 0);
	GIRQ23_REGS->EN_SET = MCHP_RTMR_GIRQ_VAL;
	irq_enable(RTMR_IRQn);
	return 0;
}
