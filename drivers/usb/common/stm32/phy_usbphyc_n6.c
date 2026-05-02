/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* STM32N6 USBPHYC driver */
#define DT_DRV_COMPAT st_stm32_usbphyc /* Unused; defined for grep-ability */

#include <soc.h>

#include <stm32_bitops.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

/* TODO: should not be hardcoded! */
#define USB_USBPHYC_CR_FSEL_24MHZ        USB_USBPHYC_CR_FSEL_1

struct stm32n6_usbphyc_config {
	USB_HS_PHYC_GlobalTypeDef *reg;
	uint32_t num_clocks;
	struct stm32_pclken clocks[];
};

static const struct device *rcc = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

/*
 * NOTE: the USBPHYC MMIO interface is clock gated by
 * the same bit as the OTG_HS instance itself; this
 * function MUST be called after clock_control_on()
 * in the main USB driver or the SoC will deadlock.
 */
static int stm32n6_usbphyc_enable(const struct stm32_usb_phy *phy)
{
	const struct stm32n6_usbphyc_config *cfg = phy->pcfg;
	int res;

	/* Configure PHY input frequency selection */
	stm32_reg_modify_bits(&cfg->reg->USBPHYC_CR,
			      USB_USBPHYC_CR_FSEL_Msk,
			      USB_USBPHYC_CR_FSEL_24MHZ);

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

static int stm32n6_usbphyc_disable(const struct stm32_usb_phy *phy)
{
	const struct stm32n6_usbphyc_config *cfg = phy->pcfg;

	return clock_control_off(rcc, (clock_control_subsys_t)&cfg->clocks[0]);
}

#define DEFINE_USBPHYC_N6(usb_node, phy_node)							\
	static const struct stm32n6_usbphyc_config CONCAT(phy, DT_DEP_ORD(phy_node), _cfg) = {	\
		.reg = (void *)DT_REG_ADDR(phy_node),						\
		.num_clocks = DT_NUM_CLOCKS(phy_node),						\
		.clocks = STM32_DT_CLOCKS(phy_node)						\
	};											\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {			\
		.enable = stm32n6_usbphyc_enable,						\
		.disable = stm32n6_usbphyc_disable,						\
		.pcfg = &CONCAT(phy, DT_DEP_ORD(phy_node), _cfg),				\
	};

/*
 * Iterate all USB nodes and instantiate PHY when appropriate.
 * We could implement a stricter check but this should suffice
 * since this pseudo-driver is only compiled for STM32N6 series.
 */
#define _FOREACH_NODE(usb_node)									\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node),					\
		(DEFINE_USBPHYC_N6(usb_node, USB_STM32_PHY(usb_node))))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
