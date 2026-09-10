/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2019 Microchip Technology Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_rtos_timer

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/spinlock.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "XEC RTOS timer doesn't support SMP");
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 32768,
	     "XEC RTOS timer HW frequency is fixed at 32768");

/* Microchip MEC 32-bit RTOS timer runs on the 32 KHz always on clock.
 * It is a downcounter with auto-reload capability.
 */
#define TIMER_CNT_OFS       0  /* R/W counter */
#define TIMER_PRLD_OFS      4u /* R/W preload value */
#define TIMER_CR_OFS        8u /* R/W control */
#define TIMER_CR_ACTV_POS   0  /* activate block */
#define TIMER_CR_ARL_EN_POS 1  /* auto-reload enable */
#define TIMER_CR_START_POS  2  /* start timer counting */
#define TIMER_CR_HDBA_POS   3  /* Halt counting if debugger not in reset */
#define TIMER_CR_HALT_POS   4  /* Halt if written to 1, unhalt by clearing */

/* MEC GIRQ */
#define GIRQ_SIZE       20u /* Each GIRQx is 5 32-bit registers */
#define GIRQ_SRC_OFS    0   /* R/W1C latched status bits */
#define GIRQ_ENSET_OFS  4u  /* read, write 1 to set enable bit(s) */
#define GIRQ_RESULT_OFS 8u  /* R/O bitwise AND of SRC and ENSET */
#define GIRQ_ENCLR_OFS  12u /* read, write 1 to clear enable bit(s) */

#define DEBUG_RTOS_TIMER 0

#if DEBUG_RTOS_TIMER != 0
/* Enable feature to halt timer on JTAG/SWD CPU halt */
#define TIMER_START_VAL (BIT(TIMER_CR_ACTV_POS) | BIT(TIMER_CR_START_POS) | BIT(TIMER_CR_HALT_POS))
#else
#define TIMER_START_VAL (BIT(TIMER_CR_ACTV_POS) | BIT(TIMER_CR_START_POS))
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
 * To reduce truncation errors from accumulating due to conversion
 * to/from time, ticks, and HW cycles set ticks per second equal to
 * the frequency. With tickless kernel mode enabled the kernel will not
 * program a periodic timer at this fast rate.
 * CONFIG_SYS_CLOCK_TICKS_PER_SEC=32768
 */

#define CYCLES_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define TIMER_BASE        (mm_reg_t) DT_INST_REG_ADDR(0)
#define TIMER_GIRQ_NUM    DT_INST_PROP_BY_IDX(0, girqs, 0)
#define TIMER_GIRQ_BITPOS DT_INST_PROP_BY_IDX(0, girqs, 1)
/* data sheet GIRQ numbers start at 8 */
#define TIMER_GIRQ_BASE                                                                            \
	(mm_reg_t)(DT_REG_ADDR(DT_NODELABEL(ecia)) + (GIRQ_SIZE * (TIMER_GIRQ_NUM - 8u)))

#define TIMER_NVIC_NO   DT_INST_IRQN(0)
#define TIMER_NVIC_PRIO DT_INST_IRQ(0, priority)

/* Mask off bits[31:28] of 32-bit count */
#define TIMER_MAX        0x0fffffffu
#define TIMER_COUNT_MASK 0x0fffffffu
#define TIMER_STOPPED    0xf0000000u

/* Adjust cycle count programmed into timer for HW restart latency */
#define TIMER_ADJUST_LIMIT  2
#define TIMER_ADJUST_CYCLES 1

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
BUILD_ASSERT(DT_INST_NODE_HAS_PROP(0, busy_wait_timer),
	     "Driver does not not have busy-wait-timer property!");

#define BTMR_NODE DT_INST_PHANDLE(0, busy_wait_timer)

BUILD_ASSERT(DT_PROP(BTMR_NODE, max_value) == UINT32_MAX, "Custom busy wait timer is not 32-bit!");

#define BTMR_BASE (mm_reg_t) DT_REG_ADDR(BTMR_NODE)

#define BTMR_CNT_OFS         0
#define BTMR_PRLD_OFS        4u
#define BTMR_SR_OFS          8u
#define BTMR_IER_OFS         0xcu
#define BTMR_CR_OFS          0x10u
#define BTMR_CR_ACTV_POS     0
#define BTMR_CR_CNT_UP_POS   2
#define BTMR_CR_ARS_POS      3
#define BTMR_CR_SOFT_RST_POS 4
#define BTMR_CR_START_POS    5
#define BTMR_CR_RLD_POS      6
#define BTMR_CR_HALT_POS     7
#define BTMR_CR_PS_POS       16
#define BTMR_CR_PS_MSK       GENMASK(31, 16)
#define BTMR_CR_PS_SET(n)    FIELD_PREP(BTMR_CR_PS_MSK, (n))
#define BMTR_CR_PS_GET(n)    FIELD_GET(BTMR_CR_PS_MSK, (n))

#endif

/*
 * The system clock lock protects all access to the RTMR registers, as well as
 * 'total_cycles' and 'cached_icr'.
 */

/* Whole reloads consumed so far, the low bits of the synthesized count */
static uint32_t total_cycles;
static uint32_t cached_icr = CYCLES_PER_TICK;

static inline void timer_restart(uint32_t countdown)
{
	sys_write32(0, TIMER_BASE + TIMER_CR_OFS);
	sys_write32(BIT(TIMER_CR_ACTV_POS), TIMER_BASE + TIMER_CR_OFS);
	sys_write32(countdown, TIMER_BASE + TIMER_PRLD_OFS);
	sys_write32(TIMER_START_VAL, TIMER_BASE + TIMER_CR_OFS);
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
	uint32_t ccr = sys_read32(TIMER_BASE + TIMER_CNT_OFS);

	if ((ccr == 0) && sys_test_bit(TIMER_BASE + TIMER_CR_OFS, TIMER_CR_START_POS)) {
		ccr = cached_icr;
	}

	return ccr;
}

/*
 * A down-counter reloaded from the preload register, which interrupts at zero
 * and does not free-run: a RELOAD backend. The counter the core reads is
 * synthesized, the reloads consumed so far plus what the one in flight has
 * counted down, and it is 28 bits wide like the hardware. That read touches
 * state the ISR and the reload path also write, hence
 * TIMER_CORE_COUNTER_NONATOMIC.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 28
#define TIMER_CORE_COUNTER_NONATOMIC

/* One reload cannot express more than the preload register holds. */
#define TIMER_CORE_ALARM_MAX_CYCLES TIMER_MAX

static inline uint32_t timer_driver_cycle_get(void)
{
	return (total_cycles + (cached_icr - timer_count())) & TIMER_COUNT_MASK;
}

static void timer_driver_set_reload(uint32_t cycles)
{
	uint32_t ccr = timer_count();

	/* Fold what the reload in flight has already counted down into the
	 * synthesized count before replacing it.
	 */
	total_cycles = (total_cycles + (cached_icr - ccr)) & TIMER_COUNT_MASK;
	cached_icr = cycles;

	/* Adjust cycle count programmed into timer for HW restart latency */
	if (cycles > TIMER_ADJUST_LIMIT) {
		cycles -= TIMER_ADJUST_CYCLES;
	}

	timer_restart(cycles);
}

#include "system_timer_generic.h"

static void xec_rtos_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	sys_write32(BIT(TIMER_GIRQ_BITPOS), TIMER_GIRQ_BASE + GIRQ_SRC_OFS);

	/* Restart as early as possible to minimise drift, and account the
	 * reload just consumed. A tickless kernel gets the next deadline from
	 * the core right after the announce, so this reload only has to keep the
	 * counter running; a ticked one leaves the period to the driver.
	 */
	total_cycles = (total_cycles + cached_icr) & TIMER_COUNT_MASK;
	cached_icr = IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? TIMER_MAX : CYCLES_PER_TICK;
	timer_restart(cached_icr);

	timer_core_announce_from(key);
}

void sys_clock_idle_enter(uint32_t ticks)
{
	if (ticks != SYS_CLOCK_IDLE_FOREVER) {
		sys_clock_set_timeout(ticks, false);
		return;
	}

	/* Nothing to wake up for and the uptime accounting may drift: stop the
	 * timer. Only this path may, and only because the CPU is on its way to
	 * sleep, so nothing is left to read a synthesized count that stops with
	 * it. sys_clock_idle_exit() starts it again.
	 */
	sys_write32(0, TIMER_BASE + TIMER_CR_OFS);
	cached_icr = TIMER_STOPPED;
}

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
void sys_clock_idle_exit(void)
{
	if (cached_icr == TIMER_STOPPED) {
		cached_icr = CYCLES_PER_TICK;
		timer_restart(cached_icr);
	}
}

void sys_clock_disable(void)
{
	sys_write32(0, TIMER_BASE + TIMER_CR_OFS);
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

	uint32_t start = sys_read32(BTMR_BASE + BTMR_CNT_OFS);

	for (;;) {
		uint32_t curr = sys_read32(BTMR_BASE + BTMR_CNT_OFS);

		if ((curr - start) >= usec_to_wait) {
			break;
		}
	}
}
#endif

static int sys_clock_driver_init(void)
{
	sys_write32(0, TIMER_BASE + TIMER_CR_OFS);
	sys_write32(BIT(TIMER_GIRQ_BITPOS), TIMER_GIRQ_BASE + GIRQ_ENCLR_OFS);
	sys_write32(BIT(TIMER_GIRQ_BITPOS), TIMER_GIRQ_BASE + GIRQ_SRC_OFS);
	NVIC_ClearPendingIRQ(TIMER_NVIC_NO);

	IRQ_CONNECT(TIMER_NVIC_NO, TIMER_NVIC_PRIO, xec_rtos_timer_isr, 0, 0);
	irq_enable(TIMER_NVIC_NO);
	sys_write32(BIT(TIMER_GIRQ_BITPOS), TIMER_GIRQ_BASE + GIRQ_ENSET_OFS);

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
	uint32_t btmr_ctrl = (BIT(BTMR_CR_ACTV_POS) | BIT(BTMR_CR_ARS_POS) |
			      BIT(BTMR_CR_CNT_UP_POS) | BTMR_CR_PS_SET(47u));

	sys_write32(BIT(BTMR_CR_SOFT_RST_POS), BTMR_BASE + BTMR_CR_OFS);
	sys_write32(btmr_ctrl, BTMR_BASE + BTMR_CR_OFS);
	sys_write32(UINT32_MAX, BTMR_BASE + BTMR_PRLD_OFS);
	sys_set_bit(BTMR_BASE + BTMR_CR_OFS, BTMR_CR_START_POS);

	timer_restart(cached_icr);
	/* wait for RTOS timer to load count register from preload */
	while (sys_read32(BTMR_BASE + BTMR_CNT_OFS) == 0) {
		;
	}
#else
	timer_restart(cached_icr);
#endif

	/* Seed the announce baseline and arm the first tick. */
	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
