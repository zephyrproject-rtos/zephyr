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

#include <cy_sysclk.h>
#include <cy_systick.h>

struct ifx_peri_clock_data {
	struct ifx_cat1_resource_inst hw_resource;
	struct ifx_cat1_clock clock;
	uint16_t divider;
	uint8_t frac_divider;
	uint8_t div_type;
	CySCB_Type *reg_addr;
};

static inline en_clk_dst_t peri_pclk_build_en_clk_dst(uint8_t output, uint8_t group,
						      uint8_t instance)
{
	en_clk_dst_t clk_dst;

	clk_dst = output;
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	/* These devices pack instance, group, and output together in the en_clk_dst_t.  Group and
	 * Instance are used by the enable_divider and set_divider functions to determine which
	 * clock is being referenced.
	 */
	clk_dst |= ((uint32_t)group << PERI_PCLK_GR_NUM_Pos);
	clk_dst |= ((uint32_t)instance << PERI_PCLK_INST_NUM_Pos);
#endif
	return clk_dst;
}

static int ifx_cat1_peri_clock_init(const struct device *dev)
{
	struct ifx_peri_clock_data *const data = dev->data;
	en_clk_dst_t clk_dst;

	/* PDL calls to set the and enable peri clock divider use the en_clk_dst_t
	 * enumeration. This enumeration contains the peripheral clock instance, peripheral
	 * clock group, and the peripheral connection.  We don't know what the peripheral
	 * connection is in the clock control driver, so we will use a value of 0.  The
	 * specific peripheral connection is not needed in the underlying pdl enable and
	 * clock configuration calls.
	 */
	clk_dst = peri_pclk_build_en_clk_dst(0, data->clock.group, data->clock.instance);

	/* Note: This function sets up the divider and enables it.  Each peripheral that needs to
	 * use the clock must connect to the clock by calling:
	 * ifx_cat1_utils_peri_pclk_assign_divider()
	 */
	if ((data->div_type == CY_SYSCLK_DIV_8_BIT) || (data->div_type == CY_SYSCLK_DIV_16_BIT)) {
		if (CY_RSLT_SUCCESS != ifx_cat1_utils_peri_pclk_set_divider(clk_dst,
						&data->clock, data->divider - 1)) {
			return -EIO;
		}
	} else {
		if (CY_RSLT_SUCCESS != ifx_cat1_utils_peri_pclk_set_frac_divider(clk_dst,
				&data->clock, data->divider - 1, data->frac_divider)) {
			return -EIO;
		}
	}

	if (CY_RSLT_SUCCESS != ifx_cat1_utils_peri_pclk_enable_divider(clk_dst, &data->clock)) {
		return -EIO;
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
		.instance = DT_INST_PROP_BY_IDX(n, peri_group, 0),                                 \
		.group = DT_INST_PROP_BY_IDX(n, peri_group, 1),                                    \
	},
#else
#define PERI_CLOCK_INIT(n)                                                                         \
	.clock = {                                                                                 \
		.block = IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_INST_PROP_BY_IDX(n, peri_group, 1),   \
							  DT_INST_PROP(n, div_type)),              \
		.channel = DT_INST_PROP(n, channel),                                               \
		.instance = DT_INST_PROP_BY_IDX(n, peri_group, 0),                                 \
		.group = DT_INST_PROP_BY_IDX(n, peri_group, 1),                                    \
	},
#endif

#define INFINEON_CAT1_PERI_CLOCK_INIT(n)                                                           \
	static struct ifx_peri_clock_data ifx_cat1_peri_clock##n##_data = {                        \
		.div_type = DT_INST_PROP(n, div_type),                                             \
		.divider = DT_INST_PROP(n, clock_div),                                             \
		.frac_divider = DT_INST_PROP_OR(n, div_frac_value, 0),                             \
		.hw_resource = {.type = DT_INST_PROP(n, resource_type),                            \
				.block_num = DT_INST_PROP(n, resource_instance),                   \
				.channel_num = DT_INST_PROP_OR(n, resource_channel, 0)},           \
		PERI_CLOCK_INIT(n)};                                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_peri_clock_init, NULL, &ifx_cat1_peri_clock##n##_data,  \
			      NULL, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_PERI_CLOCK_INIT)
