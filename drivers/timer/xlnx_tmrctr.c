/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>

#include <zephyr/sys_clock.h>
#include <zephyr/arch/cpu.h>
#include <soc.h>

#define DT_DRV_COMPAT xlnx_tmrctr

#define IRQ_TIMER            DT_INST_IRQN(CONFIG_XLNX_TMRCTR_TIMER_INDEX)
#define TIMER_CYCLES_PER_SEC DT_INST_PROP(CONFIG_XLNX_TMRCTR_TIMER_INDEX, clock_frequency)
#define BASE_ADDRESS         DT_INST_REG_ADDR(0)

#define TICK_TIMER_COUNTER_NUMBER 0U
#define SYS_CLOCK_COUNTER_NUMBER  1U

#define TIMER_CYCLES_PER_TICK (TIMER_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define TICK_TIMER_TOP_VALUE  (TIMER_CYCLES_PER_TICK - 1UL)

#define NUM_COUNTERS 2

/* Register definitions */
#define XTC_TCSR_OFFSET 0 /**< Control/Status register */
#define XTC_TLR_OFFSET  4 /**< Load register */
#define XTC_TCR_OFFSET  8 /**< Timer counter register */

/* Control status register mask */
#define XTC_CSR_CASC_MASK         0x00000800
#define XTC_CSR_ENABLE_ALL_MASK   0x00000400
#define XTC_CSR_ENABLE_PWM_MASK   0x00000200
#define XTC_CSR_INT_OCCURRED_MASK 0x00000100
#define XTC_CSR_ENABLE_TMR_MASK   0x00000080
#define XTC_CSR_ENABLE_INT_MASK   0x00000040
#define XTC_CSR_LOAD_MASK         0x00000020
#define XTC_CSR_AUTO_RELOAD_MASK  0x00000010
#define XTC_CSR_EXT_CAPTURE_MASK  0x00000008
#define XTC_CSR_EXT_GENERATE_MASK 0x00000004
#define XTC_CSR_DOWN_COUNT_MASK   0x00000002
#define XTC_CSR_CAPTURE_MODE_MASK 0x00000001

/* 1st counter is at offset 0, 2nd counter is at offset 16 */
#define NUM_REGS_PER_COUNTER    16
#define COUNTER_REG_OFFSET(idx) (NUM_REGS_PER_COUNTER * idx)

/*
 * CYCLES_NEXT_MIN must be large enough to ensure that the timer does not miss
 * interrupts.  This value was conservatively set, and there is room for improvement.
 */
#define CYCLES_NEXT_MIN (TIMER_CYCLES_PER_SEC / 5000)
/* We allow only half the maximum numerical range of the cycle counters so that we
 * can never miss a sysclock overflow. This is also being very conservative.
 */
#define CYCLES_NEXT_MAX (0xFFFFFFFFU / 2)

static volatile uint32_t last_cycles;

BUILD_ASSERT(TIMER_CYCLES_PER_SEC >= CONFIG_SYS_CLOCK_TICKS_PER_SEC,
			 "Timer clock frequency must be greater than the system tick "
			 "frequency");

BUILD_ASSERT((TIMER_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) == 0,
			 "Timer clock frequency is not divisible by the system tick "
			 "frequency");

BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % TIMER_CYCLES_PER_SEC) == 0,
			 "CPU clock frequency is not divisible by the Timer clock frequency "
			 "frequency");

enum xlnx_tmrctr_state {
	XLNX_TMRCTR_INIT,   /* Initial (inactive) state */
	XLNX_TMRCTR_READY,  /* Initialised */
	XLNX_TMRCTR_RUNNING /* Started */
};

struct xlnx_tmrctr_data {
	mm_reg_t base;
	enum xlnx_tmrctr_state state;
};

struct xlnx_tmrctr_data xlnx_tmrctr = {
	.base = BASE_ADDRESS,
	.state = XLNX_TMRCTR_INIT,
};

#define xlnx_tmrctr_read32(counter_number, offset)                                                 \
	sys_read32(BASE_ADDRESS + COUNTER_REG_OFFSET(counter_number) + offset)

#define xlnx_tmrctr_write32(counter_number, value, offset)                                         \
	sys_write32(value, BASE_ADDRESS + COUNTER_REG_OFFSET(counter_number) + offset)

volatile uint32_t xlnx_tmrctr_read_count(void)
{
	return xlnx_tmrctr_read32(SYS_CLOCK_COUNTER_NUMBER, XTC_TCR_OFFSET);
}

volatile uint32_t xlnx_tmrctr_read_hw_cycle_count(void)
{
	return (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / TIMER_CYCLES_PER_SEC) *
				 xlnx_tmrctr_read_count();
}

static void xlnx_tmrctr_clear_interrupt(void)
{
	uint32_t control_status_register =
		xlnx_tmrctr_read32(TICK_TIMER_COUNTER_NUMBER, XTC_TCSR_OFFSET);

	xlnx_tmrctr_write32(TICK_TIMER_COUNTER_NUMBER,
		control_status_register | XTC_CSR_INT_OCCURRED_MASK, XTC_TCSR_OFFSET);
}

static inline void xlnx_tmrctr_set_reset_value(uint8_t counter_number, uint32_t reset_value)
{
	xlnx_tmrctr_write32(counter_number, reset_value, XTC_TLR_OFFSET);
}

static inline void xlnx_tmrctr_set_options(uint8_t counter_number, uint32_t options)
{
	xlnx_tmrctr_write32(counter_number, options, XTC_TCSR_OFFSET);
}

#ifdef CONFIG_TICKLESS_KERNEL
static void xlnx_tmrctr_reload_tick_timer(uint32_t delta_cycles)
{
	uint32_t csr_val;
	uint32_t cur_cycle_count = xlnx_tmrctr_read_count();

	/* Ensure that the delta_cycles value meets the timing requirements */
	if (delta_cycles < CYCLES_NEXT_MIN) {
		/* Don't risk missing an interrupt */
		delta_cycles = CYCLES_NEXT_MIN;
	}
	if (delta_cycles > CYCLES_NEXT_MAX - cur_cycle_count) {
		/* Don't risk missing a sysclock overflow */
		delta_cycles = CYCLES_NEXT_MAX - cur_cycle_count;
	}

	/* Write counter load value for interrupt generation */
	xlnx_tmrctr_set_reset_value(TICK_TIMER_COUNTER_NUMBER, delta_cycles);

	/* Load the load value */
	csr_val = xlnx_tmrctr_read32(TICK_TIMER_COUNTER_NUMBER, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(TICK_TIMER_COUNTER_NUMBER, csr_val | XTC_CSR_LOAD_MASK,
			    XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(TICK_TIMER_COUNTER_NUMBER, csr_val, XTC_TCSR_OFFSET);
}
#endif /* CONFIG_TICKLESS_KERNEL */

static void xlnx_tmrctr_irq_handler(const void *unused)
{
	uint32_t cycles;
	uint32_t delta_ticks;

	ARG_UNUSED(unused);

	cycles = xlnx_tmrctr_read_count();
	/* Calculate the number of ticks since last announcement  */
	delta_ticks = (cycles - last_cycles) / TIMER_CYCLES_PER_TICK;
	/* Update last cycles count without the rounding error */
	last_cycles += (delta_ticks * TIMER_CYCLES_PER_TICK);

	/* Announce to the kernel*/
	sys_clock_announce(delta_ticks);

	xlnx_tmrctr_clear_interrupt();
	xlnx_intc_irq_acknowledge(BIT(IRQ_TIMER));
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles;
	uint32_t delta_cycles;

	/* Read counter value */
	cycles = xlnx_tmrctr_read_count();

	/* Calculate timeout counter value */
	if (ticks == K_TICKS_FOREVER) {
		delta_cycles = CYCLES_NEXT_MAX;
	} else {
		delta_cycles = ((uint32_t)ticks * TIMER_CYCLES_PER_TICK);
	}

	/* Set timer reload value for the next interrupt */
	xlnx_tmrctr_reload_tick_timer(delta_cycles);
#endif
}

uint32_t sys_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	uint32_t cycles = xlnx_tmrctr_read_count();

	return (cycles - last_cycles) / TIMER_CYCLES_PER_TICK;
#else
	/* Always return 0 for tickful operation */
	return 0;
#endif
}

uint32_t sys_clock_cycle_get_32(void)
{
	return xlnx_tmrctr_read_hw_cycle_count();
}

static int xlnx_tmrctr_initialize(void)
{
	if (xlnx_tmrctr.state != XLNX_TMRCTR_INIT) {
		return -EEXIST;
	}

	xlnx_tmrctr.state = XLNX_TMRCTR_READY;

	for (uint8_t counter_number = 0; counter_number < NUM_COUNTERS; counter_number++) {
		/* Set the compare register to 0. */
		xlnx_tmrctr_write32(counter_number, 0, XTC_TLR_OFFSET);
		/* Reset the timer and the interrupt. */
		xlnx_tmrctr_write32(counter_number, XTC_CSR_INT_OCCURRED_MASK | XTC_CSR_LOAD_MASK,
						XTC_TCSR_OFFSET);
		/* Release the reset. */
		xlnx_tmrctr_write32(counter_number, 0, XTC_TCSR_OFFSET);
	}

	return 0;
}

static int xlnx_tmrctr_start(void)
{
	if (xlnx_tmrctr.state == XLNX_TMRCTR_INIT) {
		return -ENODEV;
	}
	if (xlnx_tmrctr.state == XLNX_TMRCTR_RUNNING) {
		return -EALREADY;
	}

	int control_status_register = xlnx_tmrctr_read32(
		TICK_TIMER_COUNTER_NUMBER, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(TICK_TIMER_COUNTER_NUMBER, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(TICK_TIMER_COUNTER_NUMBER,
		control_status_register | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);

	control_status_register = xlnx_tmrctr_read32(SYS_CLOCK_COUNTER_NUMBER, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(SYS_CLOCK_COUNTER_NUMBER, XTC_CSR_LOAD_MASK, XTC_TCSR_OFFSET);
	xlnx_tmrctr_write32(SYS_CLOCK_COUNTER_NUMBER,
		control_status_register | XTC_CSR_ENABLE_TMR_MASK, XTC_TCSR_OFFSET);

	xlnx_tmrctr.state = XLNX_TMRCTR_RUNNING;

	return 0;
}

static int sys_clock_driver_init(void)
{
	int status = xlnx_tmrctr_initialize();

	if (status != 0) {
		return status;
	}

#ifdef CONFIG_TICKLESS_KERNEL
	xlnx_tmrctr_set_reset_value(TICK_TIMER_COUNTER_NUMBER, CYCLES_NEXT_MAX);
	xlnx_tmrctr_set_options(TICK_TIMER_COUNTER_NUMBER, XTC_CSR_ENABLE_INT_MASK |
									 XTC_CSR_DOWN_COUNT_MASK);
#else
	xlnx_tmrctr_set_reset_value(TICK_TIMER_COUNTER_NUMBER, TIMER_CYCLES_PER_TICK);
	xlnx_tmrctr_set_options(TICK_TIMER_COUNTER_NUMBER, XTC_CSR_ENABLE_INT_MASK |
									 XTC_CSR_AUTO_RELOAD_MASK |
									 XTC_CSR_DOWN_COUNT_MASK);
#endif

	xlnx_tmrctr_set_options(SYS_CLOCK_COUNTER_NUMBER, XTC_CSR_AUTO_RELOAD_MASK);

	status = xlnx_tmrctr_start();

	if (status != 0) {
		return status;
	}

	last_cycles = xlnx_tmrctr_read_count();

	IRQ_CONNECT(IRQ_TIMER, 0, xlnx_tmrctr_irq_handler, NULL, 0);
	irq_enable(IRQ_TIMER);

	return 0;
}

#if defined(CONFIG_MICROBLAZE)
/**
 * @brief Overwrite cycle based busy wait
 * Implementation is derived from z_impl_k_busy_wait@kernel/timeout.c
 *
 * @param usec_to_wait
 * @note Microblaze arch already implements an unaccurate, nop based
 * no-timer-required busy wait. This routine simply overrides it with
 * a much more accurate version.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	uint32_t start_cycles = xlnx_tmrctr_read_count();

	/* use 64-bit math to prevent overflow when multiplying */
	uint32_t cycles_to_wait =
		(uint32_t)((uint64_t)usec_to_wait * (uint64_t)TIMER_CYCLES_PER_SEC /
				 (uint64_t)USEC_PER_SEC);

	for (;;) {
		uint32_t current_cycles = xlnx_tmrctr_read_count();

		/* this handles the rollover on an unsigned 32-bit value */
		if ((current_cycles - start_cycles) >= cycles_to_wait) {
			break;
		}
	}
}
#endif

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
