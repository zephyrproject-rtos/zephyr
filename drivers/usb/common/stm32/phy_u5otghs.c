/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * STM32 U5OTGHS embedded HS PHY driver
 *
 * Covers the unnamed PHY first found in STM32U5 series,
 * and later used in other series such as STM32WBA, ...
 *
 * Note: the HAL SYSCFG used is provided by stm32XXX_hal.c
 * which is always included in the build, and the __HAL_RCC
 * function is actually an in-header/LL-like macro.
 */
#include <soc.h>
#include <stm32_ll_system.h>

#include <stm32_bitops.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

/* Even though we won't use this macro, define it for grep-ability */
#define DT_DRV_COMPAT st_stm32u5_otghs_phy

/*
 * Detect appropriate CLKSEL value automatically based on input clock frequency.
 * This value must differ from all SYSCFG_OTG_HS_PHY_CLK_SELECT_<n> values!
 */
#define CLKSEL_AUTODETECT	0u

/*
 * Does any PHY node need CLKSEL autodetection?
 * True if any node lacks the `clock-reference` property.
 */
#define ANY_U5OTGHS_PHY_NEEDS_AUTODETECT \
	!DT_ALL_COMPAT_HAS_PROP_STATUS_OKAY(DT_DRV_COMPAT, clock_reference)

struct stm32_u5otghs_phy_config {
	uint32_t phycr_clksel;
	uint32_t num_clocks;
	struct stm32_pclken clocks[];
};

static const struct device *rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

static int stm32_u5otghs_phy_enable(const struct stm32_usb_phy *phy)
{
	const struct stm32_u5otghs_phy_config *cfg = phy->pcfg;
	uint32_t clksel;
	int res;

	/* Enable SYSCFG where PHY configuration registers reside */
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/* Configure PHY input frequency selection  */
	if (ANY_U5OTGHS_PHY_NEEDS_AUTODETECT && cfg->phycr_clksel == CLKSEL_AUTODETECT) {
		clock_control_subsys_t ck_src;
		uint32_t phy_ck_freq;

		if (cfg->num_clocks > 1) {
			ck_src = (clock_control_subsys_t)&cfg->clocks[1];
		} else {
			ck_src = (clock_control_subsys_t)&cfg->clocks[0];
		}

		res = clock_control_get_rate(rcc, ck_src, &phy_ck_freq);
		if (res != 0) {
			return res;
		}

		switch (phy_ck_freq) {
		case MHZ(16):
			clksel = SYSCFG_OTG_HS_PHY_CLK_SELECT_1;
			break;
		case KHZ(19200):
			clksel = SYSCFG_OTG_HS_PHY_CLK_SELECT_2;
			break;
		case MHZ(20):
			clksel = SYSCFG_OTG_HS_PHY_CLK_SELECT_3;
			break;
		case MHZ(24):
			clksel = SYSCFG_OTG_HS_PHY_CLK_SELECT_4;
			break;
		case MHZ(26):
			clksel = SYSCFG_OTG_HS_PHY_CLK_SELECT_5;
			break;
		case MHZ(32):
			clksel = SYSCFG_OTG_HS_PHY_CLK_SELECT_6;
			break;
		default:
			return -EINVAL;
		}
	} else {
		clksel = cfg->phycr_clksel;
	}
	HAL_SYSCFG_SetOTGPHYReferenceClockSelection(clksel);

	/* Deassert PHY reset */
	HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

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

static int stm32_u5otghs_phy_disable(const struct stm32_usb_phy *phy)
{
	const struct stm32_u5otghs_phy_config *cfg = phy->pcfg;

	return clock_control_off(rcc, (clock_control_subsys_t)&cfg->clocks[0]);
}

/*
 * SYSCFG_OTG_HS_PHY_CLK_SELECT_<> values go from 1 to N,
 * but DT_ENUM_IDX() is zero-based, hence the UTIL_INC().
 */
#define PHY_CLK_REF(n)	\
	COND_CODE_0(DT_NODE_HAS_PROP(n, clock_reference),					\
		(CLKSEL_AUTODETECT),								\
		(CONCAT(SYSCFG_OTG_HS_PHY_CLK_SELECT_, UTIL_INC(DT_ENUM_IDX(n, clock_reference)))))

#define DEFINE_U5OTGHS_PHY(usb_node, phy_node)							\
	static const struct stm32_u5otghs_phy_config CONCAT(phy, DT_DEP_ORD(phy_node), _cfg) = {\
		.phycr_clksel = PHY_CLK_REF(phy_node),						\
		.num_clocks = DT_NUM_CLOCKS(phy_node),						\
		.clocks = STM32_DT_CLOCKS(phy_node)						\
	};											\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {			\
		.enable = stm32_u5otghs_phy_enable,						\
		.disable = stm32_u5otghs_phy_disable,						\
		.pcfg = &CONCAT(phy, DT_DEP_ORD(phy_node), _cfg),				\
	};

/*
 * Iterate all USB nodes and instantiate PHY when appropriate.
 * We could implement a stricter check but this should suffice;
 * there are no series with different type of embedded HS PHYs.
 */
#define _FOREACH_NODE(usb_node)									\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node),					\
		(DEFINE_U5OTGHS_PHY(usb_node, USB_STM32_PHY(usb_node))))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
