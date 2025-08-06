/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Peripheral Clock control driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_peri_div

#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <cy_sysclk.h>
#include <cy_systick.h>

struct ifx_cat1_peri_clock_data {
	struct ifx_cat1_resource_inst hw_resource;
	struct ifx_cat1_clock clock;
	uint16_t divider;
	CySCB_Type *reg_addr;
};

#if defined(CY_IP_MXPERI) || defined(CY_IP_M0S8PERI)

#define _IFX_CAT1_PCLK_GROUP(clkdst) 0
#define _IFX_CAT1_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM0_CLOCKS0
#define _IFX_CAT1_TCPWM1_PCLK_CLOCK0 PCLK_TCPWM1_CLOCKS0
#define _IFX_CAT1_SCB0_PCLK_CLOCK    PCLK_SCB0_CLOCK

#elif defined(CY_IP_MXSPERI)

#define _IFX_CAT1_PCLK_GROUP(clkdst) ((uint8_t)((uint32_t)(clkdst) >> 8))
#define _IFX_CAT1_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM0_CLOCK_COUNTER_EN0
#define _IFX_CAT1_TCPWM1_PCLK_CLOCK0 PCLK_TCPWM1_CLOCK_COUNTER_EN0
#define _IFX_CAT1_SCB0_PCLK_CLOCK    PCLK_SCB0_CLOCK_SCB_EN
#define _IFX_CAT1_SCB1_PCLK_CLOCK    PCLK_SCB1_CLOCK_SCB_EN
#define _IFX_CAT1_SCB5_PCLK_CLOCK    PCLK_SCB5_CLOCK_SCB_EN
#endif

en_clk_dst_t ifx_cat1_scb_get_clock_index(uint32_t block_num)
{
	en_clk_dst_t clk;
/* PSOC6A256K does not have SCB 3 */
#if defined(CY_DEVICE_PSOC6A256K)
	if (block_num < 3) {
		clk = (en_clk_dst_t)((uint32_t)_IFX_CAT1_SCB0_PCLK_CLOCK + block_num);
	} else {
		clk = (en_clk_dst_t)((uint32_t)_IFX_CAT1_SCB0_PCLK_CLOCK + block_num - 1);
	}
#else
	clk = (en_clk_dst_t)((uint32_t)_IFX_CAT1_SCB0_PCLK_CLOCK + block_num);
#endif
	return clk;
}

static int ifx_cat1_peri_clock_init(const struct device *dev)
{
	struct ifx_cat1_peri_clock_data *const data = dev->data;

	en_clk_dst_t clk_idx = ifx_cat1_scb_get_clock_index(data->hw_resource.block_num);

	ifx_cat1_utils_peri_pclk_set_divider(clk_idx, &data->clock, data->divider - 1);
	ifx_cat1_utils_peri_pclk_assign_divider(clk_idx, &data->clock);
	ifx_cat1_utils_peri_pclk_enable_divider(clk_idx, &data->clock);

	return 0;
}

#define INFINEON_CAT1_PERI_CLOCK_INIT(n)                                                           \
	static struct ifx_cat1_peri_clock_data ifx_cat1_peri_clock##n##_data = {                   \
		.clock =                                                                           \
			{                                                                          \
				.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(                         \
					DT_INST_PROP_BY_IDX(n, clk_dst, 1),                        \
					DT_INST_PROP(n, div_type)),                                \
				.channel = DT_INST_PROP(n, div_num),                               \
			},                                                                         \
		.divider = DT_INST_PROP(n, div_value),                                             \
		.hw_resource = {.type = IFX_CAT1_RSC_SCB,                                          \
				.block_num = DT_INST_PROP(n, scb_block)},                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_peri_clock_init, NULL, &ifx_cat1_peri_clock##n##_data,  \
			      NULL, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_PERI_CLOCK_INIT)
