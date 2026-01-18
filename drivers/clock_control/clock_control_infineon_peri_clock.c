/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Peripheral Clock control driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_peri_div

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#include <zephyr/dt-bindings/clock/ifx_clock_source_common.h>

#include <cy_sysclk.h>
#include <cy_systick.h>

struct ifx_peri_clock_data {
	struct ifx_cat1_resource_inst hw_resource;
	struct ifx_cat1_clock clock;
	uint16_t divider;
	CySCB_Type *reg_addr;
};

#if defined(CY_IP_MXPERI) || defined(CY_IP_M0S8PERI)

#define IFX_PCLK_GROUP(clkdst) 0
#if (CY_IP_MXTCPWM_INSTANCES > 1) || (CY_IP_M0S8TCPWM_INSTANCES > 1)
#define IFX_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM0_CLOCKS0
#define IFX_TCPWM1_PCLK_CLOCK0 PCLK_TCPWM1_CLOCKS0
#else
#define IFX_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM_CLOCKS0
#endif
#define IFX_SCB0_PCLK_CLOCK PCLK_SCB0_CLOCK

#elif defined(CY_IP_MXSPERI)

#define IFX_PCLK_GROUP(clkdst) ((uint8_t)((uint32_t)(clkdst) >> 8))
#define IFX_TCPWM0_PCLK_CLOCK0 PCLK_TCPWM0_CLOCK_COUNTER_EN0
#define IFX_TCPWM1_PCLK_CLOCK0 PCLK_TCPWM0_CLOCK_COUNTER_EN256
#define IFX_SCB0_PCLK_CLOCK    PCLK_SCB0_CLOCK_SCB_EN
#define IFX_SCB1_PCLK_CLOCK    PCLK_SCB1_CLOCK_SCB_EN
#define IFX_SCB5_PCLK_CLOCK    PCLK_SCB5_CLOCK_SCB_EN
#endif

#if defined(CY_IP_MXSDHC)
#define IFX_SDHC0_PCLK_CLOCK PCLK_SDHC0_CLK_HF
#define IFX_SDHC1_PCLK_CLOCK PCLK_SDHC1_CLK_HF
#endif

#define CLK_FRAC_DIV_MODE 0x02

en_clk_dst_t ifx_cat1_scb_get_clock_index(uint32_t block_num)
{
	en_clk_dst_t clk;
/* PSOC6A256K does not have SCB 3 */
#if defined(CY_DEVICE_PSOC6A256K)
	if (block_num < 3) {
		clk = (en_clk_dst_t)((uint32_t)IFX_SCB0_PCLK_CLOCK + block_num);
	} else {
		clk = (en_clk_dst_t)((uint32_t)IFX_SCB0_PCLK_CLOCK + block_num - 1);
	}
#elif defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	if (block_num == 0) {
		clk = (en_clk_dst_t)((uint32_t)IFX_SCB0_PCLK_CLOCK);
	} else if (block_num == 1) {
		clk = (en_clk_dst_t)((uint32_t)IFX_SCB1_PCLK_CLOCK);
	} else {
		clk = (en_clk_dst_t)((uint32_t)IFX_SCB0_PCLK_CLOCK + block_num - 1);
	}
#else
	clk = (en_clk_dst_t)((uint32_t)IFX_SCB0_PCLK_CLOCK + block_num);
#endif

	return clk;
}

en_clk_dst_t ifx_cat1_tcpwm_get_clock_index(uint32_t block_num, uint32_t channel)
{
	en_clk_dst_t clk = -EINVAL;

	if (block_num == 0) {
		/* block_num 0 is 32-bit tcpwm instances */
		clk = (en_clk_dst_t)((uint32_t)IFX_TCPWM0_PCLK_CLOCK0 + channel);
#if (CY_IP_MXTCPWM_INSTANCES > 1) || (CY_IP_M0S8TCPWM_INSTANCES > 1) || (CY_IP_MXSPERI)
	} else if (block_num == 1) {
		/* block_num 1 is 16-bit tcwpm instances */
		clk = (en_clk_dst_t)((uint32_t)IFX_TCPWM1_PCLK_CLOCK0 + channel);
#endif
	} else {
		/* Current support does not account for block_num other than 0 or 1 */
		__ASSERT(block_num == 0 || block_num == 1,
			 "Invalid block_num used for tcpwm clock index.");
#if (CY_IP_MXTCPWM_INSTANCES == 1) || (CY_IP_M0S8TCPWM_INSTANCES == 1) || (CY_IP_MXSPERI)
		__ASSERT(block_num == 0, "Invalid block_num used for tcpwm clock index.");
#endif
	}

	return clk;
}

en_clk_dst_t ifx_cat1_sdhc_get_clock_index(uint32_t block_num, uint32_t channel)
{
	en_clk_dst_t clk = -EINVAL;

#if defined(CY_IP_MXSDHC)
	if (block_num == 0) {
		clk = (en_clk_dst_t)((uint32_t)IFX_SDHC0_PCLK_CLOCK);
	} else {
		clk = (en_clk_dst_t)((uint32_t)IFX_SDHC1_PCLK_CLOCK);
	}
#endif

	return clk;
}

static int ifx_cat1_peri_clock_init(const struct device *dev)
{
	struct ifx_peri_clock_data *const data = dev->data;
	int err;

	if (data->hw_resource.type == IFX_RSC_SCB) {
		en_clk_dst_t clk_idx = ifx_cat1_scb_get_clock_index(data->hw_resource.block_num);

		if ((data->clock.block & CLK_FRAC_DIV_MODE) == 0) {
			err = ifx_cat1_utils_peri_pclk_set_divider(clk_idx, &data->clock,
								   data->divider - 1);
		} else {
			err = ifx_cat1_utils_peri_pclk_set_frac_divider(clk_idx, &(data->clock),
									data->divider - 1, 0);
		}

		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}

		err = ifx_cat1_utils_peri_pclk_assign_divider(clk_idx, &data->clock);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}

		err = ifx_cat1_utils_peri_pclk_enable_divider(clk_idx, &data->clock);
		if (err != CY_SYSCLK_SUCCESS) {
			return -EIO;
		}
	} else if (data->hw_resource.type == IFX_RSC_TCPWM) {
		en_clk_dst_t clk_idx = ifx_cat1_tcpwm_get_clock_index(
			data->hw_resource.block_num, data->hw_resource.channel_num);

		ifx_cat1_utils_peri_pclk_set_divider(clk_idx, &data->clock, data->divider - 1);
		ifx_cat1_utils_peri_pclk_assign_divider(clk_idx, &data->clock);
		ifx_cat1_utils_peri_pclk_enable_divider(clk_idx, &data->clock);
	} else if (data->hw_resource.type == IFX_RSC_SDHC) {
		en_clk_dst_t clk_idx = ifx_cat1_sdhc_get_clock_index(data->hw_resource.block_num,
								     data->hw_resource.channel_num);

		ifx_cat1_utils_peri_pclk_set_divider(clk_idx, &data->clock, data->divider - 1);
		ifx_cat1_utils_peri_pclk_assign_divider(clk_idx, &data->clock);
		ifx_cat1_utils_peri_pclk_enable_divider(clk_idx, &data->clock);
	} else {
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
#define PERI_CLOCK_INIT(n)                                                                         \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_INST_PROP_BY_IDX(n, peri_group, 0),   \
							  DT_INST_PROP_BY_IDX(n, peri_group, 1),   \
							  DT_INST_PROP(n, div_type)),              \
		.channel = DT_INST_PROP(n, channel),                                               \
	},
#else
#define PERI_CLOCK_INIT(n)                                                                         \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_INST_PROP_BY_IDX(n, peri_group, 1),   \
							  DT_INST_PROP(n, div_type)),              \
		.channel = DT_INST_PROP(n, channel),                                               \
	},
#endif

#define INFINEON_CAT1_PERI_CLOCK_INIT(n)                                                           \
	static struct ifx_peri_clock_data ifx_cat1_peri_clock##n##_data = {                        \
		.divider = DT_INST_PROP(n, clock_div),                                             \
		.hw_resource = {.type = DT_INST_PROP(n, resource_type),                            \
				.block_num = DT_INST_PROP(n, resource_instance),                   \
				.channel_num = DT_INST_PROP_OR(n, resource_channel, 0)},           \
		PERI_CLOCK_INIT(n)};                                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_peri_clock_init, NULL, &ifx_cat1_peri_clock##n##_data,  \
			      NULL, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_PERI_CLOCK_INIT)
