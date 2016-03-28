/* pinmux_board_galileo.c - pin out mapping for the Galileo board */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <nanokernel.h>
#include <board.h>
#include <device.h>
#include <init.h>

#include <pinmux.h>
#include <i2c.h>
#include <gpio.h>
#include <pwm.h>

#include <pinmux/pinmux.h>

#include "pinmux_galileo.h"

/* Alter this table to change the default pin settings on the Galileo Gen2
 * boards.  Specifically change the PINMUX_FUNC_* value to represent the
 * functionaltiy desired
 */
static struct pin_config mux_config[PINMUX_NUM_PINS] = {
	/* pin, selected mode    <mode A, mode B, mode C, mode D> */
	/* Analog Inputs */
	{ 0,  PINMUX_FUNC_C }, /* GPIO3 (out), GPIO3 (in), UART0_RXD, NA */
	{ 1,  PINMUX_FUNC_C }, /* GPIO4 (out), GPIO4 (in), UART0_TXD, NA */
	{ 2,  PINMUX_FUNC_C }, /* GPIO5 (out), GPIO5 (in), UART1_RXD, NA */
	{ 3,  PINMUX_FUNC_C }, /* GPIO6 (out), GPIO6 (in), UART1_TXD, PWM.LED1 */
	{ 4,  PINMUX_FUNC_B }, /* GPIO_SUS4 (out), GPIO_SUS4 (in), NA, NA */
	{ 5,  PINMUX_FUNC_B }, /* GPIO8 (out), GPIO8 (in), PWM.LED3, NA */
	{ 6,  PINMUX_FUNC_B }, /* GPIO9 (out), GPIO9 (in), PWM.LED5, NA */
	{ 7,  PINMUX_FUNC_A }, /* EXP1.P0_6 (out), EXP1.P0_6 (in), NA, NA */
	{ 8,  PINMUX_FUNC_A }, /* EXP1.P1_0 (out), EXP1.P1_0 (in), NA, NA */
	{ 9,  PINMUX_FUNC_B }, /* GPIO_SUS2 (out), GPIO_SUS2 (in), PWM.LED7, NA */
	{ 10, PINMUX_FUNC_B }, /* GPIO2 (out), GPIO2 (in), PWM.LED11, NA */
	{ 11, PINMUX_FUNC_B }, /* GPIO_SUS3 (out), GPIO_SUS3 (in), PWM.LED9, SPI1_MOSI */
	{ 12, PINMUX_FUNC_B }, /* GPIO7 (out), GPIO7 (in), SPI1_MISO, NA */
	{ 13, PINMUX_FUNC_B }, /* GPIO_SUS5 (out), GPIO_SUS5(in), SPI1_SCK, NA */
	{ 14, PINMUX_FUNC_B }, /* EXP2.P0_0 (out)/ADC.IN0, EXP2.P0_0 (in)/ADC.IN0, NA, NA */
	{ 15, PINMUX_FUNC_B }, /* EXP2.P0_2 (out)/ADC.IN1, EXP2.P0_2 (in)/ADC.IN1, NA, NA */
	{ 16, PINMUX_FUNC_B }, /* EXP2.P0_4 (out)/ADC.IN2, EXP2.P0_4 (in)/ADC.IN2, NA, NA */
	{ 17, PINMUX_FUNC_B }, /* EXP2.P0_6 (out)/ADC.IN3, EXP2.P0_6 (in)/ADC.IN3, NA, NA */
	{ 18, PINMUX_FUNC_C }, /* EXP2.P1_0 (out), ADC.IN4, I2C_SDA, NA */
	{ 19, PINMUX_FUNC_C }, /* EXP2.P1_2 (out), ADC.IN5, I2C_SCL, NA */
};

struct galileo_data galileo_pinmux_driver = {
	.exp0 = NULL,
	.exp1 = NULL,
	.exp2 = NULL,
	.pwm0 = NULL,
	.mux_config = mux_config,
};

static int pinmux_galileo_initialize(struct device *port)
{
	struct galileo_data *dev = port->driver_data;
	int i;

	/* Grab the EXP0, EXP1, EXP2, and PWM0 now by name */
	dev->exp0 = device_get_binding(CONFIG_PINMUX_GALILEO_EXP0_NAME);
	if (!dev->exp0) {
		return -EINVAL;
	}
	dev->exp1 = device_get_binding(CONFIG_PINMUX_GALILEO_EXP1_NAME);
	if (!dev->exp1) {
		return -EINVAL;
	}
	dev->exp2 = device_get_binding(CONFIG_PINMUX_GALILEO_EXP2_NAME);
	if (!dev->exp2) {
		return -EINVAL;
	}
	dev->pwm0 = device_get_binding(CONFIG_PINMUX_GALILEO_PWM0_NAME);
	if (!dev->pwm0) {
		return -EINVAL;
	}
	dev->gpio_dw = device_get_binding(CONFIG_PINMUX_GALILEO_GPIO_DW_NAME);
	if (!dev->gpio_dw) {
		return -EINVAL;
	}
	dev->gpio_core = device_get_binding(
			    CONFIG_PINMUX_GALILEO_GPIO_INTEL_CW_NAME);
	if (!dev->gpio_core) {
		return -EINVAL;
	}
	dev->gpio_resume = device_get_binding(
			    CONFIG_PINMUX_GALILEO_GPIO_INTEL_RW_NAME);
	if (!dev->gpio_resume) {
		return -EINVAL;
	}

	/*
	 * Now that we have everything, let us start parsing everything
	 * from the above mapping as selected by the end user
	 */
	for (i = 0; i < PINMUX_NUM_PINS; i++) {
		_galileo_pinmux_set_pin(port,
				 mux_config[i].pin_num,
				 mux_config[i].mode);
	}

	return 0;
}

/*
 * This needs to be a level 2 or later init process due to the following
 * dependency chain:
 * 0 - I2C
 * 1 - PCA9535 and PCAL9685
 * 2 - pinmux
 */
DEVICE_INIT(pmux, PINMUX_NAME, &pinmux_galileo_initialize,
			&galileo_pinmux_driver, NULL,
			SECONDARY, CONFIG_PINMUX_INIT_PRIORITY);
