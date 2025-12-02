/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "am_mcu_apollo.h"
#include "shield.h"

/* shield init */
static int ap4p_ap4_evb_disp_shield_init(void)
{
	am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI1_MUX_SEL, am_hal_gpio_pincfg_output);
	am_hal_gpio_pinconfig(AM_BSP_GPIO_MSPI1_MUX_OE, am_hal_gpio_pincfg_output);

#if defined(CONFIG_MEMC_MSPI_APS_Z8) && \
	(DT_ENUM_IDX(DT_ALIAS(psram0), mspi_io_mode) == 11)
	/* MSPI mux select and output enable for MSPI1 to be used with aps256n when in HEX mode */
	am_hal_gpio_state_write(AM_BSP_GPIO_MSPI1_MUX_SEL, AM_HAL_GPIO_OUTPUT_SET);
	am_hal_gpio_state_write(AM_BSP_GPIO_MSPI1_MUX_OE, AM_HAL_GPIO_OUTPUT_CLEAR);
#else
	am_hal_gpio_state_write(AM_BSP_GPIO_MSPI1_MUX_SEL, AM_HAL_GPIO_OUTPUT_CLEAR);
	am_hal_gpio_state_write(AM_BSP_GPIO_MSPI1_MUX_OE, AM_HAL_GPIO_OUTPUT_CLEAR);
#endif

	am_hal_gpio_pinconfig(AM_BSP_GPIO_DISP_DEVICE_EN, am_hal_gpio_pincfg_output);
	am_hal_gpio_state_write(AM_BSP_GPIO_DISP_DEVICE_EN, AM_HAL_GPIO_OUTPUT_SET);

	return 0;
}

/* needs to be done after GPIO driver init and device tree available but prior to MSPI peripherals
 * on shield
 */
SYS_INIT(ap4p_ap4_evb_disp_shield_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
