/*
 * Copyright (c) 2020, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>
#include <fsl_gpio.h>


static int mimxrt1024_evk_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	CLOCK_EnableClock(kCLOCK_Iomuxc);
	CLOCK_EnableClock(kCLOCK_IomuxcSnvs);

	/* LED */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B1_08_GPIO1_IO24, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B1_08_GPIO1_IO24,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	/* SW4 */
	IOMUXC_SetPinMux(IOMUXC_SNVS_WAKEUP_GPIO5_IO00, 0);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart1), okay) && CONFIG_SERIAL
	/* LPUART1 TX/RX */
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_06_LPUART1_TX, 0);
	IOMUXC_SetPinMux(IOMUXC_GPIO_AD_B0_07_LPUART1_RX, 0);

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_06_LPUART1_TX,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));

	IOMUXC_SetPinConfig(IOMUXC_GPIO_AD_B0_07_LPUART1_RX,
			    IOMUXC_SW_PAD_CTL_PAD_PKE_MASK |
			    IOMUXC_SW_PAD_CTL_PAD_SPEED(2) |
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6));
#endif

	return 0;
}

SYS_INIT(mimxrt1024_evk_init, PRE_KERNEL_1, 0);
