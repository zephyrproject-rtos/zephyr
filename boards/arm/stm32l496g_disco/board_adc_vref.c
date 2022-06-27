/*
 * Copyright (c) 2021 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/init.h>
#include <stm32_ll_adc.h>
#include <zephyr/devicetree.h>

static int enable_adc_reference(const struct device *dev)
{
	uint8_t init_status;
	/* VREF+ is not connected to VDDA by default */
	/* Use 2.5V as reference (instead of 3.3V) for internal channels
	 * calculation
	 */
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/* VREF_OUT2 = 2.5 V */
	HAL_SYSCFG_VREFBUF_VoltageScalingConfig(SYSCFG_VREFBUF_VOLTAGE_SCALE1);
	HAL_SYSCFG_VREFBUF_HighImpedanceConfig(
					SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE);

	init_status = HAL_SYSCFG_EnableVREFBUF();
	__ASSERT(init_status == HAL_OK,	"ADC Conversion value may be incorrect");

	return init_status;
}

SYS_INIT(enable_adc_reference, POST_KERNEL, 0);
