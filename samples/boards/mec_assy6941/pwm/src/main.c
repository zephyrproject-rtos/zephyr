/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2022 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#include <soc.h>

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
const struct gpio_dt_spec button_s4_dt = GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, gpios, 0);

static const struct device *pwm_dev = DEVICE_DT_GET(DT_NODELABEL(pwm0));

static volatile uint32_t spin_val;
static volatile int ret_val;
static volatile bool run;

static void spin_on(uint32_t id, int rval);

#define GPIO_0162_PIN_MASK BIT(18)
static void gpio_cb(const struct device *port,
		    struct gpio_callback *cb,
		    gpio_port_pins_t pins);

static struct gpio_callback gpio_cb_button_s4;
static struct k_sem button_s4_sync;

int main(void)
{
	int ret = 0;
	uint32_t channel = 0, flags = 0;
	uint32_t period_cycles = 0, pulse_cycles = 0;
	uint32_t period_cycles2 = 0, pulse_cycles2 = 0;
	uint64_t cycles = 0;

	LOG_INF("MEC5 PWMN sample: board: %s", DT_N_P_compatible_IDX_0);

	k_sem_init(&button_s4_sync, 0, 1);

	if (!device_is_ready(pwm_dev)) {
		LOG_ERR("PWM device is not ready! (%d)", -3);
		spin_on((uint32_t)__LINE__, -3);
	}

	ret = gpio_pin_configure_dt(&button_s4_dt, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Configure Target_nReady GPIO as output hi (%d)", ret);
		spin_on((uint32_t)__LINE__, 2);
	}

	gpio_init_callback(&gpio_cb_button_s4, gpio_cb, GPIO_0162_PIN_MASK);
	ret = gpio_add_callback_dt(&button_s4_dt, &gpio_cb_button_s4);
	if (ret) {
		LOG_ERR("GPIO add callback for button S4 pin error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	/* button OFF is low, depressed is high, trigger on rising edge */
	ret =  gpio_pin_interrupt_configure_dt(&button_s4_dt,
					       (GPIO_INPUT | GPIO_INT_EDGE_RISING));
	if (ret) {
		LOG_ERR("GPIO interrupt config for button S4 error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	run = true;

	ret = pwm_get_cycles_per_sec(pwm_dev, channel, &cycles);
	if (ret) {
		LOG_ERR("PWM API get cycles error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	LOG_INF("PWM get cycles returned %llu cycles per second", cycles);

	/* 1KHz 50% duty cycle */
	period_cycles = (uint32_t)(cycles / 1000u);
	pulse_cycles = period_cycles / 2u;

	/* 5KHz 33% duty cycles */
	period_cycles2 = (uint32_t)(cycles / 5000u);
	pulse_cycles2 = period_cycles2 / 3u;

	while (run) {
		LOG_INF("1KHz 50 percent duty: period cycles = %u  pulse cycles = %u",
			period_cycles, pulse_cycles);
		ret = pwm_set_cycles(pwm_dev, channel, period_cycles, pulse_cycles, flags);
		if (ret) {
			LOG_ERR("PWM API set cycles error (%d)", ret);
			spin_on((uint32_t)__LINE__, ret);
		}

		k_sem_take(&button_s4_sync, K_FOREVER); /* decrements count */
		/* k_sem_reset(&button_s4_sync); */

		LOG_INF("5KHz 33 percent duty: period cycles = %u  pulse cycles = %u",
			period_cycles2, pulse_cycles2);
		ret = pwm_set_cycles(pwm_dev, channel, period_cycles2, pulse_cycles2, flags);
		if (ret) {
			LOG_ERR("PWM API set cycles error (%d)", ret);
			spin_on((uint32_t)__LINE__, ret);
		}

		k_sem_take(&button_s4_sync, K_FOREVER);
		/* k_sem_reset(&button_s4_sync); */

		LOG_INF("PWM turn off");
		ret = pwm_set_cycles(pwm_dev, channel, period_cycles2, 0, flags);
		if (ret) {
			LOG_ERR("PWM API set OFF cycles error (%d)", ret);
			spin_on((uint32_t)__LINE__, ret);
		}

		k_sem_take(&button_s4_sync, K_FOREVER);
		/* k_sem_reset(&button_s4_sync); */

		LOG_INF("PWM turn ON");
		ret = pwm_set_cycles(pwm_dev, channel, period_cycles2, period_cycles2, flags);
		if (ret) {
			LOG_ERR("PWM API set ON cycles error (%d)", ret);
			spin_on((uint32_t)__LINE__, ret);
		}

		k_sem_take(&button_s4_sync, K_FOREVER);
		/* k_sem_reset(&button_s4_sync); */

	}

	LOG_INF("Application Done (%d)", ret);
	spin_on(256, 0);

	return 0;
}

/* GPIO callbacks */
static void gpio_cb(const struct device *port,
		    struct gpio_callback *cb,
		    gpio_port_pins_t pins)
{
	if (pins & GPIO_0162_PIN_MASK) {
		LOG_INF("GPIO CB: Button S4 falling edge");
		k_sem_give(&button_s4_sync);
	} else {
		LOG_ERR("GPIO CB: unknown pin 0x%08x", pins);
	}
}

static void spin_on(uint32_t id, int rval)
{
	spin_val = id;
	ret_val = rval;

	log_panic(); /* flush log buffers */

	while (spin_val) {
		;
	}
}
