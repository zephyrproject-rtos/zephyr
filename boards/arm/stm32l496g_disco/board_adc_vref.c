/*
 * Copyright (c) 2021 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <stm32_ll_adc.h>
#include <devicetree.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(board_control, CONFIG_BOARD_STM32L496G_DISCO_LOG_LEVEL);

static int enable_adc_reference(const struct device *dev)
{
	/* VREF+ is not connected to VDDA by default */
	/* Use 2.5V as reference (instead of 3.3V) for internal channels
	 * calculation
	 */
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/* VREF_OUT2 = 2.5 V */
	HAL_SYSCFG_VREFBUF_VoltageScalingConfig(SYSCFG_VREFBUF_VOLTAGE_SCALE1);
	HAL_SYSCFG_VREFBUF_HighImpedanceConfig(
					SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE);

	if (HAL_SYSCFG_EnableVREFBUF() != HAL_OK) {
		LOG_WRN("Could not enable HAL_SYSCFG_EnableVREFBUF\n");
		LOG_WRN("ADC Conversion value may be incorrect\n");
	}
	return 0;
}

SYS_INIT(enable_adc_reference, PRE_KERNEL_1, 0);
