/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#define GWU0_NODE DT_ALIAS(gwu0)
#if !DT_NODE_HAS_STATUS(GWU0_NODE, okay)
#error "Error: gwu0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec wakeupPin = GPIO_DT_SPEC_GET(GWU0_NODE, gpios);
static struct gpio_callback wakeupPin_cb_data;

volatile k_tid_t my_tid;

#define MY_STACK_SIZE 500
#define MY_PRIORITY 5
struct k_thread my_thread_data;
K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);

void my_entry_point(void *, void *, void *){
	printk("Going sleep.\n");
	k_msleep(10000);
}

void wakeup_cb(const struct device *dev, struct gpio_callback *cb,
			uint32_t pins)
{
	printk("Rising edge detected\n");
	k_thread_abort(my_tid);
}

int main(void)
{
	int ret;

	if (!gpio_is_ready_dt(&wakeupPin)) {
		printk("Error: wake-up gpio device %s is not ready\n",
		       wakeupPin.port->name);
		return 0;
	}

	// ret = gpio_pin_configure_dt(&wakeupPin, GPIO_INPUT | GPIO_PULL_DOWN | GPIO_INT_WAKEUP);
	ret = gpio_pin_configure_dt(&wakeupPin, GPIO_INPUT | GPIO_PULL_DOWN );
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, wakeupPin.port->name, wakeupPin.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&wakeupPin,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, wakeupPin.port->name, wakeupPin.pin);
		return 0;
	}

	gpio_init_callback(&wakeupPin_cb_data, wakeup_cb, BIT(wakeupPin.pin));
	gpio_add_callback(wakeupPin.port, &wakeupPin_cb_data);
	printk("Wake-up set at %s pin %d\n", wakeupPin.port->name, wakeupPin.pin);
	
	printk("Created the thread.\n");
	my_tid = k_thread_create(&my_thread_data, my_stack_area,
									K_THREAD_STACK_SIZEOF(my_stack_area),
									my_entry_point,
									NULL, NULL, NULL,
									MY_PRIORITY, 0, K_NO_WAIT);

	k_thread_join(my_tid, K_FOREVER);

	while(1)
	{
		arch_nop();
	}
	return 0;
}
