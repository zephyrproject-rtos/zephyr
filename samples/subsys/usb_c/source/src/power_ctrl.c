/*
 * Copyright (c) 2023 The Chromium OS Authors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/usb_c/usbc.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(power_ctrl, LOG_LEVEL_DBG);

#include "power_ctrl.h"

#define PORT1_DCDC_DETECT_NODE	DT_PATH(dcdc_detect)

#define PORT1_SOURCE_EN_NODE	DT_NODELABEL(source_en)
#define PORT1_DCDC_EN_NODE	DT_NODELABEL(dcdc_en)
#define PORT1_PWM_CTL_NODE	DT_NODELABEL(pwm_ctl)
#define PORT1_VCONN1_EN_NODE	DT_NODELABEL(vconn1_en)
#define PORT1_VCONN2_EN_NODE	DT_NODELABEL(vconn2_en)

/* DCDC Voltage is 19V and setting min threshold to 18V */
#define MIN_DCDC_DETECT_MV 18000

#define PWM_FOR_15V	45000
#define PWM_FOR_9V	30000
#define PWM_FOR_5V	21500
#define PWM_FOR_0V	0

static const struct gpio_dt_spec source_en = GPIO_DT_SPEC_GET(PORT1_SOURCE_EN_NODE, gpios);
static const struct gpio_dt_spec dcdc_en = GPIO_DT_SPEC_GET(PORT1_DCDC_EN_NODE, gpios);

static const struct gpio_dt_spec vconn1_en = GPIO_DT_SPEC_GET(PORT1_VCONN1_EN_NODE, gpios);
static const struct gpio_dt_spec vconn2_en = GPIO_DT_SPEC_GET(PORT1_VCONN2_EN_NODE, gpios);

static const struct pwm_dt_spec pwm_ctl = PWM_DT_SPEC_GET(PORT1_PWM_CTL_NODE);

int vconn_ctrl_set(enum vconn_t v)
{
	switch (v) {
	case VCONN_OFF:
		gpio_pin_set_dt(&vconn1_en, 0);
		gpio_pin_set_dt(&vconn2_en, 0);
		break;
	case VCONN1_ON:
		gpio_pin_set_dt(&vconn1_en, 1);
		gpio_pin_set_dt(&vconn2_en, 0);
		break;
	case VCONN2_ON:
		gpio_pin_set_dt(&vconn1_en, 0);
		gpio_pin_set_dt(&vconn2_en, 1);
		break;
	};

	return 0;
}

int source_ctrl_set(enum source_t v)
{
	uint32_t pwmv;

	switch (v) {
	case SOURCE_0V:
		pwmv = PWM_FOR_0V;
		break;
	case SOURCE_5V:
		pwmv = PWM_FOR_5V;
		break;
	case SOURCE_9V:
		pwmv = PWM_FOR_9V;
		break;
	case SOURCE_15V:
		pwmv = PWM_FOR_15V;
		break;
	default:
		pwmv = PWM_FOR_0V;
	}

	return pwm_set_pulse_dt(&pwm_ctl, pwmv);
}

static int power_ctrl_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&source_en)) {
		LOG_ERR("Error: Source Enable device %s is not ready",
			source_en.port->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&dcdc_en)) {
		LOG_ERR("Error: DCDC Enable device %s is not ready",
			dcdc_en.port->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&vconn1_en)) {
		LOG_ERR("Error: VCONN1 Enable device %s is not ready",
			vconn1_en.port->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&vconn2_en)) {
		LOG_ERR("Error: VCONN2 Enable device %s is not ready",
			vconn2_en.port->name);
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(&pwm_ctl)) {
		LOG_ERR("Error: PWM CTL device is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&source_en, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure Source Enable device %s pin %d",
			ret, source_en.port->name, source_en.pin);
		return ret;
	}

	ret = gpio_pin_configure_dt(&dcdc_en, GPIO_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure DCDC Enable device %s pin %d",
			ret, dcdc_en.port->name, dcdc_en.pin);
		return ret;
	}

	ret = gpio_pin_configure_dt(&vconn1_en, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure VCONN1 Enable device %s pin %d",
			ret, vconn1_en.port->name, vconn1_en.pin);
		return ret;
	}

	ret = gpio_pin_configure_dt(&vconn2_en, GPIO_OUTPUT | GPIO_OPEN_DRAIN);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure VCONN2 Enable device %s pin %d",
			ret, vconn2_en.port->name, vconn1_en.pin);
		return ret;
	}

	ret = gpio_pin_set_dt(&source_en, 1);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to enable source", ret);
		return ret;
	}

	ret = gpio_pin_set_dt(&dcdc_en, 1);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to enable dcdc converter", ret);
		return ret;
	}

	vconn_ctrl_set(VCONN_OFF);

	ret = source_ctrl_set(SOURCE_0V);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to set VBUS to 0V", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(power_ctrl_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
