/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Low Power timer driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_lp_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
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
static uint32_t last_lptimer_value;
static struct k_spinlock lock;

static void lptimer_interrupt_handler(void *handler_arg, cyhal_lptimer_event_t event)
{
	CY_UNUSED_PARAMETER(handler_arg);
	CY_UNUSED_PARAMETER(event);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* announce the elapsed time in ms */
	uint32_t lptimer_value = cyhal_lptimer_read(&lptimer_obj);
	uint32_t delta_ticks =
		((uint64_t)(lptimer_value - last_lptimer_value) * CONFIG_SYS_CLOCK_TICKS_PER_SEC) /
		LPTIMER_FREQ;
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : (delta_ticks > 0));
	last_lptimer_value += (delta_ticks * LPTIMER_FREQ) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	k_spin_unlock(&lock, key);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	k_spinlock_key_t key = {0};

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		key = k_spin_lock(&lock);
		/* Disable the LPTIMER events */
		cyhal_lptimer_enable_event(&lptimer_obj, CYHAL_LPTIMER_COMPARE_MATCH,
					   LPTIMER_INTR_PRIORITY, false);
		k_spin_unlock(&lock, key);
		return;
	}

	/* passing ticks==1 means "announce the next tick", ticks value of zero (or even negative)
	 * is legal and treated identically: it simply indicates the kernel would like the next
	 * tick announcement as soon as possible.
	 */
	if (ticks < 1) {
		ticks = 1;
	}

	uint32_t set_ticks = ((uint32_t)(ticks)*LPTIMER_FREQ) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	key = k_spin_lock(&lock);

	/* Configure and Enable the LPTIMER events */
	cyhal_lptimer_enable_event(&lptimer_obj, CYHAL_LPTIMER_COMPARE_MATCH, LPTIMER_INTR_PRIORITY,
				   true);
	/* Set the delay value for the next wakeup interrupt */
	cyhal_lptimer_set_delay(&lptimer_obj, set_ticks);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t lptimer_value = cyhal_lptimer_read(&lptimer_obj);

	k_spin_unlock(&lock, key);

	/* gives the value of LPTIM counter (ms) since the previous 'announce' */
	uint64_t ret = (((uint64_t)(lptimer_value - last_lptimer_value)) *
			CONFIG_SYS_CLOCK_TICKS_PER_SEC) /
		       LPTIMER_FREQ;

	return (uint32_t)ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* just gives the accumulated count in a number of hw cycles */

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t lp_time = cyhal_lptimer_read(&lptimer_obj);

	k_spin_unlock(&lock, key);

	/* convert lptim count in a nb of hw cycles with precision */
	uint64_t ret = ((uint64_t)lp_time * sys_clock_hw_cycles_per_sec()) / LPTIMER_FREQ;

	/* convert in hw cycles (keeping 32bit value) */
	return (uint32_t)ret;
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

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
