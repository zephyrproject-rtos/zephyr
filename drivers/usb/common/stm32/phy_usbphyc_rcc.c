/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Generic driver for RCC-configured USBPHYC */
#define DT_DRV_COMPAT st_stm32_usbphyc /* Unused; defined for grep-ability */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

struct stm32_rcc_usbphyc_config {
	/* PHY frequency selection configuration */
	struct stm32_pclken fsel;

	/* PHY clock mux configuration (if applicable) */
	struct stm32_pclken muxcfg;

	/* PHY clock gate configuration (if applicable) */
	struct stm32_pclken gatecfg;

	bool has_muxcfg : 1;
	bool has_gatecfg : 1;
};

static const struct device *rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

static int stm32_rcc_usbphyc_enable(const struct stm32_usb_phy *phy)
{
	const struct stm32_rcc_usbphyc_config *cfg = phy->pcfg;
	int res;

	/* Configure PHY input frequency selection (mandatory) */
	res = clock_control_configure(rcc, (clock_control_subsys_t)&cfg->fsel, NULL);
	if (res != 0) {
		return res;
	}

	/* Configure PHY clock mux (if provided) */
	if (cfg->has_muxcfg) {
		res = clock_control_configure(rcc, (clock_control_subsys_t)&cfg->muxcfg, NULL);
		if (res != 0) {
			return res;
		}
	}

	/* Enable PHY clock gate (if provided) */
	if (cfg->has_gatecfg) {
		res = clock_control_on(rcc, (clock_control_subsys_t)&cfg->gatecfg);
		if (res != 0) {
			return res;
		}
	}

	return 0;
}

static int stm32_rcc_usbphyc_disable(const struct stm32_usb_phy *phy)
{
	const struct stm32_rcc_usbphyc_config *cfg = phy->pcfg;
	int res;

	/* Disable PHY clock gate (if provided) */
	if (cfg->has_gatecfg) {
		res = clock_control_off(rcc, (clock_control_subsys_t)&cfg->gatecfg);
		if (res != 0) {
			return res;
		}
	}

	return 0;
}

/*
 * Note: we use an IF_ENABLED() to conditionally serialize `fsel`
 * despite the cell being mandatory to reduce noise in build log
 * when building with a DTS where the cell is missing, which will
 * be caught by the BUILD_ASSERT() with a clear error message.
 */
#define DEFINE_USBPHYC_RCC(usb_node, phy_node)							\
	BUILD_ASSERT(DT_CLOCKS_HAS_NAME(phy_node, fsel),					\
		"Missing 'fsel' clock cell in " DT_NODE_PATH(phy_node));			\
	static const struct stm32_rcc_usbphyc_config CONCAT(phy, DT_DEP_ORD(phy_node), _cfg) =	\
	{											\
		IF_ENABLED(DT_CLOCKS_HAS_NAME(phy_node, fsel),					\
			(.fsel = STM32_CLOCK_INFO_BY_NAME(phy_node, fsel),))			\
		IF_ENABLED(DT_CLOCKS_HAS_NAME(phy_node, mux),					\
			(.muxcfg = STM32_CLOCK_INFO_BY_NAME(phy_node, mux),			\
			 .has_muxcfg = true,))							\
		IF_ENABLED(DT_CLOCKS_HAS_NAME(phy_node, gate),					\
			(.gatecfg = STM32_CLOCK_INFO_BY_NAME(phy_node, gate),			\
			 .has_gatecfg = true,))							\
	};											\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {			\
		.enable = stm32_rcc_usbphyc_enable,						\
		.disable = stm32_rcc_usbphyc_disable,						\
		.pcfg = &CONCAT(phy, DT_DEP_ORD(phy_node), _cfg),				\
	};

/*
 * Iterate all USB nodes and instantiate PHY when appropriate.
 * We could implement a stricter check but this should suffice
 * since this pseudo-driver is only compiled for STM32N6 series.
 */
#define _FOREACH_NODE(usb_node)									\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node),					\
		(DEFINE_USBPHYC_RCC(usb_node, USB_STM32_PHY(usb_node))))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
