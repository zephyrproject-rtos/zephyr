/*
 * Copyright (c) 2021, Kwon Tae-young <tykwon@m2i.co.kr>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>

static int mimx8mq_evk_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay)
	IOMUXC_SetPinMux(IOMUXC_UART2_RXD_UART2_RX, 0U);
	IOMUXC_SetPinConfig(IOMUXC_UART2_RXD_UART2_RX,
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
			    IOMUXC_SW_PAD_CTL_PAD_SRE(2U));
	IOMUXC_SetPinMux(IOMUXC_UART2_TXD_UART2_TX, 0U);
	IOMUXC_SetPinConfig(IOMUXC_UART2_TXD_UART2_TX,
			    IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
			    IOMUXC_SW_PAD_CTL_PAD_SRE(2U));
#endif

	return 0;
}

SYS_INIT(mimx8mq_evk_pinmux_init, PRE_KERNEL_1, 0);
