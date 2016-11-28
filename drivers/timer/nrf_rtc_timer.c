/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
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

#include <soc.h>
#include <clock_control.h>
#include <system_timer.h>
#include <drivers/clock_control/nrf5_clock_control.h>

#define RTC_TICKS ((uint32_t)(((((uint64_t)1000000UL / \
				 CONFIG_SYS_CLOCK_TICKS_PER_SEC) * \
				1000000000UL) / 30517578125UL)) & 0x00FFFFFF)

extern int64_t _sys_clock_tick_count;

#ifdef CONFIG_TICKLESS_IDLE
extern int32_t _sys_idle_elapsed_ticks;

void _timer_idle_enter(int32_t ticks)
{
	/* restrict ticks to max supported by RTC */
	if ((ticks < 0) || (ticks > (0x00FFFFFF / RTC_TICKS))) {
		ticks = 0x00FFFFFF / RTC_TICKS;
	}

	/* setup next RTC compare event by ticks amount */
	NRF_RTC1->CC[0] = ((_sys_clock_tick_count + ticks) * RTC_TICKS) &
			  0x00FFFFFF;

	/* TODO: check if CC is set to stale value */
}

void _timer_idle_exit(void)
{
	uint32_t elapsed_ticks;

	/* update with elapsed ticks from h/w */
	elapsed_ticks = ((NRF_RTC1->COUNTER -
			  (_sys_clock_tick_count * RTC_TICKS)) /
			 RTC_TICKS) & 0x00FFFFFF;

	/* setup next RTC compare event by 1 tick */
	NRF_RTC1->CC[0] = ((_sys_clock_tick_count + elapsed_ticks + 1) *
			   RTC_TICKS) & 0x00FFFFFF;

	/* TODO: check if CC is set to stale value */
}
#endif /* CONFIG_TICKLESS_IDLE */

static void rtc1_nrf5_isr(void *arg)
{
	ARG_UNUSED(arg);

	if (NRF_RTC1->EVENTS_COMPARE[0]) {
		NRF_RTC1->EVENTS_COMPARE[0] = 0;

#ifdef CONFIG_TICKLESS_IDLE
		/* update with elapsed ticks from h/w */
		_sys_idle_elapsed_ticks = ((NRF_RTC1->COUNTER -
					    (_sys_clock_tick_count *
					     RTC_TICKS)) / RTC_TICKS) &
					  0x00FFFFFF;
#endif

		/* setup next RTC compare event */
		NRF_RTC1->CC[0] = ((_sys_clock_tick_count +
				    _sys_idle_elapsed_ticks + 1) * RTC_TICKS)
				  & 0x00FFFFFF;

		/* TODO: check if CC is set to stale value */

		_sys_clock_tick_announce();
	}
}

int _sys_clock_driver_init(struct device *device)
{
	struct device *clock;

	ARG_UNUSED(device);

	clock = device_get_binding(CONFIG_CLOCK_CONTROL_NRF5_K32SRC_DRV_NAME);
	if (!clock) {
		return -1;
	}

	clock_control_on(clock, (void *)CLOCK_CONTROL_NRF5_K32SRC);

	/* TODO: replace with counter driver to access RTC */
	NRF_RTC1->PRESCALER = 0;
	NRF_RTC1->CC[0] = RTC_TICKS;
	NRF_RTC1->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
	NRF_RTC1->INTENSET = RTC_INTENSET_COMPARE0_Msk;

	IRQ_CONNECT(NRF5_IRQ_RTC1_IRQn, 1, rtc1_nrf5_isr, 0, 0);
	irq_enable(NRF5_IRQ_RTC1_IRQn);

	NRF_RTC1->TASKS_START = 1;

	return 0;
}

uint32_t sys_cycle_get_32(void)
{
	uint32_t elapsed_cycles;

	elapsed_cycles = (NRF_RTC1->COUNTER -
			  (_sys_clock_tick_count * RTC_TICKS)) & 0x00FFFFFF;

	return (_sys_clock_tick_count * sys_clock_hw_cycles_per_tick) +
	       elapsed_cycles;
}
