/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>

#include <zephyr/display/mb_display.h>

#define PERIOD_MIN     PWM_USEC(50)
#define PERIOD_MAX     PWM_USEC(3900)

#define BEEP_DURATION  K_MSEC(60)

#define NS_TO_HZ(_ns)  (NSEC_PER_SEC / (_ns))

static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_PATH(zephyr_user));

static uint32_t period;
static struct k_work beep_work;
static volatile bool beep_active;

static const struct gpio_dt_spec sw0_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec sw1_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

/* ensure SW0 & SW1 are on same gpio controller */
BUILD_ASSERT(DT_SAME_NODE(DT_GPIO_CTLR(DT_ALIAS(sw0), gpios), DT_GPIO_CTLR(DT_ALIAS(sw1), gpios)));

static void beep(struct k_work *work)
{
	/* The "period / 2" pulse duration gives 50% duty cycle, which
	 * should result in the maximum sound volume.
	 */
	pwm_set_dt(&pwm, period, period / 2U);
	k_sleep(BEEP_DURATION);

	/* Disable the PWM */
	pwm_set_pulse_dt(&pwm, 0);

	/* Ensure there's a clear silent period between two tones */
	k_sleep(K_MSEC(50));
	beep_active = false;
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
			   uint32_t pins)
{
	struct mb_display *disp;

	if (beep_active) {
		printk("Button press while beeping\n");
		return;
	}

	beep_active = true;

	if (pins & BIT(sw0_gpio.pin)) {
		printk("A pressed\n");
		if (period < PERIOD_MAX) {
			period += PWM_USEC(50U);
		}
	} else {
		printk("B pressed\n");
		if (period > PERIOD_MIN) {
			period -= PWM_USEC(50U);
		}
	}

	printk("Period is %u us (%u Hz)\n", period / NSEC_PER_USEC,
	       NS_TO_HZ(period));

	disp = mb_display_get();
	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, 500, "%uHz",
			 NS_TO_HZ(period));

	k_work_submit(&beep_work);
}

int main(void)
{
	static struct gpio_callback button_cb_data;

	if (!device_is_ready(pwm.dev)) {
		printk("%s: device not ready.\n", pwm.dev->name);
		return 0;
	}

	/* since sw0_gpio.port == sw1_gpio.port, we only need to check ready once */
	if (!device_is_ready(sw0_gpio.port)) {
		printk("%s: device not ready.\n", sw0_gpio.port->name);
		return 0;
	}

	period = pwm.period;

	gpio_pin_configure_dt(&sw0_gpio, GPIO_INPUT);
	gpio_pin_configure_dt(&sw1_gpio, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&sw0_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&sw1_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed,
			   BIT(sw0_gpio.pin) | BIT(sw1_gpio.pin));

	k_work_init(&beep_work, beep);
	/* Notify with a beep that we've started */
	k_work_submit(&beep_work);

	gpio_add_callback(sw0_gpio.port, &button_cb_data);
	return 0;
}
