/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Peripheral Clock control driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_peri

#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>

#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>

#include <cy_sysclk.h>
#include <cy_systick.h>

LOG_MODULE_REGISTER(clock_control_ifx_peri, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/** @brief Compile time description of a single peripheral clock divider. */
struct ifx_peri_divider {
	struct ifx_cat1_clock clock;
	uint16_t divider;
	uint8_t frac_divider;
	uint8_t div_type;
};

/** @brief All dividers belonging to one peripheral clock instance. */
struct ifx_peri_clock_config {
	const struct ifx_peri_divider *dividers;
	size_t num_dividers;
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

static int ifx_peri_divider_setup(const struct ifx_peri_divider *div_cfg)
{
	en_clk_dst_t clk_dst;

	/* PDL calls to set the and enable peri clock divider use the en_clk_dst_t
	 * enumeration. This enumeration contains the peripheral clock instance, peripheral
	 * clock group, and the peripheral connection.  We don't know what the peripheral
	 * connection is in the clock control driver, so we will use a value of 0.  The
	 * specific peripheral connection is not needed in the underlying pdl enable and
	 * clock configuration calls.
	 */
#if defined(COMPONENT_CAT1B) || defined(COMPONENT_CAT1C) || defined(CONFIG_SOC_FAMILY_INFINEON_EDGE)
	clk_dst = peri_pclk_build_en_clk_dst(0, div_cfg->clock.group, div_cfg->clock.instance);
#else
	/* For PSOC4, clk_dst is simply 0 since we don't have instance/group fields */
	clk_dst = 0;
#endif

	/* Note: This function sets up the divider and enables it.  Each peripheral that needs to
	 * use the clock must connect to the clock by calling:
	 * ifx_cat1_utils_peri_pclk_assign_divider()
	 */
	if ((div_cfg->div_type == CY_SYSCLK_DIV_8_BIT) ||
	    (div_cfg->div_type == CY_SYSCLK_DIV_16_BIT)) {
		if (CY_RSLT_SUCCESS != ifx_cat1_utils_peri_pclk_set_divider(
					       clk_dst, &div_cfg->clock, div_cfg->divider - 1)) {
			return -EIO;
		}
	} else {
		if (CY_RSLT_SUCCESS != ifx_cat1_utils_peri_pclk_set_frac_divider(
					       clk_dst, &div_cfg->clock, div_cfg->divider - 1,
					       div_cfg->frac_divider)) {
			return -EIO;
		}
	}

	if (CY_RSLT_SUCCESS != ifx_cat1_utils_peri_pclk_enable_divider(clk_dst, &div_cfg->clock)) {
		return -EIO;
	}

	return 0;
}

static int ifx_peri_clock_init(const struct device *dev)
{
	const struct ifx_peri_clock_config *const config = dev->config;

	for (size_t i = 0; i < config->num_dividers; i++) {
		int ret = ifx_peri_divider_setup(&config->dividers[i]);

		if (ret != 0) {
			LOG_ERR("%s: failed to configure divider %zu (%d)", dev->name, i, ret);
			return ret;
		}
	}

	return 0;
}

/* The struct ifx_cat1_clock initializer is shared with the peripheral drivers so that
 * the divider encoding only has to be described in one place.
 */
#define IFX_PERI_DIV_ENTRY(node_id)                                                                \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, infineon_peri_div),                                 \
		   ({                                                                              \
			    .clock = IFX_CAT1_PERI_CLOCK_DT_INIT(node_id),                         \
			    .div_type = DT_PROP(node_id, div_type),                                \
			    .divider = DT_PROP(node_id, clock_div),                                \
			    .frac_divider = DT_PROP_OR(node_id, div_frac_value, 0),                \
		    },))

/* A peripheral clock instance with no enabled dividers is legitimate, so a terminating
 * entry keeps the array from being empty. clock-div is always at least 1 for a real
 * divider, which makes a zero divider usable as a sentinel.
 */
#define IFX_PERI_DIV_END {.divider = 0}

#define IFX_PERI_CLOCK_INIT(n)                                                                     \
	static const struct ifx_peri_divider ifx_peri_dividers_##n[] = {                           \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IFX_PERI_DIV_ENTRY) IFX_PERI_DIV_END,         \
	};                                                                                         \
                                                                                                   \
	static const struct ifx_peri_clock_config ifx_peri_clock_config_##n = {                    \
		.dividers = ifx_peri_dividers_##n,                                                 \
		.num_dividers = ARRAY_SIZE(ifx_peri_dividers_##n) - 1,                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_peri_clock_init, NULL, NULL, &ifx_peri_clock_config_##n,     \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(IFX_PERI_CLOCK_INIT)

/* Dividers are only programmed when they are a child of an infineon,peri node. Catch any
 * out of tree devicetree that still places them elsewhere instead of silently leaving
 * them unconfigured.
 */
#define IFX_PERI_COUNT_DIV(node_id) 1 +
#define IFX_PERI_COUNT_CHILD_DIV(node_id)                                                          \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, infineon_peri_div), (1 +))
#define IFX_PERI_COUNT_INST_DIV(n) DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IFX_PERI_COUNT_CHILD_DIV)

BUILD_ASSERT((DT_INST_FOREACH_STATUS_OKAY(IFX_PERI_COUNT_INST_DIV) 0) ==
		     (DT_FOREACH_STATUS_OKAY(infineon_peri_div, IFX_PERI_COUNT_DIV) 0),
	     "Every enabled infineon,peri-div node must be a child of an infineon,peri node");
