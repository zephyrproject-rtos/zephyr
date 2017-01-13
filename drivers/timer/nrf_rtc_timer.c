/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <clock_control.h>
#include <system_timer.h>
#include <drivers/clock_control/nrf5_clock_control.h>
#include <arch/arm/cortex_m/cmsis.h>

#define RTC_TICKS ((uint32_t)(((((uint64_t)1000000UL / \
				 CONFIG_SYS_CLOCK_TICKS_PER_SEC) * \
				1000000000UL) / 30517578125UL)) & 0x00FFFFFF)

extern int64_t _sys_clock_tick_count;
extern int32_t _sys_idle_elapsed_ticks;
static uint32_t rtc_clock_tick_count;

#ifdef CONFIG_TICKLESS_IDLE
static uint8_t volatile isr_req;
static uint8_t isr_ack;
#endif /* CONFIG_TICKLESS_IDLE */

static uint32_t rtc_compare_set(uint32_t rtc_ticks)
{
	uint32_t prev, cc, elapsed_ticks;
	uint8_t retry = 10;

	prev = NRF_RTC1->COUNTER;

	do {
		/* Assert if retries failed to set compare in the future */
		__ASSERT_NO_MSG(retry);
		retry--;

		/* update with elapsed ticks from h/w */
		elapsed_ticks = (prev - rtc_clock_tick_count) & 0x00FFFFFF;

		/* setup next RTC compare event by ticks */
		cc = (rtc_clock_tick_count + elapsed_ticks + rtc_ticks) &
		     0x00FFFFFF;

		NRF_RTC1->CC[0] = cc;

		prev = NRF_RTC1->COUNTER;
	} while (((cc - prev) & 0x00FFFFFF) < 3);

#ifdef CONFIG_TICKLESS_IDLE
	/* If system clock ticks have elapsed, pend RTC IRQ which will
	 * call announce
	 */
	if (elapsed_ticks >= rtc_ticks) {
		uint8_t req;

		/* pending the interrupt does not trigger the RTC event, hence
		 * use a request/ack mechanism to let the ISR know that the
		 * interrupt was requested
		 */
		req = isr_req + 1;
		if (req != isr_ack) {
			isr_req = req;
		}

		NVIC_SetPendingIRQ(NRF5_IRQ_RTC1_IRQn);
	}
#endif /* CONFIG_TICKLESS_IDLE */

	return elapsed_ticks;
}

#ifdef CONFIG_TICKLESS_IDLE
void _timer_idle_enter(int32_t ticks)
{
	/* restrict ticks to max supported by RTC */
	if ((ticks < 0) || (ticks > (0x00FFFFFF / RTC_TICKS))) {
		ticks = 0x00FFFFFF / RTC_TICKS;
	}

	/* Postpone RTC compare event by requested system clock ticks */
	rtc_compare_set(ticks * RTC_TICKS);
}

void _timer_idle_exit(void)
{
	/* Advance RTC compare event to next system clock tick */
	rtc_compare_set(RTC_TICKS);
}
#endif /* CONFIG_TICKLESS_IDLE */

static void rtc1_nrf5_isr(void *arg)
{
#ifdef CONFIG_TICKLESS_IDLE
	uint8_t req;

	ARG_UNUSED(arg);

	req = isr_req;
	/* iterate here since pending the interrupt can be done from higher
	 * priority, and thus queuing multiple triggers
	 */
	while (NRF_RTC1->EVENTS_COMPARE[0] || (req != isr_ack)) {
		uint32_t elapsed_ticks;

		NRF_RTC1->EVENTS_COMPARE[0] = 0;

		if (req != isr_ack) {
			isr_ack = req;
			req = isr_req;

			elapsed_ticks = (NRF_RTC1->COUNTER -
					 rtc_clock_tick_count)
					& 0x00FFFFFF;
		} else {
			elapsed_ticks = rtc_compare_set(RTC_TICKS);
		}
#else
	ARG_UNUSED(arg);

	if (NRF_RTC1->EVENTS_COMPARE[0]) {
		uint32_t elapsed_ticks;

		NRF_RTC1->EVENTS_COMPARE[0] = 0;

		elapsed_ticks = rtc_compare_set(RTC_TICKS);
#endif

		rtc_clock_tick_count += elapsed_ticks;
		rtc_clock_tick_count &= 0x00FFFFFF;

		/* update with elapsed ticks from the hardware */
		_sys_idle_elapsed_ticks = elapsed_ticks / RTC_TICKS;

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

uint32_t k_cycle_get_32(void)
{
	uint32_t elapsed_cycles;

	elapsed_cycles = (NRF_RTC1->COUNTER -
			  (_sys_clock_tick_count * RTC_TICKS)) & 0x00FFFFFF;

	return (_sys_clock_tick_count * sys_clock_hw_cycles_per_tick) +
	       elapsed_cycles;
}

#ifdef CONFIG_SYSTEM_CLOCK_DISABLE
/**
 *
 * @brief Stop announcing ticks into the kernel
 *
 * This routine disables the RTC1 so that timer interrupts are no
 * longer delivered.
 *
 * @return N/A
 */
void sys_clock_disable(void)
{
	irq_disable(NRF5_IRQ_RTC1_IRQn);

	NRF_RTC1->TASKS_STOP = 1;

	/* TODO: turn off (release) 32 KHz clock source.
	 * Turning off of 32 KHz clock source is not implemented in clock
	 * driver.
	 */
}
#endif /* CONFIG_SYSTEM_CLOCK_DISABLE */
