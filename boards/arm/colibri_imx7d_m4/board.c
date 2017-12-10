/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <init.h>
#include "board.h"
#include "ccm_imx7d.h"
#include "rdc.h"
#include "wdog_imx.h"
#include "uart_imx.h"

static void configure_uart_pins(UART_Type* base)
{
    switch((uint32_t)base)
    {
        case UART2_BASE:
            // UART2 iomux configuration
            IOMUXC_SW_MUX_CTL_PAD_UART2_RX_DATA = IOMUXC_SW_MUX_CTL_PAD_UART2_RX_DATA_MUX_MODE(0);
            IOMUXC_SW_MUX_CTL_PAD_UART2_TX_DATA = IOMUXC_SW_MUX_CTL_PAD_UART2_TX_DATA_MUX_MODE(0);
            IOMUXC_SW_PAD_CTL_PAD_UART2_RX_DATA = IOMUXC_SW_PAD_CTL_PAD_UART2_RX_DATA_PE_MASK  |
                                                  IOMUXC_SW_PAD_CTL_PAD_UART2_RX_DATA_PS(3)    |
                                                  IOMUXC_SW_PAD_CTL_PAD_UART2_RX_DATA_HYS_MASK |
                                                  IOMUXC_SW_PAD_CTL_PAD_UART2_RX_DATA_DSE(0);
            IOMUXC_SW_PAD_CTL_PAD_UART2_TX_DATA = IOMUXC_SW_PAD_CTL_PAD_UART2_TX_DATA_PE_MASK  |
                                                  IOMUXC_SW_PAD_CTL_PAD_UART2_TX_DATA_PS(3)    |
                                                  IOMUXC_SW_PAD_CTL_PAD_UART2_RX_DATA_HYS_MASK |
                                                  IOMUXC_SW_PAD_CTL_PAD_UART2_TX_DATA_DSE(0);
            IOMUXC_UART2_RX_DATA_SELECT_INPUT = IOMUXC_UART2_RX_DATA_SELECT_INPUT_DAISY(3); /* Select TX_PAD for RX data (DTE mode...) */
            break;
        default:
            break;
    }
}


static void board_clock_init(void)
{
}

/* Initialize debug console. */
static void board_dbg_uart_init(void)
{
}

static void board_rdc_init(void)
{
}

static int colibri_imx7d_init(struct device *dev)
{
	ARG_UNUSED(dev);

    board_clock_init();
    board_rdc_init();
    configure_uart_pins(UART2);
    board_dbg_uart_init();

	return 0;
}

SYS_INIT(colibri_imx7d_init, PRE_KERNEL_1, 0);

/*******************************************************************************
 * EOF
 ******************************************************************************/
