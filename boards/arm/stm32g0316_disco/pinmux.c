/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <soc.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32G0316-DISCO board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_UART_1
	{STM32_PIN_PA9, STM32G0_PINMUX_FUNC_PA9_USART1_TX},
	{STM32_PIN_PB7, STM32G0_PINMUX_FUNC_PB7_USART1_RX},
#endif /* CONFIG_UART_1 */
};

static int pinmux_stm32_init(struct device *port)
{
	ARG_UNUSED(port);

#ifdef CONFIG_UART_1
	/* Remap PA11 to PA9 */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_EnablePinRemap(LL_SYSCFG_PIN_RMP_PA11);
#endif /* CONFIG_UART_1 */

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
