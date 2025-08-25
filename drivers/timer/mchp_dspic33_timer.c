/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_DSPIC33AK_INCLUDE_ARCH_H_
#define ZEPHYR_ARCH_DSPIC33AK_INCLUDE_ARCH_H_

#define DT_DRV_COMPAT microchip_dspic33_timer

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <xc.h>

#define TIMER1_CYCLES_PER_TICK                                                                     \
	((float)((sys_clock_hw_cycles_per_sec())) /                                                \
	 (float)((2 * DT_INST_PROP(0, prescalar) * CONFIG_SYS_CLOCK_TICKS_PER_SEC)))

#define MAX_TIMER_CLOCK_CYCLES 0xFFFFFFFFU
static struct k_spinlock lock;
static uint64_t total_cycles;

uint8_t map_prescaler_to_bits(uint32_t val)
{
	uint8_t ret_val;

	switch (val) {
	case 1:
		ret_val = 0b00;
		break;
	case 8:
		ret_val = 0b01;
		break;
	case 64:
		ret_val = 0b10;
		break;
	case 256:
		ret_val = 0b11;
		break;
	default:
		ret_val = 0b00;
		break;
	}
	return ret_val;
}

static void configure_timer1(void)
{
	const uint32_t timer_count = (uint32_t)TIMER1_CYCLES_PER_TICK - 1U;

	/* clear timer control and timer count register */
	T1CONbits.ON = 0;

	/* Select standard peripheral clock */
	T1CONbits.TCS = 0;
	T1CONbits.TCKPS = map_prescaler_to_bits(DT_INST_PROP(0, prescalar));
	TMR1 = 0;
	IEC1bits.T1IE = 0;
	IFS1bits.T1IF = 0;

	/* set the time out count */
	PR1 = (uint32_t)timer_count;

	/* Start the timer. */
	T1CONbits.ON = 1;
}

/**
 * @brief Return the current 32-bit cycle count from a hardware timer
 */
uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t cycles;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	cycles = ((uint32_t)total_cycles + (uint32_t)TMR1);
	k_spin_unlock(&lock, key);

	return cycles * 2U * DT_INST_PROP(0, prescalar);
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t ticks_elapsed;
	k_spinlock_key_t key;

	do {
		if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			ticks_elapsed = 0;
			break;
		}
		key = k_spin_lock(&lock);

		/* Compute the number of ticks from the last sys_clock_announce callback.
		 * Since timer1 counter always starts from 0 when the sys_clock_announce
		 * Call is made, the ticks elapsed is current timer1 count divided by
		 * Number of cycles per tick
		 */
		ticks_elapsed =
			(uint32_t)TMR1 < (uint32_t)TIMER1_CYCLES_PER_TICK
				? 0
				: (uint32_t)(TMR1 + ((uint32_t)TIMER1_CYCLES_PER_TICK / 2U)) /
					  ((uint32_t)TIMER1_CYCLES_PER_TICK);
		k_spin_unlock(&lock, key);
	} while (0);

	return ticks_elapsed;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	volatile uint32_t next_count;
	k_spinlock_key_t key;

	ARG_UNUSED(idle);

	do {
		if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {

			/* if it is not in tickles mode, no need to change the
			 * Timeout interval, it will periodically interrupt
			 * At every tick
			 */
			break;
		}

		/* check if it is K_TICKS_FOREVER, is so set to max cycles */
		next_count = (ticks == K_TICKS_FOREVER)
				     ? MAX_TIMER_CLOCK_CYCLES
				     : (uint32_t)(ticks * TIMER1_CYCLES_PER_TICK);
		key = k_spin_lock(&lock);
		total_cycles = total_cycles + (uint64_t)TMR1;

		/* clear the timer1 counter register and set the period register to the
		 * New timeout value. This should be done with TIMER1 disabled
		 */
		T1CONbits.ON = 0;
		TMR1 = 0;
		PR1 = next_count;
		T1CONbits.ON = 1;
		k_spin_unlock(&lock, key);
	} while (0);
}

/* Timer1 ISR */
static void timer1_isr(const void *arg)
{
	uint32_t elapsed_ticks;
	k_spinlock_key_t key;

	ARG_UNUSED(arg);

	/* compute the number of elapsed ticks. For both tickles and tick
	 * Based kernel, it will be the period count divided by the number
	 * Of cycles per tick. For tickles, the period would have been set
	 * To the next event time.
	 */
	elapsed_ticks = (uint32_t)PR1 / (uint32_t)TIMER1_CYCLES_PER_TICK;
	key = k_spin_lock(&lock);

	total_cycles = total_cycles + (uint64_t)PR1;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* If not in tickles mode set the interrupt to happen at the next
		 * Tick. Clear the timer1 counter register and set the period register
		 * to the tick timeout value. This should be done with TIMER1 disabled.
		 */
		T1CONbits.ON = 0;
		PR1 = (uint32_t)TIMER1_CYCLES_PER_TICK;
		T1CONbits.ON = 1;
	}
	k_spin_unlock(&lock, key);

	/* notify the kernel about the tick */
	sys_clock_announce(elapsed_ticks);
}

/* Initialize the system clock driver */
int sys_clock_driver_init(void)
{
	/* connect the timer1 isr to the interrupt. The interrupt number
	 * Is obtained from the timer device node
	 */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), timer1_isr, NULL, 0);

	/* configure timer1 with cycles per tic as the Period count.
	 * Every interrupt will reload the period register with the
	 * next interrupt tick count
	 */
	configure_timer1();
	irq_enable(DT_INST_IRQN(0));
	return 0;
}

#endif /* ZEPHYR_ARCH_DSPIC33AK_INCLUDE_ARCH_H_ */

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
