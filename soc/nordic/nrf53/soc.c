/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF53 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF53 family processor.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/dt-bindings/regulator/nrf5x.h>
#include <soc/nrfx_coredep.h>
#include <zephyr/logging/log.h>
#include <nrf_erratas.h>
#include <hal/nrf_power.h>
#include <hal/nrf_ipc.h>
#include <helpers/nrfx_gppi.h>
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <hal/nrf_cache.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_oscillators.h>
#include <hal/nrf_regulators.h>
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#include <hal/nrf_nvmc.h>
#endif
#include <hal/nrf_wdt.h>
#include <hal/nrf_rtc.h>
#include <soc_secure.h>

#include <cmsis_core.h>

#include "nrf53_cpunet_mgmt.h"

#define PIN_XL1 0
#define PIN_XL2 1

#define RTC1_PRETICK_CC_CHAN (RTC1_CC_NUM - 1)

/* Mask of CC channels capable of generating interrupts, see nrf_rtc_timer.c */
#define RTC1_PRETICK_SELECTED_CC_MASK   BIT_MASK(CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT + 1U)
#define RTC0_PRETICK_SELECTED_CC_MASK	BIT_MASK(NRF_RTC_CC_COUNT_MAX)

#ifdef CONFIG_SOC_NRF5340_CPUAPP
#define LFXO_NODE DT_NODELABEL(lfxo)
#define HFXO_NODE DT_NODELABEL(hfxo)

/* LFXO config from DT - if the LFXO node has a status of okay, we can assign
 * P0.00 and P0.01 to the peripheral and use the load_capacitors property set
 * (or default to external if not present). If the LFXO node does not have a
 * status of okay, assign the pins for use by the app core.
 */
#if DT_NODE_HAS_STATUS_OKAY(LFXO_NODE)
#define LFXO_PIN_SEL NRF_GPIO_PIN_SEL_PERIPHERAL
#if DT_NODE_HAS_PROP(LFXO_NODE, load_capacitors)
#if DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, external)
#define LFXO_CAP NRF_OSCILLATORS_LFXO_CAP_EXTERNAL
#elif DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, internal)
#define LFXO_CAP (DT_ENUM_IDX(LFXO_NODE, load_capacitance_picofarad) + 1U)
#endif /*DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, external) */
#else
#define LFXO_CAP NRF_OSCILLATORS_LFXO_CAP_EXTERNAL
#endif /* DT_NODE_HAS_PROP(LFXO_NODE, load_capacitors) */
#endif /* DT_NODE_HAS_STATUS_OKAY(LFXO_NODE) */

/* HFXO config from DT */
#if DT_ENUM_HAS_VALUE(HFXO_NODE, load_capacitors, internal)
#define HFXO_CAP_VAL_X2 (DT_PROP(HFXO_NODE, load_capacitance_femtofarad)) * 2U / 1000U
#elif defined(CONFIG_SOC_HFXO_CAP_INTERNAL)
/* HFXO config from legacy Kconfig */
#define HFXO_CAP_VAL_X2 CONFIG_SOC_HFXO_CAP_INT_VALUE_X2
#endif
#endif /* CONFIG_SOC_NRF5340_CPUAPP */

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);


#if defined(CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND)
/* This code prevents the CPU from entering sleep again if it already
 * entered sleep 5 times within last 200 us.
 */
static bool nrf53_anomaly_160_check(void)
{
	/* System clock cycles needed to cover 200 us window. */
	const uint32_t window_cycles =
		DIV_ROUND_UP(200 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
				 1000000);
	static uint32_t timestamps[5];
	static bool timestamps_filled;
	static uint8_t current;
	uint8_t oldest = (current + 1) % ARRAY_SIZE(timestamps);
	uint32_t now = k_cycle_get_32();

	if (timestamps_filled &&
	    /* + 1 because only fully elapsed cycles need to be counted. */
	    (now - timestamps[oldest]) < (window_cycles + 1)) {
		return false;
	}

	/* Check if the CPU actually entered sleep since the last visit here
	 * (WFE/WFI could return immediately if the wake-up event was already
	 * registered).
	 */
	if (nrf_power_event_check(NRF_POWER, NRF_POWER_EVENT_SLEEPENTER)) {
		nrf_power_event_clear(NRF_POWER, NRF_POWER_EVENT_SLEEPENTER);
		/* If so, update the index at which the current timestamp is
		 * to be stored so that it replaces the oldest one, otherwise
		 * (when the CPU did not sleep), the recently stored timestamp
		 * is updated.
		 */
		current = oldest;
		if (current == 0) {
			timestamps_filled = true;
		}
	}

	timestamps[current] = k_cycle_get_32();

	return true;
}
#endif /* CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND */

#if defined(CONFIG_SOC_NRF53_RTC_PRETICK) && defined(CONFIG_SOC_NRF5340_CPUNET)

BUILD_ASSERT(!IS_ENABLED(CONFIG_WDT_NRFX),
	     "For CONFIG_SOC_NRF53_RTC_PRETICK watchdog is used internally for the pre-tick workaround on nRF5340 cpunet. Application cannot use the watchdog.");

static inline uint32_t rtc_counter_sub(uint32_t a, uint32_t b)
{
	return (a - b) & NRF_RTC_COUNTER_MAX;
}

static bool rtc_ticks_to_next_event_get(NRF_RTC_Type *rtc, uint32_t selected_cc_mask, uint32_t cntr,
					uint32_t *ticks_to_next_event)
{
	bool result = false;

	/* Let's preload register to speed-up. */
	uint32_t reg_intenset = rtc->INTENSET;

	/* Note: TICK event not handled. */

	if (reg_intenset & NRF_RTC_INT_OVERFLOW_MASK) {
		/* Overflow can generate an interrupt. */
		*ticks_to_next_event = NRF_RTC_COUNTER_MAX + 1U - cntr;
		result = true;
	}

	for (uint32_t chan = 0; chan < NRF_RTC_CC_COUNT_MAX; chan++) {
		if ((selected_cc_mask & (1U << chan)) &&
		    (reg_intenset & NRF_RTC_CHANNEL_INT_MASK(chan))) {
			/* The CC is in selected mask and is can generate an interrupt. */
			uint32_t cc = nrf_rtc_cc_get(rtc, chan);
			uint32_t ticks_to_fire = rtc_counter_sub(cc, cntr);

			if (ticks_to_fire == 0U) {
				/* When ticks_to_fire == 0, the event should have been just
				 * generated the interrupt can be already handled or be pending.
				 * However the next event is expected to be after counter wraps.
				 */
				ticks_to_fire = NRF_RTC_COUNTER_MAX + 1U;
			}

			if (!result) {
				*ticks_to_next_event = ticks_to_fire;
				result = true;
			} else if (ticks_to_fire < *ticks_to_next_event) {
				*ticks_to_next_event = ticks_to_fire;
				result = true;
			} else {
				/* CC that fires no earlier than already found. */
			}
		}
	}

	return result;
}

static void rtc_counter_synchronized_get(NRF_RTC_Type *rtc_a, NRF_RTC_Type *rtc_b,
					 uint32_t *counter_a, uint32_t *counter_b)
{
	do {
		*counter_a = nrf_rtc_counter_get(rtc_a);
		barrier_dmem_fence_full();
		*counter_b = nrf_rtc_counter_get(rtc_b);
		barrier_dmem_fence_full();
	} while (*counter_a != nrf_rtc_counter_get(rtc_a));
}

static uint8_t cpu_idle_prepare_monitor_dummy;
static bool cpu_idle_prepare_allows_sleep;

static void cpu_idle_prepare_monitor_begin(void)
{
	__LDREXB(&cpu_idle_prepare_monitor_dummy);
}

/* Returns 0 if no exception preempted since the last call to cpu_idle_prepare_monitor_begin. */
static bool cpu_idle_prepare_monitor_end(void)
{
	/* The value stored is irrelevant. If any exception took place after
	 * cpu_idle_prepare_monitor_begin, the local monitor is cleared and
	 * the store fails returning 1.
	 * See Arm v8-M Architecture Reference Manual:
	 *   Chapter B9.2 The local monitors
	 *   Chapter B9.4 Exclusive access instructions and the monitors
	 * See Arm Cortex-M33 Processor Technical Reference Manual
	 *   Chapter 3.5 Exclusive monitor
	 */
	return __STREXB(0U, &cpu_idle_prepare_monitor_dummy);
}

static void rtc_pretick_finish_previous(void)
{
	NRF_IPC->PUBLISH_RECEIVE[CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET] &=
			~IPC_PUBLISH_RECEIVE_EN_Msk;

	nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_CHANNEL_EVENT_ADDR(RTC1_PRETICK_CC_CHAN));
}


void z_arm_on_enter_cpu_idle_prepare(void)
{
	bool ok_to_sleep = true;

	cpu_idle_prepare_monitor_begin();

	uint32_t rtc_counter = 0U;
	uint32_t rtc_ticks_to_next_event = 0U;
	uint32_t rtc0_counter = 0U;
	uint32_t rtc0_ticks_to_next_event = 0U;

	rtc_counter_synchronized_get(NRF_RTC1, NRF_RTC0, &rtc_counter, &rtc0_counter);

	bool rtc_scheduled = rtc_ticks_to_next_event_get(NRF_RTC1, RTC1_PRETICK_SELECTED_CC_MASK,
							 rtc_counter, &rtc_ticks_to_next_event);

	if (rtc_ticks_to_next_event_get(NRF_RTC0, RTC0_PRETICK_SELECTED_CC_MASK, rtc0_counter,
					&rtc0_ticks_to_next_event)) {
		/* An event is scheduled on RTC0. */
		if (!rtc_scheduled) {
			rtc_ticks_to_next_event = rtc0_ticks_to_next_event;
			rtc_scheduled = true;
		} else if (rtc0_ticks_to_next_event < rtc_ticks_to_next_event) {
			rtc_ticks_to_next_event = rtc0_ticks_to_next_event;
		} else {
			/* Event on RTC0 will not happen earlier than already found. */
		}
	}

	if (rtc_scheduled) {
		static bool rtc_pretick_cc_set_on_time;
		/* The pretick should happen 1 tick before the earliest scheduled event
		 * that can trigger an interrupt.
		 */
		uint32_t rtc_pretick_cc_val = (rtc_counter + rtc_ticks_to_next_event - 1U)
						& NRF_RTC_COUNTER_MAX;

		if (rtc_pretick_cc_val != nrf_rtc_cc_get(NRF_RTC1, RTC1_PRETICK_CC_CHAN)) {
			/* The CC for pretick needs to be updated. */
			rtc_pretick_finish_previous();
			nrf_rtc_cc_set(NRF_RTC1, RTC1_PRETICK_CC_CHAN, rtc_pretick_cc_val);

			if (rtc_ticks_to_next_event >= NRF_RTC_COUNTER_MAX/2) {
				/* Pretick is scheduled so far in the future, assumed on time. */
				rtc_pretick_cc_set_on_time = true;
			} else {
				/* Let's check if we updated CC on time, so that the CC can
				 * take effect.
				 */
				barrier_dmem_fence_full();
				rtc_counter = nrf_rtc_counter_get(NRF_RTC1);
				uint32_t pretick_cc_to_counter =
						rtc_counter_sub(rtc_pretick_cc_val, rtc_counter);

				if ((pretick_cc_to_counter < 3) ||
				    (pretick_cc_to_counter >= NRF_RTC_COUNTER_MAX/2)) {
					/* The COUNTER value is close enough to the expected
					 * pretick CC or has just expired, so the pretick event
					 * generation is not guaranteed.
					 */
					rtc_pretick_cc_set_on_time = false;
				} else {
					/* The written rtc_pretick_cc is guaranteed to trigger
					 * compare event.
					 */
					rtc_pretick_cc_set_on_time = true;
				}
			}
		} else {
			/* The CC for pretick doesn't need to be updated, however
			 * rtc_pretick_cc_set_on_time still holds if we managed to set it on time.
			 */
		}

		/* If the CC for pretick is set on time, so the pretick CC event can be reliably
		 * generated then allow to sleep. Otherwise (the CC for pretick cannot be reliably
		 * generated, because CC was set very short to it's fire time) sleep not at all.
		 */
		ok_to_sleep = rtc_pretick_cc_set_on_time;
	} else {
		/* No events on any RTC timers are scheduled. */
	}

	if (ok_to_sleep) {
		NRF_IPC->PUBLISH_RECEIVE[CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET] |=
			IPC_PUBLISH_RECEIVE_EN_Msk;
		if (!nrf_rtc_event_check(NRF_RTC1,
				NRF_RTC_CHANNEL_EVENT_ADDR(RTC1_PRETICK_CC_CHAN))) {
			NRF_WDT->TASKS_STOP = 1;
			/* Check if any event did not occur after we checked for
			 * stopping condition. If yes, we might have stopped WDT
			 * when it should be running. Restart it.
			 */
			if (nrf_rtc_event_check(NRF_RTC1,
					NRF_RTC_CHANNEL_EVENT_ADDR(RTC1_PRETICK_CC_CHAN))) {
				NRF_WDT->TASKS_START = 1;
			}
		}
	}

	cpu_idle_prepare_allows_sleep = ok_to_sleep;
}
#endif /* CONFIG_SOC_NRF53_RTC_PRETICK && CONFIG_SOC_NRF5340_CPUNET */

#if defined(CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND) || \
	(defined(CONFIG_SOC_NRF53_RTC_PRETICK) && defined(CONFIG_SOC_NRF5340_CPUNET))
bool z_arm_on_enter_cpu_idle(void)
{
	bool ok_to_sleep = true;

#if defined(CONFIG_SOC_NRF53_RTC_PRETICK) && defined(CONFIG_SOC_NRF5340_CPUNET)
	if (cpu_idle_prepare_monitor_end() == 0) {
		/* No exception happened since cpu_idle_prepare_monitor_begin.
		 * We can trust the outcome of. z_arm_on_enter_cpu_idle_prepare
		 */
		ok_to_sleep = cpu_idle_prepare_allows_sleep;
	} else {
		/* Exception happened since cpu_idle_prepare_monitor_begin.
		 * The values which z_arm_on_enter_cpu_idle_prepare could be changed
		 * by the exception, so we can not trust to it's outcome.
		 * Do not sleep at all, let's try in the next iteration of idle loop.
		 */
		ok_to_sleep = false;
	}
#endif

#if defined(CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND)
	if (ok_to_sleep) {
		ok_to_sleep = nrf53_anomaly_160_check();

#if (LOG_LEVEL >= LOG_LEVEL_DBG)
		static bool suppress_message;

		if (ok_to_sleep) {
			suppress_message = false;
		} else if (!suppress_message) {
			LOG_DBG("Anomaly 160 trigger conditions detected.");
			suppress_message = true;
		}
#endif
	}
#endif /* CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND */

#if defined(CONFIG_SOC_NRF53_RTC_PRETICK) && defined(CONFIG_SOC_NRF5340_CPUNET)
	if (!ok_to_sleep) {
		NRF_IPC->PUBLISH_RECEIVE[CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET] &=
			~IPC_PUBLISH_RECEIVE_EN_Msk;
		NRF_WDT->TASKS_STOP = 1;
	}
#endif

	return ok_to_sleep;
}
#endif /* CONFIG_SOC_NRF53_ANOMALY_160_WORKAROUND ||
	* (CONFIG_SOC_NRF53_RTC_PRETICK && CONFIG_SOC_NRF5340_CPUNET)
	*/

#if CONFIG_SOC_NRF53_RTC_PRETICK
#ifdef CONFIG_SOC_NRF5340_CPUAPP
/* RTC pretick - application core part. */
static int rtc_pretick_cpuapp_init(void)
{
	uint8_t ch;
	nrfx_err_t err;
	nrf_ipc_event_t ipc_event =
		nrf_ipc_receive_event_get(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_FROM_NET);
	nrf_ipc_task_t ipc_task =
		nrf_ipc_send_task_get(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET);
	uint32_t task_ipc = nrf_ipc_task_address_get(NRF_IPC, ipc_task);
	uint32_t evt_ipc = nrf_ipc_event_address_get(NRF_IPC, ipc_event);

	err = nrfx_gppi_channel_alloc(&ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	nrf_ipc_receive_config_set(NRF_IPC, CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_FROM_NET,
				   BIT(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_FROM_NET));
	nrf_ipc_send_config_set(NRF_IPC, CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET,
				   BIT(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET));

	nrfx_gppi_task_endpoint_setup(ch, task_ipc);
	nrfx_gppi_event_endpoint_setup(ch, evt_ipc);
	nrfx_gppi_channels_enable(BIT(ch));

	return 0;
}
#else /* CONFIG_SOC_NRF5340_CPUNET */

void rtc_pretick_rtc0_isr_hook(void)
{
	rtc_pretick_finish_previous();
}

void rtc_pretick_rtc1_isr_hook(void)
{
	rtc_pretick_finish_previous();
}

static int rtc_pretick_cpunet_init(void)
{
	uint8_t ppi_ch;
	nrf_ipc_task_t ipc_task =
		nrf_ipc_send_task_get(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_FROM_NET);
	nrf_ipc_event_t ipc_event =
		nrf_ipc_receive_event_get(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET);
	uint32_t task_ipc = nrf_ipc_task_address_get(NRF_IPC, ipc_task);
	uint32_t evt_ipc = nrf_ipc_event_address_get(NRF_IPC, ipc_event);
	uint32_t task_wdt = nrf_wdt_task_address_get(NRF_WDT, NRF_WDT_TASK_START);
	uint32_t evt_cc = nrf_rtc_event_address_get(NRF_RTC1,
				NRF_RTC_CHANNEL_EVENT_ADDR(RTC1_PRETICK_CC_CHAN));

	/* Configure Watchdog to allow stopping. */
	nrf_wdt_behaviour_set(NRF_WDT, WDT_CONFIG_STOPEN_Msk | BIT(4));
	*((volatile uint32_t *)0x41203120) = 0x14;

	/* Configure IPC */
	nrf_ipc_receive_config_set(NRF_IPC, CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET,
				   BIT(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_TO_NET));
	nrf_ipc_send_config_set(NRF_IPC, CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_FROM_NET,
				   BIT(CONFIG_SOC_NRF53_RTC_PRETICK_IPC_CH_FROM_NET));

	/* Allocate PPI channel for RTC Compare event publishers that starts WDT. */
	nrfx_err_t err = nrfx_gppi_channel_alloc(&ppi_ch);

	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	nrfx_gppi_event_endpoint_setup(ppi_ch, evt_cc);
	nrfx_gppi_task_endpoint_setup(ppi_ch, task_ipc);
	nrfx_gppi_event_endpoint_setup(ppi_ch, evt_ipc);
	nrfx_gppi_task_endpoint_setup(ppi_ch, task_wdt);
	nrfx_gppi_channels_enable(BIT(ppi_ch));

	nrf_rtc_event_enable(NRF_RTC1, NRF_RTC_CHANNEL_INT_MASK(RTC1_PRETICK_CC_CHAN));
	nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_CHANNEL_EVENT_ADDR(RTC1_PRETICK_CC_CHAN));

	return 0;
}
#endif /* CONFIG_SOC_NRF5340_CPUNET */

static int rtc_pretick_init(void)
{
#ifdef CONFIG_SOC_NRF5340_CPUAPP
	return rtc_pretick_cpuapp_init();
#else
	return rtc_pretick_cpunet_init();
#endif
}
#endif /* CONFIG_SOC_NRF53_RTC_PRETICK */

void soc_early_init_hook(void)
{
#if defined(CONFIG_SOC_NRF5340_CPUAPP) && defined(CONFIG_NRF_ENABLE_CACHE)
#if !defined(CONFIG_BUILD_WITH_TFM)
	/* Enable the instruction & data cache.
	 * This can only be done from secure code.
	 * This is handled by the TF-M platform so we skip it when TF-M is
	 * enabled.
	 */
	nrf_cache_enable(NRF_CACHE);
#endif
#elif defined(CONFIG_SOC_NRF5340_CPUNET) && defined(CONFIG_NRF_ENABLE_CACHE)
	nrf_nvmc_icache_config_set(NRF_NVMC, NRF_NVMC_ICACHE_ENABLE);
#endif

#ifdef CONFIG_SOC_NRF5340_CPUAPP
#if DT_NODE_HAS_STATUS_OKAY(LFXO_NODE)
	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS, LFXO_CAP);
#if !defined(CONFIG_BUILD_WITH_TFM)
	/* This can only be done from secure code.
	 * This is handled by the TF-M platform so we skip it when TF-M is
	 * enabled.
	 */
	nrf_gpio_pin_control_select(PIN_XL1, LFXO_PIN_SEL);
	nrf_gpio_pin_control_select(PIN_XL2, LFXO_PIN_SEL);
#endif /* !defined(CONFIG_BUILD_WITH_TFM) */
#endif /* DT_NODE_HAS_STATUS_OKAY(LFXO_NODE) */
#if defined(HFXO_CAP_VAL_X2)
	/* This register is only accessible from secure code. */
	uint32_t xosc32mtrim = soc_secure_read_xosc32mtrim();
	/* The SLOPE field is in the two's complement form, hence this special
	 * handling. Ideally, it would result in just one SBFX instruction for
	 * extracting the slope value, at least gcc is capable of producing such
	 * output, but since the compiler apparently tries first to optimize
	 * additions and subtractions, it generates slightly less than optimal
	 * code.
	 */
	uint32_t slope_field = (xosc32mtrim & FICR_XOSC32MTRIM_SLOPE_Msk)
			       >> FICR_XOSC32MTRIM_SLOPE_Pos;
	uint32_t slope_mask = FICR_XOSC32MTRIM_SLOPE_Msk
			      >> FICR_XOSC32MTRIM_SLOPE_Pos;
	uint32_t slope_sign = (slope_mask - (slope_mask >> 1));
	int32_t slope = (int32_t)(slope_field ^ slope_sign) - (int32_t)slope_sign;
	uint32_t offset = (xosc32mtrim & FICR_XOSC32MTRIM_OFFSET_Msk)
			  >> FICR_XOSC32MTRIM_OFFSET_Pos;
	/* As specified in the nRF5340 PS:
	 * CAPVALUE = (((FICR->XOSC32MTRIM.SLOPE+56)*(CAPACITANCE*2-14))
	 *            +((FICR->XOSC32MTRIM.OFFSET-8)<<4)+32)>>6;
	 * where CAPACITANCE is the desired capacitor value in pF, holding any
	 * value between 7.0 pF and 20.0 pF in 0.5 pF steps.
	 */
	uint32_t capvalue =
		((slope + 56) * (HFXO_CAP_VAL_X2 - 14)
		 + ((offset - 8) << 4) + 32) >> 6;

	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, true, capvalue);
#elif defined(CONFIG_SOC_HFXO_CAP_EXTERNAL) || \
	DT_ENUM_HAS_VALUE(HFXO_NODE, load_capacitors, external)
	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, false, 0);
#endif
#endif /* CONFIG_SOC_NRF5340_CPUAPP */

#if defined(CONFIG_SOC_DCDC_NRF53X_APP) || \
	(DT_PROP(DT_NODELABEL(vregmain), regulator_initial_mode) == NRF5X_REG_MODE_DCDC)
	nrf_regulators_vreg_enable_set(NRF_REGULATORS, NRF_REGULATORS_VREG_MAIN, true);
#endif
#if defined(CONFIG_SOC_DCDC_NRF53X_NET) || \
	(DT_PROP(DT_NODELABEL(vregradio), regulator_initial_mode) == NRF5X_REG_MODE_DCDC)
	nrf_regulators_vreg_enable_set(NRF_REGULATORS, NRF_REGULATORS_VREG_RADIO, true);
#endif
#if defined(CONFIG_SOC_DCDC_NRF53X_HV) || DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(vregh))
	nrf_regulators_vreg_enable_set(NRF_REGULATORS, NRF_REGULATORS_VREG_HIGH, true);
#endif

#if defined(CONFIG_SOC_NRF53_CPUNET_MGMT)
	int err = nrf53_cpunet_mgmt_init();

	__ASSERT_NO_MSG(err == 0);
	(void)err;
#endif
}

void soc_late_init_hook(void)
{
#ifdef CONFIG_SOC_NRF53_RTC_PRETICK
	int err = rtc_pretick_init();

	__ASSERT_NO_MSG(err == 0);
	(void)err;
#endif

#ifdef CONFIG_SOC_NRF53_CPUNET_ENABLE
	nrf53_cpunet_init();
#endif
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}
