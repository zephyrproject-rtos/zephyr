/*
 * Copyright (c) 2020, Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>

static int mimx8mm_evk_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
	IOMUXC_SetPinMux(IOMUXC_UART4_RXD_UART4_RX, 0U);
	IOMUXC_SetPinConfig(IOMUXC_UART4_RXD_UART4_RX,
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
			    IOMUXC_SW_PAD_CTL_PAD_FSEL(2U));
	IOMUXC_SetPinMux(IOMUXC_UART4_TXD_UART4_TX, 0U);
	IOMUXC_SetPinConfig(IOMUXC_UART4_TXD_UART4_TX,
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
			    IOMUXC_SW_PAD_CTL_PAD_FSEL(2U));
#endif

	return 0;

}

SYS_INIT(mimx8mm_evk_pinmux_init, PRE_KERNEL_1, 0);
