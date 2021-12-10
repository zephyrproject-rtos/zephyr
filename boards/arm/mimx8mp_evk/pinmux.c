/*
 * Copyright (c) 2021, Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iomuxc.h>

static int mimx8mp_evk_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay)
	IOMUXC_SetPinMux(IOMUXC_UART4_RXD_UART4_RX, 0U);
	IOMUXC_SetPinConfig(IOMUXC_UART4_RXD_UART4_RX,
						IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
						IOMUXC_SW_PAD_CTL_PAD_PE_MASK);
	IOMUXC_SetPinMux(IOMUXC_UART4_TXD_UART4_TX, 0U);
	IOMUXC_SetPinConfig(IOMUXC_UART4_TXD_UART4_TX,
						IOMUXC_SW_PAD_CTL_PAD_PUE_MASK |
						IOMUXC_SW_PAD_CTL_PAD_PE_MASK);
#endif

	return 0;

}

SYS_INIT(mimx8mp_evk_pinmux_init, PRE_KERNEL_1, 0);
