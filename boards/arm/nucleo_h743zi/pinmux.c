/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* NUCLEO-H743ZI pin configurations */
static const struct pin_config pinconf[] = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart3), okay) && CONFIG_SERIAL
	{ STM32_PIN_PD8, STM32H7_PINMUX_FUNC_PD8_USART3_TX },
	{ STM32_PIN_PD9, STM32H7_PINMUX_FUNC_PD9_USART3_RX },
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	{ STM32_PIN_PB8, STM32H7_PINMUX_FUNC_PB8_I2C1_SCL },
	{ STM32_PIN_PB9, STM32H7_PINMUX_FUNC_PB9_I2C1_SDA },
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm12), okay) && CONFIG_PWM
	{ STM32_PIN_PB14, STM32H7_PINMUX_FUNC_PB14_PWM12_CH1 },
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc1_2), okay) && CONFIG_ADC
	{ STM32_PIN_PA3, STM32H7_PINMUX_FUNC_PA3_ADC12_INP15 },
#endif
};

static int pinmux_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
