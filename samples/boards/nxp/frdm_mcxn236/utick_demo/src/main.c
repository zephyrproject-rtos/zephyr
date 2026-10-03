/*
 * Copyright 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <fsl_clock.h>
#include <fsl_utick.h>

/* The MCUX SDK UTICK driver provides the peripheral ISR implementation. */
extern void UTICK0_DriverIRQHandler(void);

#define LED_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED_NODE, okay)
#error "This demo requires led0"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static volatile uint32_t tick_count;

static void utick_callback(void)
{
	/* Keep the ISR short; do printing and GPIO updates from the main thread. */
	tick_count++;
}

int main(void)
{
	uint32_t handled_ticks = 0;
	int ret;

	printk("MCXN236 UTICK demo: 1 MHz clock, 1 second repeat interval\n");

	if (!gpio_is_ready_dt(&led)) {
		printk("LED GPIO is not ready\n");
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		printk("LED setup failed: %d\n", ret);
		return 0;
	}

	/* Use the internal 1 MHz source and divide by one for easy timing math. */
	CLOCK_SetupClockCtrl(kCLOCK_FRO1MHZ_ENA | kCLOCK_FRO1MHZ_CLK_ENA);
	CLOCK_SetClkDiv(kCLOCK_DivUtickClk, 1U);
	CLOCK_AttachClk(kCLK_1M_to_UTICK);

	UTICK_Init(UTICK0_NS);
	/* Route the UTICK IRQ through Zephyr to the MCUX SDK driver's ISR. */
    IRQ_CONNECT(UTICK0_IRQn, 2, UTICK0_DriverIRQHandler, NULL, 0);
    irq_enable(UTICK0_IRQn);
	/* DELAYVAL + 1 = 1,000,000 timer clocks = one second. */
	UTICK_SetTick(UTICK0_NS, kUTICK_Repeat, 999999U, utick_callback);

	while (true) {
		uint32_t current = tick_count;

		if (current != handled_ticks) {
			handled_ticks = current;
			gpio_pin_toggle_dt(&led);
			printk("UTICK interrupt %u\n", handled_ticks);
		}

		k_sleep(K_MSEC(10));
	}

	return 0;
}
