/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>

#include <zephyr/display/mb_display.h>

#define BUZZER_PWM_CHANNEL 0

#define PERIOD_MIN     50
#define PERIOD_MAX     3900
#define PERIOD_INIT    1500

#define BEEP_DURATION  K_MSEC(60)

#define US_TO_HZ(_us)  (USEC_PER_SEC / (_us))

static const struct device *pwm;
static const struct device *gpio;
static uint32_t period = PERIOD_INIT;
static struct k_work beep_work;
static volatile bool beep_active;

static void beep(struct k_work *work)
{
	/* The "period / 2" pulse duration gives 50% duty cycle, which
	 * should result in the maximum sound volume.
	 */
	pwm_set(pwm, BUZZER_PWM_CHANNEL, PWM_USEC(period), PWM_USEC(period) / 2U, 0);
	k_sleep(BEEP_DURATION);

	/* Disable the PWM */
	pwm_set(pwm, BUZZER_PWM_CHANNEL, 0, 0, 0);

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

	if (pins & BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios))) {
		printk("A pressed\n");
		if (period < PERIOD_MAX) {
			period += 50U;
		}
	} else {
		printk("B pressed\n");
		if (period > PERIOD_MIN) {
			period -= 50U;
		}
	}

	printk("Period is %u us (%u Hz)\n", period, US_TO_HZ(period));

	disp = mb_display_get();
	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, 500, "%uHz",
			 US_TO_HZ(period));

	k_work_submit(&beep_work);
}

void main(void)
{
	static struct gpio_callback button_cb_data;

	gpio = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));

	gpio_pin_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
			   DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios) | GPIO_INPUT);
	gpio_pin_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw1), gpios),
			   DT_GPIO_FLAGS(DT_ALIAS(sw1), gpios) | GPIO_INPUT);

	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

	gpio_pin_interrupt_configure(gpio, DT_GPIO_PIN(DT_ALIAS(sw1), gpios),
				     GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&button_cb_data, button_pressed,
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)) |
			   BIT(DT_GPIO_PIN(DT_ALIAS(sw1), gpios)));

	pwm = device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_sw_pwm)));

	k_work_init(&beep_work, beep);
	/* Notify with a beep that we've started */
	k_work_submit(&beep_work);

	gpio_add_callback(gpio, &button_cb_data);
}
