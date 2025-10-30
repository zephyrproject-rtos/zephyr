/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Peripheral Clock control driver for Infineon CAT2 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat2_peri_div

#include <zephyr/drivers/clock_control/clock_control_ifx_cat2.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <cy_sysclk.h>
#include <cy_systick.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ifx_cat2_peri_clock);

struct ifx_cat2_peri_clock_data {
	struct ifx_cat2_resource_inst hw_resource;
	struct ifx_cat2_clock clock;
	uint16_t divider;
	CySCB_Type *reg_addr;
};

#if defined(CY_IP_MXPERI) || defined(CY_IP_M0S8PERI)

#define _IFX_CAT2_PCLK_GROUP(clkdst) 0
#define _IFX_CAT2_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM0_CLOCKS0
#define _IFX_CAT2_TCPWM1_PCLK_CLOCK0 PCLK_TCPWM1_CLOCKS0
#define _IFX_CAT2_SCB0_PCLK_CLOCK    PCLK_SCB0_CLOCK

#elif defined(CY_IP_MXSPERI)

#define _IFX_CAT2_PCLK_GROUP(clkdst) ((uint8_t)((uint32_t)(clkdst) >> 8))
#define _IFX_CAT2_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM0_CLOCK_COUNTER_EN0
#define _IFX_CAT2_TCPWM1_PCLK_CLOCK0 PCLK_TCPWM1_CLOCK_COUNTER_EN0
#define _IFX_CAT2_SCB0_PCLK_CLOCK    PCLK_SCB0_CLOCK_SCB_EN
#define _IFX_CAT2_SCB1_PCLK_CLOCK    PCLK_SCB1_CLOCK_SCB_EN
#define _IFX_CAT2_SCB5_PCLK_CLOCK    PCLK_SCB5_CLOCK_SCB_EN
#endif

en_clk_dst_t ifx_cat2_scb_get_clock_index(uint32_t block_num)
{
	en_clk_dst_t clk;

	clk = (en_clk_dst_t)((uint32_t)_IFX_CAT2_SCB0_PCLK_CLOCK + block_num);

	return clk;
}

static int ifx_cat2_peri_clock_init(const struct device *dev)
{
	int ret = CY_SYSCLK_SUCCESS;
	struct ifx_cat2_peri_clock_data *const data = dev->data;

	en_clk_dst_t clk_idx = ifx_cat2_scb_get_clock_index(data->hw_resource.block_num);

	cy_en_divider_types_t div_type =
		IFX_CAT2_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(data->clock.block);

	if (div_type == CY_SYSCLK_DIV_16_5_BIT || div_type == CY_SYSCLK_DIV_24_5_BIT) {
		ret = ifx_cat2_utils_peri_pclk_disable_divider(clk_idx, &data->clock);
		if (ret != CY_SYSCLK_SUCCESS) {
			LOG_ERR("Failed to disable peripheral clock divider %d\n", ret);
			return -EIO;
		}

		ret = ifx_cat2_utils_peri_pclk_set_frac_divider(clk_idx, &data->clock,
								data->divider - 1, 0);
		if (ret != CY_SYSCLK_SUCCESS) {
			LOG_ERR("Failed to set the fraction divider %d\n", ret);
			return -EIO;
		}

	} else {
		ret = ifx_cat2_utils_peri_pclk_disable_divider(clk_idx, &data->clock);
		if (ret != CY_SYSCLK_SUCCESS) {
			LOG_ERR("Failed to disable peripheral clock divider %d\n", ret);
			return -EIO;
		}

		ret = ifx_cat2_utils_peri_pclk_set_divider(clk_idx, &data->clock,
							   data->divider - 1);
		if (ret != CY_SYSCLK_SUCCESS) {
			LOG_ERR("Failed to set pclk divider %d\n", ret);
			return -EIO;
		}
	}

	ret = ifx_cat2_utils_peri_pclk_enable_divider(clk_idx, &data->clock);
	if (ret != CY_SYSCLK_SUCCESS) {
		LOG_ERR("Failed to enable pclk divider %d\n", ret);
		return -EIO;
	}

	ret = ifx_cat2_utils_peri_pclk_assign_divider(clk_idx, &data->clock);
	if (ret != CY_SYSCLK_SUCCESS) {
		LOG_ERR("Failed to assign pclk divider %d\n", ret);
		return -EIO;
	}

	return 0;
}

#define INFINEON_CAT2_PERI_CLOCK_INIT(n)                                                           \
	static struct ifx_cat2_peri_clock_data ifx_cat2_peri_clock##n##_data = {                   \
		.clock =                                                                           \
			{                                                                          \
				.block = IFX_CAT2_PERIPHERAL_GROUP_ADJUST(                         \
					DT_INST_PROP_BY_IDX(n, clk_dst, 1),                        \
					DT_INST_PROP(n, div_type)),                                \
				.channel = DT_INST_PROP(n, div_num),                               \
			},                                                                         \
		.divider = DT_INST_PROP(n, div_value),                                             \
		.hw_resource = {.type = IFX_CAT2_RSC_SCB,                                          \
				.block_num = DT_INST_PROP(n, scb_block)},                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat2_peri_clock_init, NULL, &ifx_cat2_peri_clock##n##_data,  \
			      NULL, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT2_PERI_CLOCK_INIT)
