/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Embedded FS PHY driver for STM32H7 & STM32F4
 *
 * The embedded FS PHY itself never needs to be configured;
 * this is really only needed due to a silly quirk on STM32H7
 * and some STM32F4 parts, where the ULPI clock low-power sleep
 * must be disabled when using an USB instance in Full-Speed mode.
 *
 * Note use of __maybe_unused because this is included
 * unconditionally in the build, but we may end up not
 * instancing any device.
 */
#include <stm32_ll_bus.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

__maybe_unused
static int stm32_embedded_fs_phy_enable(const struct stm32_usb_phy *phy)
{
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_AHB1_GRP1_DisableClockSleep(phy->cfg);
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
	if (phy->cfg != 0U) {
		LL_AHB1_GRP1_DisableClockLowPower(phy->cfg);
	}
#endif
	return 0;
}

__maybe_unused
static int stm32_embedded_fs_phy_disable(const struct stm32_usb_phy *phy)
{
	/* Re-enable low-power sleep when PHY is no longer used. */
#if defined(CONFIG_SOC_SERIES_STM32H7X)
	LL_AHB1_GRP1_EnableClockSleep(phy->cfg);
#elif defined(CONFIG_SOC_SERIES_STM32F4X)
	if (phy->cfg != 0U) {
		LL_AHB1_GRP1_EnableClockLowPower(phy->cfg);
	}
#endif
	return 0;
}

/* PHY configuration */

/*
 * STM32H7 has two instances of the same OTGHS IP; however, the
 * second instance "USB2OTGHS" can only be used in FS mode since
 * it has no HS PHY nor ULPI interface. As such, "USB1OTGHS" has
 * compatible "st,stm32-otghs" in Devicetree whereas "USB2OTGHS"
 * has compatible "st,stm32-otgfs".
 *
 * Use compatible here to distinguish between the two controllers.
 */
#define STM32H7_PHY_CFG(usb_node)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(usb_node, st_stm32_otghs),		\
		(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI),				\
		(LL_AHB1_GRP1_PERIPH_USB2OTGHSULPI))

/**
 * STM32F4 has one instance of OTGHS and one instance of OTGFS.
 * Use same trick as on H7 to distinguish them, and set cfg=0
 * on OTG_FS as sentinel for skipping the MMIO write at runtime.
 */
#define STM32F4_PHY_CFG(usb_node)						\
	COND_CODE_1(DT_NODE_HAS_COMPAT(usb_node, st_stm32_otghs),		\
		(LL_AHB1_GRP1_PERIPH_OTGHSULPI), (0))

#define EMBFS_PHY_CFG(usb_node)							\
	IF_ENABLED(CONFIG_SOC_SERIES_STM32H7X,					\
		(STM32H7_PHY_CFG(usb_node)))					\
	IF_ENABLED(CONFIG_SOC_SERIES_STM32F4X,					\
		(STM32F4_PHY_CFG(usb_node)))

#define DEFINE_EMBFS_PHY(usb_node)						\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {	\
		.enable = stm32_embedded_fs_phy_enable,				\
		.disable = stm32_embedded_fs_phy_disable,			\
		.cfg = EMBFS_PHY_CFG(usb_node),					\
	};

/* Iterate all USB nodes and instantiate PHY when appropriate */
#define _FOREACH_NODE(usb_node)							\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_EMBEDDED_FS(usb_node),			\
		(DEFINE_EMBFS_PHY(usb_node)))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
