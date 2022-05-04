/*
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>

#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <zephyr/arch/common/pm_s2ram.h>

#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <hal/nrf_gpio.h>

static void system_off(void)
{
	nrf_regulators_system_off(NRF_REGULATORS);
}

static void do_suspend(void)
{
	irq_lock();

	/*
	 * Save the CPU context (including the return address),set the SRAM
	 * marker and power off the system.
	 */
	arch_pm_s2ram_suspend(system_off);

	/*
	 * XXX: On resuming we return exactly *HERE*
	 */
}

void main(void)
{
	printk("S2RAM demo (%s)\n", CONFIG_BOARD);

	/*
	 * Enable wake-up button and LED1
	 */
	nrf_gpio_cfg_input(23,  NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_sense_set(23, NRF_GPIO_PIN_SENSE_LOW);
	nrf_gpio_cfg_output(28);

	/*
	 * Sleep enough time to trigger S2RAM
	 */
	while (true) {
		k_msleep(1000);
	}
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_RAM) {
		return;
	}

	do_suspend();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_RAM) {
		return;
	}

	/*
	 * Turn the LED3 on
	 */
	nrf_gpio_cfg_output(30);

	/*
	 * XXX: Read the README.rst
	 *
	 * This is not a fully functional test. To have the kernel back in a
	 * fully functional state, here we should restore all the peripherals
	 * of the platforms that will not be restored by the PM subsystem (like
	 * NVIC, PMU, RTC, etc...). We are still missing the framework and code
	 * for that, so for now we stop here.
	 */

	irq_unlock(0);

	while (true) {
		/* stop here */
	}
}
