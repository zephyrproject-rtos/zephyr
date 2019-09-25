/*
 * Copyright (c) 2018 Cypress
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief UART driver for Cypress PSoC6 MCU family.
 *
 * Note:
 * - Error handling is not implemented.
 * - The driver works only in polling mode, interrupt mode is not implemented.
 */
#include <device.h>
#include <errno.h>
#include <init.h>
#include <sys/__assert.h>
#include <soc.h>
#include <drivers/uart.h>

#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_gpio.h"
#include "cy_scb_uart.h"

/*
 * Verify Kconfig configuration
 */

struct cypress_psoc6_config {
	CySCB_Type *base;
	GPIO_PRT_Type *port;
	u32_t rx_num;
	u32_t tx_num;
	en_hsiom_sel_t rx_val;
	en_hsiom_sel_t tx_val;
	en_clk_dst_t scb_clock;
};

/* Populate configuration structure */
static const cy_stc_scb_uart_config_t uartConfig = {
	.uartMode                   = CY_SCB_UART_STANDARD,
	.enableMutliProcessorMode   = false,
	.smartCardRetryOnNack       = false,
	.irdaInvertRx               = false,
	.irdaEnableLowPowerReceiver = false,

	.oversample                 = DT_UART_PSOC6_CONFIG_OVERSAMPLE,

	.enableMsbFirst             = false,
	.dataWidth                  = DT_UART_PSOC6_CONFIG_DATAWIDTH,
	.parity                     = CY_SCB_UART_PARITY_NONE,
	.stopBits                   = CY_SCB_UART_STOP_BITS_1,
	.enableInputFilter          = false,
	.breakWidth                 = DT_UART_PSOC6_CONFIG_BREAKWIDTH,
	.dropOnFrameError           = false,
	.dropOnParityError          = false,

	.receiverAddress            = 0UL,
	.receiverAddressMask        = 0UL,
	.acceptAddrInFifo           = false,

	.enableCts                  = false,
	.ctsPolarity                = CY_SCB_UART_ACTIVE_LOW,
	.rtsRxFifoLevel             = 0UL,
	.rtsPolarity                = CY_SCB_UART_ACTIVE_LOW,

	.rxFifoTriggerLevel  = 0UL,
	.rxFifoIntEnableMask = 0UL,
	.txFifoTriggerLevel  = 0UL,
	.txFifoIntEnableMask = 0UL,
};

/**
 * Function Name: uart_psoc6_init()
 *
 *  Peforms hardware initialization: debug UART.
 *
 */
static int uart_psoc6_init(struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config->config_info;

	/* Connect SCB5 UART function to pins */
	Cy_GPIO_SetHSIOM(config->port, config->rx_num, config->rx_val);
	Cy_GPIO_SetHSIOM(config->port, config->tx_num, config->tx_val);

	/* Configure pins for UART operation */
	Cy_GPIO_SetDrivemode(config->port, config->rx_num, CY_GPIO_DM_HIGHZ);
	Cy_GPIO_SetDrivemode(config->port, config->tx_num,
		CY_GPIO_DM_STRONG_IN_OFF);

	/* Connect assigned divider to be a clock source for UART */
	Cy_SysClk_PeriphAssignDivider(config->scb_clock,
		DT_UART_PSOC6_UART_CLK_DIV_TYPE,
		DT_UART_PSOC6_UART_CLK_DIV_NUMBER);

	Cy_SysClk_PeriphSetDivider(DT_UART_PSOC6_UART_CLK_DIV_TYPE,
		DT_UART_PSOC6_UART_CLK_DIV_NUMBER,
		DT_UART_PSOC6_UART_CLK_DIV_VAL);
	Cy_SysClk_PeriphEnableDivider(DT_UART_PSOC6_UART_CLK_DIV_TYPE,
		DT_UART_PSOC6_UART_CLK_DIV_NUMBER);

	/* Configure UART to operate */
	(void) Cy_SCB_UART_Init(config->base, &uartConfig, NULL);
	Cy_SCB_UART_Enable(config->base);

	return 0;
}

static int uart_psoc6_poll_in(struct device *dev, unsigned char *c)
{
	const struct cypress_psoc6_config *config = dev->config->config_info;
	u32_t rec;

	rec = Cy_SCB_UART_Get(config->base);
	*c = (unsigned char)(rec & 0xff);

	return ((rec == CY_SCB_UART_RX_NO_DATA) ? -1 : 0);
}

static void uart_psoc6_poll_out(struct device *dev, unsigned char c)
{
	const struct cypress_psoc6_config *config = dev->config->config_info;

	while (Cy_SCB_UART_Put(config->base, (uint32_t)c) != 1UL) {
	}
}

static const struct uart_driver_api uart_psoc6_driver_api = {
	.poll_in = uart_psoc6_poll_in,
	.poll_out = uart_psoc6_poll_out,
};

#ifdef CONFIG_UART_PSOC6_UART_5
static const struct cypress_psoc6_config cypress_psoc6_uart5_config = {
	.base = DT_UART_PSOC6_UART_5_BASE_ADDRESS,
	.port = DT_UART_PSOC6_UART_5_PORT,
	.rx_num = DT_UART_PSOC6_UART_5_RX_NUM,
	.tx_num = DT_UART_PSOC6_UART_5_TX_NUM,
	.rx_val = DT_UART_PSOC6_UART_5_RX_VAL,
	.tx_val = DT_UART_PSOC6_UART_5_TX_VAL,
	.scb_clock = DT_UART_PSOC6_UART_5_CLOCK,
};

DEVICE_AND_API_INIT(uart_5, DT_UART_PSOC6_UART_5_NAME,
			uart_psoc6_init, NULL,
			&cypress_psoc6_uart5_config,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			(void *)&uart_psoc6_driver_api);
#endif /* CONFIG_UART_PSOC6_UART_5 */

#ifdef CONFIG_UART_PSOC6_UART_6
static const struct cypress_psoc6_config cypress_psoc6_uart6_config = {
	.base = DT_UART_PSOC6_UART_6_BASE_ADDRESS,
	.port = DT_UART_PSOC6_UART_6_PORT,
	.rx_num = DT_UART_PSOC6_UART_6_RX_NUM,
	.tx_num = DT_UART_PSOC6_UART_6_TX_NUM,
	.rx_val = DT_UART_PSOC6_UART_6_RX_VAL,
	.tx_val = DT_UART_PSOC6_UART_6_TX_VAL,
	.scb_clock = DT_UART_PSOC6_UART_6_CLOCK,
};

DEVICE_AND_API_INIT(uart_6, DT_UART_PSOC6_UART_6_NAME,
			uart_psoc6_init, NULL,
			&cypress_psoc6_uart6_config,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			(void *)&uart_psoc6_driver_api);
#endif /* CONFIG_UART_PSOC6_UART_6 */
