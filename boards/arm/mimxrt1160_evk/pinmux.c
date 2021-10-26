/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>
#include <soc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(mimxrt1160_evk, LOG_LEVEL_INF);

static int mimxrt1160_evk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CLOCK_EnableClock(kCLOCK_Iomuxc);

	/* Enable USER_LED_CTRL1 */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_04_GPIO9_IO03, 0U);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart1), okay) && CONFIG_SERIAL
	/* LPUART1 TX/RX */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_24_LPUART1_TXD, 0U);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_25_LPUART1_RXD, 0U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_24_LPUART1_TXD, 0x02U);
	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_25_LPUART1_RXD, 0x02U);
#endif
	return 0;
}

SYS_INIT(mimxrt1160_evk_init, PRE_KERNEL_1, 0);
