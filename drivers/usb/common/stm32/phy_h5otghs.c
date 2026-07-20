/*
 * Copyright (c) 2026 Arkadiusz Grzelka
 * Copyright (c) 2026 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STM32H5 OTG_HS embedded HS PHY driver
 *
 * Covers the embedded High-Speed PHY of STM32H5E5x/STM32H5F5x. The PHY IP
 * is the one already handled by phy_u5otghs.c, but on this family it is
 * controlled through RCC instead of SYSCFG: the reference-clock selector
 * is CCIPR4.OTGPHYREFCKSEL, the PHY gate is AHB2ENR.OTGPHYEN and the PHY
 * reset is AHB2RSTR.OTGHSPHYRST. There is no HAL_SYSCFG_*OTGPHY* API on
 * STM32H5, so phy_u5otghs.c cannot be reused as-is.
 */
#include <soc.h>
#include <stm32_ll_rcc.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

/* Even though we won't use this macro, define it for grep-ability */
#define DT_DRV_COMPAT st_stm32h5_otghs_phy

struct stm32_h5otghs_phy_config {
	struct reset_dt_spec reset;
	uint32_t reference;
	uint32_t num_clocks;
	struct stm32_pclken clocks[];
};

static const struct device *rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

static int stm32_h5otghs_phy_enable(const struct stm32_usb_phy *phy)
{
	const struct stm32_h5otghs_phy_config *cfg = phy->pcfg;
	int res;

	/* Tell the PHY PLL which reference frequency it is fed */
	LL_RCC_SetOTGPHYClockSource(cfg->reference);

	/* Deassert PHY reset */
	res = reset_line_deassert_dt(&cfg->reset);
	if (res != 0) {
		return res;
	}

	/* Configure PHY input mux (if provided) */
	if (cfg->num_clocks > 1) {
		res = clock_control_configure(rcc, (clock_control_subsys_t)&cfg->clocks[1], NULL);
		if (res != 0) {
			return res;
		}
	}

	/* Turn on the PHY's clock */
	return clock_control_on(rcc, (clock_control_subsys_t)&cfg->clocks[0]);
}

static int stm32_h5otghs_phy_disable(const struct stm32_usb_phy *phy)
{
	const struct stm32_h5otghs_phy_config *cfg = phy->pcfg;

	return clock_control_off(rcc, (clock_control_subsys_t)&cfg->clocks[0]);
}

/*
 * The `clock-reference` enum values are the suffixes of the matching
 * LL_RCC_OTGPHYREFCKCLKSOURCE_<> constants, e.g. "24M" -> _24M.
 */
#define PHY_CLK_REF(n)	CONCAT(LL_RCC_OTGPHYREFCKCLKSOURCE_, DT_STRING_TOKEN(n, clock_reference))

#define DEFINE_H5OTGHS_PHY(usb_node, phy_node)							\
	static const struct stm32_h5otghs_phy_config CONCAT(phy, DT_DEP_ORD(phy_node), _cfg) = {\
		.reset = RESET_DT_SPEC_GET(phy_node),						\
		.reference = PHY_CLK_REF(phy_node),						\
		.num_clocks = DT_NUM_CLOCKS(phy_node),						\
		.clocks = STM32_DT_CLOCKS(phy_node)						\
	};											\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {			\
		.enable = stm32_h5otghs_phy_enable,						\
		.disable = stm32_h5otghs_phy_disable,						\
		.pcfg = &CONCAT(phy, DT_DEP_ORD(phy_node), _cfg),				\
	};

/*
 * Iterate all USB nodes and instantiate PHY when appropriate.
 */
#define _FOREACH_NODE(usb_node)									\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node),					\
		(DEFINE_H5OTGHS_PHY(usb_node, USB_STM32_PHY(usb_node))))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
