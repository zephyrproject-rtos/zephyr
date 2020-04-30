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
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart1), okay) && CONFIG_SERIAL
	{STM32_PIN_PA9, STM32G0_PINMUX_FUNC_PA9_USART1_TX},
	{STM32_PIN_PB7, STM32G0_PINMUX_FUNC_PB7_USART1_RX},
#endif
};

static int pinmux_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart1), okay) && CONFIG_SERIAL
	/* Remap PA11 to PA9 */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	LL_SYSCFG_EnablePinRemap(LL_SYSCFG_PIN_RMP_PA11);
#endif

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
