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
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <xc.h>

#define TIMER1_CYCLES_PER_TICK                                                                     \
	((sys_clock_hw_cycles_per_sec() +                                                          \
	  ((2U * DT_INST_PROP(0, prescaler) * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / 2U)) /             \
	 (2U * DT_INST_PROP(0, prescaler) * CONFIG_SYS_CLOCK_TICKS_PER_SEC))

#define TIMER1_BASE            DT_REG_ADDR(DT_NODELABEL(timer1))
#define MAX_TIMER_CLOCK_CYCLES 0xFFFFFFFFU
#define TMRx_OFFSET            0x0004U
#define PRx_OFFSET             0x0008U

static struct k_spinlock lock;
static uint64_t total_cycles;
static uint64_t prev_announced_cycle;
static uint32_t un_announced_cycles;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_INST_IRQN(0);
#endif

/* Timer configuration from DeviceTree */
static volatile uint32_t *TxCON = (uint32_t *)TIMER1_BASE;
static volatile uint32_t *TMRx = (uint32_t *)(TIMER1_BASE + TMRx_OFFSET);
static volatile uint32_t *PRx = (uint32_t *)(TIMER1_BASE + PRx_OFFSET);

/* Map prescaler value to bits */
static uint8_t map_prescaler_to_bits(uint32_t val)
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

/* Configure timer registers */
static void configure_timer(uint32_t cycles)
{
	/* Turn off timer and clear TMR register */
	*TxCON &= ~(0x8000U);
	*TMRx = 0;

	/* set the timeout count */
	*PRx = cycles - 1U;

	/* Start the timer. */
	*TxCON |= 0x8000U;
}

static void initialize_timer(void)
{
	/* Standard peripheral clock */
	*TxCON &= ~(0x0002U);
	*TxCON |= (map_prescaler_to_bits(DT_INST_PROP(0, prescaler)) << 4U);

	configure_timer(TIMER1_CYCLES_PER_TICK);
}

/* Busy wait */
#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	ARG_UNUSED(usec_to_wait);
	__asm__ volatile("sl.l w0,#0x03,w0\n\t"
			 "repeat.w w0\n\t"
			 "neop\n\n\t");
}
#endif

/**
 * @brief Return the current 32-bit cycle count from a hardware timer
 */
uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t cycles;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	cycles = (uint32_t)total_cycles + (*PRx * (uint32_t)arch_dspic_irq_isset(DT_INST_IRQN(0))) +
		 *TMRx;
	k_spin_unlock(&lock, key);

	return cycles * 2U * DT_INST_PROP(0, prescaler);
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
		ticks_elapsed = ((uint32_t)(total_cycles - prev_announced_cycle) +
				 (uint32_t)(*TMRx + (TIMER1_CYCLES_PER_TICK / 10U))) /
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

			/* If it is not in tickles mode, no need to change the
			 * Timeout interval, it will periodically interrupt
			 * At every tick
			 */
			break;
		}

		/* check if it is K_TICKS_FOREVER, is so set to max cycles */
		next_count = (ticks == K_TICKS_FOREVER)
				     ? MAX_TIMER_CLOCK_CYCLES
				     : (uint32_t)((uint32_t)(ticks * TIMER1_CYCLES_PER_TICK) + 1U);

		key = k_spin_lock(&lock);
		total_cycles = total_cycles + (uint64_t)*TMRx;
		un_announced_cycles = un_announced_cycles + *TMRx;
		configure_timer(next_count);
		k_spin_unlock(&lock, key);
	} while (0);
}

/* ISR */
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
	elapsed_ticks = (uint32_t)(un_announced_cycles + *PRx) / (uint32_t)TIMER1_CYCLES_PER_TICK;
	key = k_spin_lock(&lock);

	total_cycles = total_cycles + (uint64_t)*PRx;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* If not in tickles mode set the interrupt to happen at the next
		 * Tick. Clear the timer1 counter register and set the period register
		 * to the tick timeout value. This should be done with TIMER1 disabled.
		 */
		*TxCON &= ~(0x8000U);
		*PRx = TIMER1_CYCLES_PER_TICK;
		*TxCON |= 0x8000U;
	}
	k_spin_unlock(&lock, key);

	/* If PR1 is within range, we reset un_announced_cycles and call
	 * sys_clock_announce(elapsed_ticks) to inform the kernel about the elapsed
	 * system ticks. This keeps the Zephyr kernel’s time accounting accurate.
	 */
	if (*PRx != MAX_TIMER_CLOCK_CYCLES) {
		*TxCON &= ~(0x8000U);
		un_announced_cycles = 0;
		prev_announced_cycle = total_cycles;
		sys_clock_announce(elapsed_ticks);
	}
}

/* Driver init: get first enabled timer from DT */
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
	initialize_timer();
	irq_enable(DT_INST_IRQN(0));
	return 0;
}

#endif /* ZEPHYR_ARCH_DSPIC33AK_INCLUDE_ARCH_H_ */

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
