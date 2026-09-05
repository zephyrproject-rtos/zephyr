/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Low Power timer driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_lp_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/clock.h>
#include <zephyr/drivers/gpio.h>

#include <cyhal_lptimer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_lp_timer, CONFIG_KERNEL_LOG_LEVEL);

/* The application only needs one lptimer. Report an error if more than one is selected. */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIMER instance should be enabled
#endif

#define LPTIMER_INTR_PRIORITY (3u)
#define LPTIMER_FREQ          (32768u)

/* We need to know the number of MCWDT instances.  Unfortunately, this information is not available
 * in a header in the HAL code.  This was extracted from the cyhal_lptimer.c file in the HAL
 */
#if (defined(CY_IP_MXS40SRSS) || defined(CY_IP_MXS40SSRSS) || defined(CY_IP_MXS28SRSS) ||          \
	defined(CY_IP_MXS22SRSS)) &&                                                               \
	!((defined(CY_IP_MXS40SRSS) && (CY_IP_MXS40SRSS_VERSION >= 3)) ||                          \
	((SRSS_NUM_MCWDT_B) > 0))
#define NUM_LPTIMERS SRSS_NUM_MCWDT
#else
#error "Selected device doesn't support low power timers at this time."
#endif

cyhal_lptimer_t lptimer_obj;

/* Set while sys_clock_no_timeout() holds the match interrupt masked. */
static bool lptimer_event_masked;

static void lptimer_event_set(bool enable)
{
	cyhal_lptimer_enable_event(&lptimer_obj, CYHAL_LPTIMER_COMPARE_MATCH,
				   LPTIMER_INTR_PRIORITY, enable);
}

/*
 * A free-running 32-bit counter at the low-frequency clock rate, matched by a
 * comparator that fires on equality alone, so a COMPARE_EXACT backend. The HAL
 * turns the absolute match back into a delay from the current count and clamps
 * that delay, so a target already behind would be programmed some 36 hours out
 * rather than fire; the core's verify loop is what catches it.
 *
 * The kernel counts in these same cycles, so the core emits both cycle getters
 * and nothing here needs to scale.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT

BUILD_ASSERT(LPTIMER_FREQ == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "the lp-timer counts clk_lf, so CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC must be "
	     "that rate; see the SoC Kconfig.defconfig");

static inline uint32_t timer_driver_cycle_get(void)
{
	return cyhal_lptimer_read(&lptimer_obj);
}

static inline void timer_driver_set_compare(uint32_t cycles)
{
	if (lptimer_event_masked) {
		lptimer_event_masked = false;
		lptimer_event_set(true);
	}
	cyhal_lptimer_set_match(&lptimer_obj, cycles);
}

#include "system_timer_generic.h"

static void lptimer_interrupt_handler(void *handler_arg, cyhal_lptimer_event_t event)
{
	CY_UNUSED_PARAMETER(handler_arg);
	CY_UNUSED_PARAMETER(event);

	timer_core_announce();
}

void sys_clock_no_timeout(void)
{
	/* No announcement is wanted, so mask the match interrupt. The counter
	 * keeps running, which is what sys_clock_cycle_get_32() reads, and the
	 * next arming unmasks it.
	 */
	lptimer_event_masked = true;
	lptimer_event_set(false);
}

static int sys_clock_driver_init(void)
{
	cy_rslt_t result;
	cyhal_lptimer_t lptimer_objs[NUM_LPTIMERS];

	/* Currently with the HAL, there is no way to directly/explicitly select the MCWDT
	 * enabled in the <board>.dts file.  So, instead, initialize LPTIMERs until we find
	 * the one from the <board>.dts file.  Free the others when done.
	 */
	for (int32_t lptimer_index = 0; lptimer_index < NUM_LPTIMERS; lptimer_index++) {
		/* Initialize the LPTIMER with default configuration */
		result = cyhal_lptimer_init(&lptimer_obj);

		if (result != CY_RSLT_SUCCESS) {
			LOG_ERR("LPTimer instance not found. Error: 0x%08X\n",
				(unsigned int)result);
			return -EIO;
		}

		if ((uint32_t)lptimer_obj.base == DT_INST_REG_ADDR(0)) {
			for (lptimer_index--; lptimer_index >= 0; lptimer_index--) {
				cyhal_lptimer_free(&lptimer_objs[lptimer_index]);
			}
			break;
		}

		cyhal_lptimer_free(&lptimer_obj);
		cyhal_lptimer_init(&lptimer_objs[lptimer_index]);
	}

	/* Register the callback handler which will be invoked when the interrupt triggers */
	cyhal_lptimer_register_callback(&lptimer_obj, lptimer_interrupt_handler, NULL);

	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("Sys Clock initialization failed. Error: 0x%08X\n", (unsigned int)result);
		return -EIO;
	}

	/* Unmask the match interrupt once here rather than on every arming.
	 * Only sys_clock_no_timeout() masks it again.
	 */
	lptimer_event_set(true);

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
