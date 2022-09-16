/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nuvoton NPCX power management driver
 *
 * This file contains the drivers of NPCX Power Manager Modules that improves
 * the efficiency of ec operation by adjusting the chip’s power consumption to
 * the level of activity required by the application. The following table
 * summarizes the main properties of the various power states and shows the
 * activity levels of the various clocks while in these power states.
 *
 * +--------------------------------------------------------------------------+
 * | Power State | LFCLK | HFCLK | APB/AHB |  Core  | RAM/Regs   | VCC | VSBY |
 * |--------------------------------------------------------------------------|
 * | Active      | On    | On    | On      | Active | Active     | On  | On   |
 * | Idle (wfi)  | On    | On    | On      | Wait   | Active     | On  | On   |
 * | Sleep       | On    | On    | Stop    | Stop   | Preserved  | On  | On   |
 * | Deep Sleep  | On    | Stop  | Stop    | Stop   | Power Down | On  | On   |
 * | Stand-By    | Off   | Off   | Off     | Off    | Off        | Off | On   |
 * +--------------------------------------------------------------------------+
 *
 * LFCLK - Low-Frequency Clock. Its frequency is fixed to 32kHz.
 * HFCLK - High-Frequency (PLL) Clock. Its frequency is configured to OFMCLK.
 *
 * Based on the following criteria:
 *
 * - A delay of 'Instant' wake-up from 'Deep Sleep' is 20 us.
 * - A delay of 'Standard' wake-up from 'Deep Sleep' is 3.43 ms.
 * - Max residency time in Deep Sleep for 'Instant' wake-up is 200 ms
 * - Min Residency time in Deep Sleep for 'Instant' wake-up is 61 us
 * - The unit to determine power state residency policy is tick.
 *
 * this driver implements one power state, PM_STATE_SUSPEND_TO_IDLE, with
 * two sub-states for power management system.
 * Sub-state 0 - "Deep Sleep" mode with “Instant” wake-up if residency time
 *               is greater or equal to 1 ms
 * Sub-state 1 - "Deep Sleep" mode with "Standard" wake-up if residency time
 *               is greater or equal to 201 ms
 *
 * INCLUDE FILES: soc_clock.h
 */

#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/zephyr.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/pm/pm.h>
#include <soc.h>

#include "soc_host.h"
#include "soc_power.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* The steps that npcx ec enters sleep/deep mode and leaves it. */
#define NPCX_ENTER_SYSTEM_SLEEP() ({                                           \
	__asm__ volatile (                                                     \
	"push {r0-r5}\n"     /* Save the registers used for delay */           \
	"wfi\n"              /* Enter sleep mode after receiving wfi */        \
	"ldm %0, {r0-r5}\n"  /* Add a delay before instructions fetching */    \
	"pop {r0-r5}\n"      /* Restore the registers used for delay */        \
	"isb\n"              /* Flush the cpu pipelines */                     \
	:: "r" (CONFIG_SRAM_BASE_ADDRESS)); /* A valid addr used for delay */  \
	})

/* Variables for tracing */
static uint32_t cnt_sleep0;
static uint32_t cnt_sleep1;

/* Supported sleep mode in npcx series */
enum {
	NPCX_SLEEP,
	NPCX_DEEP_SLEEP,
};

/* Supported wake-up mode in npcx series */
enum {
	NPCX_INSTANT_WAKE_UP,
	NPCX_STANDARD_WAKE_UP,
};

static void npcx_power_enter_system_sleep(int slp_mode, int wk_mode)
{
	/* Disable interrupts */
	__disable_irq();

	/*
	 * Disable priority mask temporarily to make sure that wake-up events
	 * are visible to the WFI instruction.
	 */
	__set_BASEPRI(0);

	/* Configure sleep/deep sleep settings in clock control module. */
	npcx_clock_control_turn_on_system_sleep(slp_mode == NPCX_DEEP_SLEEP,
					wk_mode == NPCX_INSTANT_WAKE_UP);

	/* A bypass in npcx7 series to prevent leakage in low-voltage pads */
	if (IS_ENABLED(CONFIG_SOC_SERIES_NPCX7)) {
		npcx_lvol_suspend_io_pads();
	}

	/* Turn on eSPI/LPC host access wake-up interrupt. */
	if (IS_ENABLED(CONFIG_ESPI_NPCX)) {
		npcx_host_enable_access_interrupt();
	}

	/* Turn on UART RX wake-up interrupt. */
	if (IS_ENABLED(CONFIG_UART_NPCX)) {
		npcx_uart_enable_access_interrupt();
	}

	/*
	 * Capture the reading of low-freq timer for compensation before ec
	 * enters system sleep mode.
	 */
	npcx_clock_capture_low_freq_timer();

	/* Enter system sleep mode */
	NPCX_ENTER_SYSTEM_SLEEP();

	/*
	 * Compensate system timer by the elapsed time of low-freq timer during
	 * system sleep mode.
	 */
	npcx_clock_compensate_system_timer();

	/* Turn off eSPI/LPC host access wake-up interrupt. */
	if (IS_ENABLED(CONFIG_ESPI_NPCX)) {
		npcx_host_disable_access_interrupt();
	}

	/* A bypass in npcx7 series to prevent leakage in low-voltage pads */
	if (IS_ENABLED(CONFIG_SOC_SERIES_NPCX7)) {
		npcx_lvol_restore_io_pads();
	}

	/* Turn off system sleep mode. */
	npcx_clock_control_turn_off_system_sleep();
}

/* Invoke when enter "Suspend/Low Power" mode. */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power state %u", state);
	} else {
		switch (substate_id) {
		case 0:	/* Sub-state 0: Deep sleep with instant wake-up */
			npcx_power_enter_system_sleep(NPCX_DEEP_SLEEP,
							NPCX_INSTANT_WAKE_UP);
			if (IS_ENABLED(CONFIG_NPCX_PM_TRACE)) {
				cnt_sleep0++;
			}
			break;
		case 1:	/* Sub-state 1: Deep sleep with standard wake-up */
			npcx_power_enter_system_sleep(NPCX_DEEP_SLEEP,
							NPCX_STANDARD_WAKE_UP);
			if (IS_ENABLED(CONFIG_NPCX_PM_TRACE)) {
				cnt_sleep1++;
			}
			break;
		default:
			LOG_DBG("Unsupported power substate-id %u",
				substate_id);
			break;
		}
	}
}

/* Handle soc specific activity after exiting "Suspend/Low Power" mode. */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power state %u", state);
	} else {
		switch (substate_id) {
		case 0:	/* Sub-state 0: Deep sleep with instant wake-up */
			/* Restore interrupts */
			__enable_irq();
			break;
		case 1:	/* Sub-state 1: Deep sleep with standard wake-up */
			/* Restore interrupts */
			__enable_irq();
			break;
		default:
			LOG_DBG("Unsupported power substate-id %u",
				substate_id);
			break;
		}
	}

	if (IS_ENABLED(CONFIG_NPCX_PM_TRACE)) {
		LOG_DBG("sleep: %d, deep sleep: %d", cnt_sleep0, cnt_sleep1);
		LOG_INF("total ticks in sleep: %lld",
			npcx_clock_get_sleep_ticks());
	}
}
