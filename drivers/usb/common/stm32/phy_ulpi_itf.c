/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ULPI PHY interface driver
 *
 * Note use of __maybe_unused because this is included
 * unconditionally in the build, but we may end up not
 * instancing any device.
 */
#include <stm32_ll_bus.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

/*
 * A single implementation works across all series
 * because it turns out they all have ULPI clock
 * in the same RCC register.
 */

__maybe_unused
static int stm32_ulpi_itf_enable(const struct stm32_usb_phy *phy)
{
	LL_AHB1_GRP1_EnableClock(phy->cfg);
	return 0;
}

__maybe_unused
static int stm32_ulpi_itf_disable(const struct stm32_usb_phy *phy)
{
	LL_AHB1_GRP1_DisableClock(phy->cfg);
	return 0;
}

/* Pseudo-PHY configuration */

/*
 * Most series with ULPI interface have only one OTG_HS instance,
 * so the bit is simply called "OTGHSULPI".
 *
 * STM32H7 has two instances but only USB1 can be use an ULPI PHY;
 * as such, it is safe to hardcode "USB1OTGHSULPI" here.
 */
#define ULPI_ITF_CFG(usb_node)							\
	COND_CODE_1(CONFIG_SOC_SERIES_STM32H7X,					\
		(LL_AHB1_GRP1_PERIPH_USB1OTGHSULPI),				\
		(LL_AHB1_GRP1_PERIPH_OTGHSULPI))

/* Instantiate ULPI pseudo-PHY devices */
#define DEFINE_ULPI_PHY(usb_node)						\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {	\
		.enable = stm32_ulpi_itf_enable,				\
		.disable = stm32_ulpi_itf_disable,				\
		.cfg = ULPI_ITF_CFG(usb_node),					\
	};

/* Iterate all USB nodes and instantiate PHY when appropriate */
#define _FOREACH_NODE(usb_node)							\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_ULPI(usb_node),			\
		(DEFINE_ULPI_PHY(usb_node)))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
