/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pinmux.h>
#include <sys/sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for OLIMEX-STM32-E407 board */
static const struct pin_config pinconf[] = {
#ifdef CONFIG_USB_DC_STM32
#if DT_NODE_HAS_STATUS(DT_INST(0, st_stm32_otgfs), okay)
	{STM32_PIN_PA11, STM32F4_PINMUX_FUNC_PA11_OTG_FS_DM},
	{STM32_PIN_PA12, STM32F4_PINMUX_FUNC_PA12_OTG_FS_DP},
#endif /* DT_NODE_HAS_STATUS(DT_INST(0, st_stm32_otgfs), okay) */
#if DT_NODE_HAS_STATUS(DT_INST(0, st_stm32_otghs), okay)
	{STM32_PIN_PB14, STM32F4_PINMUX_FUNC_PB14_OTG_HS_DM},
	{STM32_PIN_PB15, STM32F4_PINMUX_FUNC_PB15_OTG_HS_DP},
#endif /* DT_NODE_HAS_STATUS(DT_INST(0, st_stm32_otghs), okay) */
#endif  /* CONFIG_USB_DC_STM32 */
};

static int pinmux_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
		CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
