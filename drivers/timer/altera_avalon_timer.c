/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <system_timer.h>

/* STATUS register */
#define ALTERA_AVALON_TIMER_STATUS_REG              0
#define ALTERA_AVALON_TIMER_STATUS_TO_MSK           (0x1)
#define ALTERA_AVALON_TIMER_STATUS_TO_OFST          (0)
#define ALTERA_AVALON_TIMER_STATUS_RUN_MSK          (0x2)
#define ALTERA_AVALON_TIMER_STATUS_RUN_OFST         (1)

/* CONTROL register */
#define ALTERA_AVALON_TIMER_CONTROL_REG             1
#define ALTERA_AVALON_TIMER_CONTROL_ITO_MSK         (0x1)
#define ALTERA_AVALON_TIMER_CONTROL_ITO_OFST        (0)
#define ALTERA_AVALON_TIMER_CONTROL_CONT_MSK        (0x2)
#define ALTERA_AVALON_TIMER_CONTROL_CONT_OFST       (1)
#define ALTERA_AVALON_TIMER_CONTROL_START_MSK       (0x4)
#define ALTERA_AVALON_TIMER_CONTROL_START_OFST      (2)
#define ALTERA_AVALON_TIMER_CONTROL_STOP_MSK        (0x8)
#define ALTERA_AVALON_TIMER_CONTROL_STOP_OFST       (3)

/* Period and SnapShot Register for COUNTER_SIZE = 32 */
/*----------------------------------------------------*/
/* PERIODL register */
#define ALTERA_AVALON_TIMER_PERIODL_REG             2
#define ALTERA_AVALON_TIMER_PERIODL_MSK             (0xFFFF)
#define ALTERA_AVALON_TIMER_PERIODL_OFST            (0)

/* PERIODH register */
#define ALTERA_AVALON_TIMER_PERIODH_REG             3
#define ALTERA_AVALON_TIMER_PERIODH_MSK             (0xFFFF)
#define ALTERA_AVALON_TIMER_PERIODH_OFST            (0)

/* SNAPL register */
#define ALTERA_AVALON_TIMER_SNAPL_REG               4
#define ALTERA_AVALON_TIMER_SNAPL_MSK               (0xFFFF)
#define ALTERA_AVALON_TIMER_SNAPL_OFST              (0)

/* SNAPH register */
#define ALTERA_AVALON_TIMER_SNAPH_REG               5
#define ALTERA_AVALON_TIMER_SNAPH_MSK               (0xFFFF)
#define ALTERA_AVALON_TIMER_SNAPH_OFST              (0)

static uint32_t accumulated_cycle_count;

static uint32_t get_snapshot(void)
{
#if TIMER_0_SNAPSHOT
	uint32_t snap, s1, s2;
	int key;

	key = irq_lock();

	/* Writing any data to one of the snapshot registers populates all
	 * of them with the value of the counter. The data written is ignored
	 */
	_nios2_reg_write((void *)TIMER_0_BASE, ALTERA_AVALON_TIMER_SNAPL_REG,
			 1);

	s1 = _nios2_reg_read((void *)TIMER_0_BASE,
			     ALTERA_AVALON_TIMER_SNAPL_REG) &
		ALTERA_AVALON_TIMER_SNAPL_MSK;
	s2 = _nios2_reg_read((void *)TIMER_0_BASE,
				 ALTERA_AVALON_TIMER_SNAPH_REG) &
		ALTERA_AVALON_TIMER_SNAPH_MSK;

	irq_unlock(key);

	snap = s1 | (s2 << 16);

	return sys_clock_hw_cycles_per_tick - snap;
#else
	return 0;
#endif
}


static void timer_irq_handler(void *unused)
{
	ARG_UNUSED(unused);

	/* Clear the interrupt */
	_nios2_reg_write((void *)TIMER_0_BASE, ALTERA_AVALON_TIMER_STATUS_REG,
			 0);
	accumulated_cycle_count += sys_clock_hw_cycles_per_tick;

	_sys_clock_tick_announce();
}


#ifdef CONFIG_TICKLESS_IDLE
#error "Tickless idle not yet implemented for Avalon timer"
#endif


int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

#if TIMER_0_FIXED_PERIOD
#error "Can't set timer period!"
#else
	_nios2_reg_write((void *)TIMER_0_BASE, ALTERA_AVALON_TIMER_PERIODL_REG,
			 sys_clock_hw_cycles_per_tick & 0xFFFF);
	_nios2_reg_write((void *)TIMER_0_BASE, ALTERA_AVALON_TIMER_PERIODH_REG,
			 (sys_clock_hw_cycles_per_tick >> 16) & 0xFFFF);
#endif

	IRQ_CONNECT(TIMER_0_IRQ, 0, timer_irq_handler, NULL, 0);
	irq_enable(TIMER_0_IRQ);

	/* Initial configuration: Generate interrupts, run continuously,
	 * start running
	 */
	_nios2_reg_write((void *)TIMER_0_BASE, ALTERA_AVALON_TIMER_CONTROL_REG,
			 ALTERA_AVALON_TIMER_CONTROL_ITO_MSK  |
			 ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
			 ALTERA_AVALON_TIMER_CONTROL_START_MSK);
	return 0;
}


/**
 *
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles.
 *
 * @return up counter of elapsed clock cycles
 */
uint32_t sys_cycle_get_32(void)
{
	return accumulated_cycle_count + get_snapshot();
}

