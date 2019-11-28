/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <board.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include <device.h>

#include <display/mb_display.h>

#define BUZZER_PIN     EXT_P0_GPIO_PIN

#define PERIOD_MIN     50
#define PERIOD_MAX     3900
#define PERIOD_INIT    1500

#define BEEP_DURATION  K_MSEC(60)

#define US_TO_HZ(_us)  (USEC_PER_SEC / (_us))

static struct device *pwm;
static struct device *gpio;
static u32_t period = PERIOD_INIT;
static struct k_work beep_work;
static volatile bool beep_active;

static void beep(struct k_work *work)
{
	/* The "period / 2" pulse duration gives 50% duty cycle, which
	 * should result in the maximum sound volume.
	 */
	pwm_pin_set_usec(pwm, BUZZER_PIN, period, period / 2U);
	k_sleep(BEEP_DURATION);

	/* Disable the PWM */
	pwm_pin_set_usec(pwm, BUZZER_PIN, 0, 0);

	/* Ensure there's a clear silent period between two tones */
	k_sleep(K_MSEC(50));
	beep_active = false;
}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pins)
{
	struct mb_display *disp;

	if (beep_active) {
		printk("Button press while beeping\n");
		return;
	}

	beep_active = true;

	if (pins & BIT(DT_ALIAS_SW0_GPIOS_PIN)) {
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
	mb_display_print(disp, MB_DISPLAY_MODE_DEFAULT, K_MSEC(500), "%uHz",
			 US_TO_HZ(period));

	k_work_submit(&beep_work);
}

void main(void)
{
	static struct gpio_callback button_cb;

	gpio = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);

	gpio_pin_configure(gpio, DT_ALIAS_SW0_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));
	gpio_pin_configure(gpio, DT_ALIAS_SW1_GPIOS_PIN,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			    GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb, button_pressed,
			   BIT(DT_ALIAS_SW0_GPIOS_PIN) | BIT(DT_ALIAS_SW1_GPIOS_PIN));
	gpio_add_callback(gpio, &button_cb);

	pwm = device_get_binding(DT_INST_0_NORDIC_NRF_SW_PWM_LABEL);

	k_work_init(&beep_work, beep);
	/* Notify with a beep that we've started */
	k_work_submit(&beep_work);

	gpio_pin_enable_callback(gpio, DT_ALIAS_SW0_GPIOS_PIN);
	gpio_pin_enable_callback(gpio, DT_ALIAS_SW1_GPIOS_PIN);
}
