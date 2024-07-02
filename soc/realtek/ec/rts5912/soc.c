/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com> / Titan Chen
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include "reg/reg_system.h"
#include "reg/reg_gpio.h"

#ifdef CONFIG_ENABLE_JTAG_PIN
static void configure_jtag_interface(void)
{
	static volatile GPIO_Type *pinctrl_base = (volatile GPIO_Type *)(DT_REG_ADDR(DT_NODELABEL(pinctrl)));
	
	// set pin 87 to TDI
	pinctrl_base->GCR[87] = ((0x1<<GPIO_GCR_SCHEN_Pos) | (0x1<<GPIO_GCR_MFCTRL_Pos) | (0x1<<GPIO_GCR_INDETEN_Pos));
	// set pin 88 to TDO
	pinctrl_base->GCR[88] = ((0x1<<GPIO_GCR_SCHEN_Pos) | (0x3<<GPIO_GCR_MFCTRL_Pos) | (0x1<<GPIO_GCR_INDETEN_Pos));
	// set pin 89 to RST
	pinctrl_base->GCR[89] = ((0x1<<GPIO_GCR_SCHEN_Pos) | (0x1<<GPIO_GCR_MFCTRL_Pos) | (0x1<<GPIO_GCR_INDETEN_Pos));
	// set pin 90 to CLK
	pinctrl_base->GCR[90] = ((0x1<<GPIO_GCR_SCHEN_Pos) | (0x2<<GPIO_GCR_MFCTRL_Pos) | (0x1<<GPIO_GCR_INDETEN_Pos));
	// set pin 91 to TMS
	pinctrl_base->GCR[91] = ((0x1<<GPIO_GCR_SCHEN_Pos) | (0x2<<GPIO_GCR_MFCTRL_Pos) | (0x1<<GPIO_GCR_INDETEN_Pos));
}
#endif

#if defined(CONFIG_RTS5912_ON_ENTER_CPU_IDLE_HOOK)
bool z_arm_on_enter_cpu_idle(void)
{
	/* Returning false prevent device goes to sleep mode */
	return false;
}
#endif

void z_arm_platform_init(void)
{

#ifdef CONFIG_ENABLE_JTAG_PIN	
	configure_jtag_interface();
#endif

}
