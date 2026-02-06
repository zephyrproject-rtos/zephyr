/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* STM32F7 USBPHYC driver */
#define DT_DRV_COMPAT st_stm32_usbphyc /* Unused; defined for grep-ability */

#include <stm32_ll_bus.h>

#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "stm32_usb_common.h"

static int stm32f7_usbphyc_enable(const struct stm32_usb_phy *phy)
{
	/*
	 * The USBPHYC is configured by the USB HAL, but it needs
	 * to have its clock enabled beforehand. For some unknown
	 * reason, the OTGHSULPI clock must also be enabled, even
	 * though we are NOT using the ULPI interface...
	 *
	 * We _could_ require both OTGPHYCEN and OTGHSULPIEN bits
	 * to be provided via DTS as clock configuration, but this
	 * would create an ugly mismatch between STM32F7 and other
	 * series. To avoid this, we provide only OTGPHYCEN at DTS
	 * level but straight up ignore the property, instead using
	 * hardcoded LL Bus API calls. This is fine because STM32F7
	 * has only one OTG_HS and USBPHYC instance, so we don't
	 * need to distinguish them...
	 */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
	return 0;
}

static int stm32f7_usbphyc_disable(const struct stm32_usb_phy *phy)
{
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
	return 0;
}

#define DEFINE_USBPHYC_F7(usb_node)						\
	const struct stm32_usb_phy USB_STM32_PHY_PSEUDODEV_NAME(usb_node) = {	\
		.enable = stm32f7_usbphyc_enable,				\
		.disable = stm32f7_usbphyc_disable,				\
	};

/*
 * Iterate all USB nodes and instantiate PHY when appropriate.
 * We could implement a stricter check but this should suffice
 * since this driver is only compiled for STM32F7 series.
 */
#define _FOREACH_NODE(usb_node)							\
	IF_ENABLED(USB_STM32_NODE_PHY_IS_EMBEDDED_HS(usb_node),			\
		(DEFINE_USBPHYC_F7(usb_node)))
#define _FOREACH_COMPAT(compat) DT_FOREACH_STATUS_OKAY(compat, _FOREACH_NODE)
FOR_EACH(_FOREACH_COMPAT, (), STM32_USB_COMPATIBLES)
