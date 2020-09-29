/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2019 Microchip Technology Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_rtos_timer

#include <soc.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "XEC RTOS timer doesn't support SMP");
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 32768,
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

#define TIMER_REGS	\
	((RTMR_Type *) DT_INST_REG_ADDR(0))

/* Mask off bits[31:28] of 32-bit count */
#define TIMER_MAX	0x0FFFFFFFUL

#define TIMER_COUNT_MASK	0x0FFFFFFFUL

#define TIMER_STOPPED	0xF0000000UL

/* Adjust cycle count programmed into timer for HW restart latency */
#define TIMER_ADJUST_LIMIT	2
#define TIMER_ADJUST_CYCLES	1

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
static uint32_t total_cycles;
static uint32_t cached_icr = CYCLES_PER_TICK;

static void timer_restart(uint32_t countdown)
{
	TIMER_REGS->CTRL = 0U;
	TIMER_REGS->CTRL = MCHP_RTMR_CTRL_BLK_EN;
	TIMER_REGS->PRLD = countdown;
	TIMER_REGS->CTRL = TIMER_START_VAL;
}

/*
 * Read the RTOS timer counter handling the case where the timer
 * has been reloaded within 1 32KHz clock of reading its count register.
 * The RTOS timer hardware must synchronize the write to its control register
 * on the AHB clock domain with the 32KHz clock domain of its internal logic.
 * This synchronization can take from nearly 0 time up to 1 32KHz clock as it
 * depends upon which 48MHz AHB clock with a 32KHz period the register write
 * was on. We detect the timer is in the load state by checking the read-only
 * count register and the START bit in the control register. If count register
 * is 0 and the START bit is set then the timer has been started and is in the
 * process of moving the preload register value into the count register.
 */
static inline uint32_t timer_count(void)
{
	uint32_t ccr = TIMER_REGS->CNT;

	if ((ccr == 0) && (TIMER_REGS->CTRL & MCHP_RTMR_CTRL_START)) {
		ccr = cached_icr;
	}

	return ccr;
}

#ifdef CONFIG_TICKLESS_KERNEL

static uint32_t last_announcement;	/* last time we called z_clock_announce() */

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
void z_clock_set_timeout(int32_t n, bool idle)
{
	ARG_UNUSED(idle);

	uint32_t ccr, temp;
	int   full_ticks;	/* number of complete ticks we'll wait */
	uint32_t full_cycles;	/* full_ticks represented as cycles */
	uint32_t partial_cycles;	/* number of cycles to first tick boundary */

	if (idle && (n == K_TICKS_FOREVER)) {
		/*
		 * We are not in a locked section. Are writes to two
		 * global objects safe from pre-emption?
		 */
		TIMER_REGS->CTRL = 0U; /* stop timer */
		cached_icr = TIMER_STOPPED;
		return;
	}

	if (n < 1) {
		full_ticks = 0;
	} else if ((n == K_TICKS_FOREVER) || (n > MAX_TICKS)) {
		full_ticks = MAX_TICKS - 1;
	} else {
		full_ticks = n - 1;
	}

	full_cycles = full_ticks * CYCLES_PER_TICK;

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = timer_count();

	/* turn off to clear any pending interrupt status */
	TIMER_REGS->CTRL = 0U;
	MCHP_GIRQ_SRC(DT_INST_PROP(0, girq)) =
		BIT(DT_INST_PROP(0, girq_bit));
	NVIC_ClearPendingIRQ(RTMR_IRQn);

	temp = total_cycles;
	temp += (cached_icr - ccr);
	temp &= TIMER_COUNT_MASK;
	total_cycles = temp;

	partial_cycles = CYCLES_PER_TICK - (total_cycles % CYCLES_PER_TICK);
	cached_icr = full_cycles + partial_cycles;
	/* adjust for up to one 32KHz cycle startup time */
	temp = cached_icr;
	if (temp > TIMER_ADJUST_LIMIT) {
		temp -= TIMER_ADJUST_CYCLES;
	}

	timer_restart(temp);

	k_spin_unlock(&lock, key);
}

/*
 * Return the number of Zephyr ticks elapsed from last call to
 * z_clock_announce in the ISR. The caller casts uint32_t to int32_t.
 * We must make sure bit[31] is 0 in the return value.
 */
uint32_t z_clock_elapsed(void)
{
	uint32_t ccr;
	uint32_t ticks;
	int32_t elapsed;

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = timer_count();

	/* It may not look efficient but the compiler does a good job */
	elapsed = (int32_t)total_cycles - (int32_t)last_announcement;
	if (elapsed < 0) {
		elapsed = -1 * elapsed;
	}
	ticks = (uint32_t)elapsed;
	ticks += cached_icr - ccr;
	ticks /= CYCLES_PER_TICK;
	ticks &= TIMER_COUNT_MASK;

	k_spin_unlock(&lock, key);

	return ticks;
}

static void xec_rtos_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t cycles;
	int32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);

	MCHP_GIRQ_SRC(DT_INST_PROP(0, girq)) =
		BIT(DT_INST_PROP(0, girq_bit));

	/* Restart the timer as early as possible to minimize drift... */
	timer_restart(MAX_TICKS * CYCLES_PER_TICK);

	cycles = cached_icr;
	cached_icr = MAX_TICKS * CYCLES_PER_TICK;

	total_cycles += cycles;
	total_cycles &= TIMER_COUNT_MASK;

	/* handle wrap by using (power of 2) - 1 mask */
	ticks = total_cycles - last_announcement;
	ticks &= TIMER_COUNT_MASK;
	ticks /= CYCLES_PER_TICK;

	last_announcement = total_cycles;

	k_spin_unlock(&lock, key);
	z_clock_announce(ticks);
}

#else

/* Non-tickless kernel build. */

static void xec_rtos_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	MCHP_GIRQ_SRC(DT_INST_PROP(0, girq)) =
		BIT(DT_INST_PROP(0, girq_bit));

	/* Restart the timer as early as possible to minimize drift... */
	timer_restart(cached_icr);

	uint32_t temp = total_cycles + CYCLES_PER_TICK;

	total_cycles = temp & TIMER_COUNT_MASK;
	k_spin_unlock(&lock, key);

	z_clock_announce(1);
}

uint32_t z_clock_elapsed(void)
{
	return 0U;
}

#endif /* CONFIG_TICKLESS_KERNEL */

/*
 * Warning RTOS timer resolution is 30.5 us.
 * This is called by two code paths:
 * 1. Kernel call to k_cycle_get_32() -> arch_k_cycle_get_32() -> here.
 *    The kernel is casting return to (int) and using it uncasted in math
 *    expressions with int types. Expression result is stored in an int.
 * 2. If CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT is not defined then
 *    z_impl_k_busy_wait calls here. This code path uses the value as uint32_t.
 *
 */
uint32_t z_timer_cycle_get_32(void)
{
	uint32_t ret;
	uint32_t ccr;

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = timer_count();
	ret = (total_cycles + (cached_icr - ccr)) & TIMER_COUNT_MASK;

	k_spin_unlock(&lock, key);

	return ret;
}

void z_clock_idle_exit(void)
{
	if (cached_icr == TIMER_STOPPED) {
		cached_icr = CYCLES_PER_TICK;
		timer_restart(cached_icr);
	}
}

void sys_clock_disable(void)
{
	TIMER_REGS->CTRL = 0U;
}

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	mchp_pcr_periph_slp_ctrl(PCR_RTMR, MCHP_PCR_SLEEP_DIS);

#ifdef CONFIG_TICKLESS_KERNEL
	cached_icr = MAX_TICKS;
#endif

	TIMER_REGS->CTRL = 0U;
	MCHP_GIRQ_SRC(DT_INST_PROP(0, girq)) =
		BIT(DT_INST_PROP(0, girq_bit));
	NVIC_ClearPendingIRQ(RTMR_IRQn);

	IRQ_CONNECT(RTMR_IRQn,
		    DT_INST_IRQ(0, priority),
		    xec_rtos_timer_isr, 0, 0);

	MCHP_GIRQ_ENSET(DT_INST_PROP(0, girq)) =
		BIT(DT_INST_PROP(0, girq_bit));
	irq_enable(RTMR_IRQn);

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
	uint32_t btmr_ctrl = B32TMR0_REGS->CTRL = (MCHP_BTMR_CTRL_ENABLE
			  | MCHP_BTMR_CTRL_AUTO_RESTART
			  | MCHP_BTMR_CTRL_COUNT_UP
			  | (47UL << MCHP_BTMR_CTRL_PRESCALE_POS));
	B32TMR0_REGS->CTRL = MCHP_BTMR_CTRL_SOFT_RESET;
	B32TMR0_REGS->CTRL = btmr_ctrl;
	B32TMR0_REGS->PRLD = 0xFFFFFFFFUL;
	btmr_ctrl |= MCHP_BTMR_CTRL_START;

	timer_restart(cached_icr);
	/* wait for Hibernation timer to load count register from preload */
	while (TIMER_REGS->CNT == 0)
		;
	B32TMR0_REGS->CTRL = btmr_ctrl;
#else
	timer_restart(cached_icr);
#endif

	return 0;
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT

/*
 * We implement custom busy wait using a MEC1501 basic timer running on
 * the 48MHz clock domain. This code is here for future power management
 * save/restore of the timer context.
 */

/*
 * 32-bit basic timer 0 configured for 1MHz count up, auto-reload,
 * and no interrupt generation.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0) {
		return;
	}

	uint32_t start = B32TMR0_REGS->CNT;

	for (;;) {
		uint32_t curr = B32TMR0_REGS->CNT;

		if ((curr - start) >= usec_to_wait) {
			break;
		}
	}
}
#endif
