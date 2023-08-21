/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#if CONFIG_SOC_RISCV_TELINK_B91
#define LED0_NODE DT_ALIAS(led0)
#endif

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
#if CONFIG_SOC_RISCV_TELINK_B91
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif

int main(void)
{
#if CONFIG_SOC_RISCV_TELINK_B91
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}
#endif

#if CONFIG_SOC_RISCV_TELINK_B92
	// add debug, PE6 output 1
	__asm__("lui t0, 0x80140"); 			//0x80140322
	__asm__("li t1, 0xbf");
	__asm__("sb t1 , 0x322(t0)");		 //0x80140322  PE oen     =  0xbf
#endif

	while (1) {
#if CONFIG_SOC_RISCV_TELINK_B91
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}
		k_msleep(SLEEP_TIME_MS);
#endif

#if CONFIG_SOC_RISCV_TELINK_B92
    	__asm__("li t2, 0x40");
    	__asm__("sb t2 , 0x323(t0)");		 //0x80140323  PE output  =  0x40
		// k_msleep(SLEEP_TIME_MS);
		delay_ms( 1000 );

	    __asm__("li t2, 0x00");
	    __asm__("sb t2 , 0x323(t0)");		 //0x80140323  PE output  =  0x00
		// k_msleep(SLEEP_TIME_MS);
		delay_ms( 1000 );
#endif
	}
	return 0;
}
