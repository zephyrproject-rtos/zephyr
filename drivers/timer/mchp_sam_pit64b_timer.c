/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_sam_pit64b

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>

LOG_MODULE_REGISTER(pit64b, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* Device constant configuration parameters */
struct sam_pit64b_cfg {
	pit64b_registers_t *reg;
};

#define CYCLES_PER_TICK (sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define MAX_TICKS ((k_ticks_t)(0x1FFFFF / CYCLES_PER_TICK) - 1)
#define MIN_DELAY MAX(1024U, ((uint32_t)CYCLES_PER_TICK/16U))
#define MAX_CYCLES (MAX_TICKS * CYCLES_PER_TICK)

typedef uint32_t cycle_t;

static struct k_spinlock lock;

static cycle_t announced_cycles;
static cycle_t cycle_count;
static uint32_t last_load;
static uint32_t overflow;

const int32_t z_sys_timer_irq_for_test = DT_INST_IRQN(0);

const static struct sam_pit64b_cfg pit64b_cfg = {
		.reg = (pit64b_registers_t *)DT_INST_REG_ADDR(0),
	};

static uint32_t cycles_elapsed(void)
{
	uint32_t val1 = pit64b_cfg.reg->PIT64B_TLSBR;
	uint32_t ctrl = pit64b_cfg.reg->PIT64B_ISR;
	uint32_t val2 = pit64b_cfg.reg->PIT64B_TLSBR;

	if ((ctrl & PIT64B_IER_PERIOD_Msk) || (val1 > val2)) {
		overflow += last_load;
		(void)PIT64B0_REGS->PIT64B_TLSBR;
	}

	return val2 + overflow;
}

static void pit64b_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t dcycles;
	uint32_t delta_ticks;

	(void)cycles_elapsed();

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		cycle_count += overflow;
		overflow = 0;

		dcycles = cycle_count - announced_cycles;
		delta_ticks = dcycles / CYCLES_PER_TICK;
		announced_cycles += delta_ticks * CYCLES_PER_TICK;

		sys_clock_announce(delta_ticks);
	} else {
		sys_clock_announce(1);
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	uint32_t tmp = last_load;
	k_spinlock_key_t key;
	int32_t unannounced;
	uint32_t val1, val2;
	uint32_t delay;

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;

	key = k_spin_lock(&lock);
	cycle_count += cycles_elapsed();
	overflow = 0;

	val1 = pit64b_cfg.reg->PIT64B_TLSBR;

	unannounced = cycle_count - announced_cycles;
	if (unannounced < 0) {
		last_load = MIN_DELAY;
	} else {
		delay = ticks * CYCLES_PER_TICK;
		delay += unannounced;
		delay = DIV_ROUND_UP(delay, CYCLES_PER_TICK) * CYCLES_PER_TICK;
		delay -= unannounced;
		delay = MAX(delay, MIN_DELAY);
		if (delay > MAX_CYCLES) {
			last_load = MAX_CYCLES;
		} else {
			last_load = delay;
		}
	}

	val2 = pit64b_cfg.reg->PIT64B_TLSBR;

	pit64b_cfg.reg->PIT64B_LSBPR = last_load;

	if (val1 > val2) {
		cycle_count += (val2 + (tmp - val1));
	} else {
		cycle_count += (val2 - val1);
	}

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t cycles;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	cycles = cycles_elapsed();

	k_spin_unlock(&lock, key);

	return (cycles + cycle_count - announced_cycles) / CYCLES_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t cycles = cycles_elapsed();

	k_spin_unlock(&lock, key);

	return cycle_count + cycles;
}

uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t cycles = cycles_elapsed();

	k_spin_unlock(&lock, key);

	return cycle_count + cycles;
}

static int sys_clock_driver_init(void)
{
	pit64b_cfg.reg->PIT64B_CR |= PIT64B_CR_SWRST_Msk;

	pit64b_cfg.reg->PIT64B_IER |= PIT64B_IER_PERIOD_Msk;
	pit64b_cfg.reg->PIT64B_MR = PIT64B_MR_CONT_Msk | PIT64B_MR_SMOD_Msk |
				    PIT64B_MR_SGCLK_Msk | PIT64B_MR_PRESCALER(1 - 1);

	last_load = CYCLES_PER_TICK;
	announced_cycles = 0;
	cycle_count = 0;
	overflow = 0;
	pit64b_cfg.reg->PIT64B_MSBPR = 0;
	pit64b_cfg.reg->PIT64B_LSBPR = last_load;
	pit64b_cfg.reg->PIT64B_CR |= PIT64B_CR_START_Msk;

	IRQ_CONNECT(DT_INST_IRQN(0), 0, pit64b_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(sys_clock_driver_init, POST_KERNEL, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
