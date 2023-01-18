/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <zephyr/arch/arm/aarch32/exc.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void timer_handler(struct k_timer *timer)
{
	gpio_pin_toggle_dt(&led);
}
K_TIMER_DEFINE(led_timer, timer_handler, NULL);

int debug_mon_enable(void)
{
	/*
	 * Cannot enable monitor mode if C_DEBUGEN bit is set. This bit can only be
	 * altered from debug access port. It is cleared on power-on-reset.
	 */
	bool is_in_halting_mode = (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 1;

	if (is_in_halting_mode) {
		return -1;
	}

	/* Enable monitor mode debugging by setting MON_EN bit of DEMCR */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk;
	return 0;
}

/*
 * Interrupt handler that will be called each time the device
 * enters a breakpoint while debugging in monitor mode.
 * With CONFIG_CORTEX_M_DEBUG_MONITOR_HOOK enabled this exception is set to lowest
 * possible priority (IRQ_PRIO_LOWEST).
 */
void z_arm_debug_monitor(void)
{
	/*
	 * Implement the logic or use a ready implementation available for your platform
	 * (ie. SEGGER implementation for their debug probes)
	 */
	printk("Entered debug monitor interrupt\n");

	/* Spin in breakpoint. Other, higher-priority interrupts will continue to execute */
	while (true)
		;
}

void main(void)
{
	/* Set up led and led timer */
	if (!device_is_ready(led.port)) {
		printk("Device not ready\n");
		return;
	}

	int err = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	if (err) {
		printk("Error configuring LED\n");
		return;
	}
	k_timer_start(&led_timer, K_NO_WAIT, K_SECONDS(1));

	/* Set up debug monitor */
	err = debug_mon_enable();
	if (err) {
		printk("Error enabling monitor mode:\n"
			"Cannot enable DBM when CPU is in Debug mode");
		return;
	}

	/* Enter a breakpoint */
	__asm("bkpt");
}
