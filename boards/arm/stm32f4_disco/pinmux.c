/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32F4DISCOVERY board */
static const struct pin_config pinconf[] = {
#if DT_HAS_NODE(DT_NODELABEL(usart1))
	{STM32_PIN_PB6, STM32F4_PINMUX_FUNC_PB6_USART1_TX},
	{STM32_PIN_PB7, STM32F4_PINMUX_FUNC_PB7_USART1_RX},
#endif
#if DT_HAS_NODE(DT_NODELABEL(usart2))
	{STM32_PIN_PA2, STM32F4_PINMUX_FUNC_PA2_USART2_TX},
	{STM32_PIN_PA3, STM32F4_PINMUX_FUNC_PA3_USART2_RX},
#endif
#ifdef CONFIG_PWM_STM32_2
	{STM32_PIN_PA0, STM32F4_PINMUX_FUNC_PA0_PWM2_CH1},
#endif /* CONFIG_PWM_STM32_2 */
#ifdef CONFIG_USB_DC_STM32
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif	/* CONFIG_USB_DC_STM32 */
#ifdef CONFIG_CAN_1
	{STM32_PIN_PB8, STM32F4_PINMUX_FUNC_PB8_CAN1_RX},
	{STM32_PIN_PB9, STM32F4_PINMUX_FUNC_PB9_CAN1_TX},
#endif	/* CONFIG_CAN_1 */
#ifdef CONFIG_CAN_2
	{STM32_PIN_PB5, STM32F4_PINMUX_FUNC_PB5_CAN2_RX},
	{STM32_PIN_PB13, STM32F4_PINMUX_FUNC_PB13_CAN2_TX},
#endif	/* CONFIG_CAN_2 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
