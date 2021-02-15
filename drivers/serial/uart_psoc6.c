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

/* UART desired baud rate is 115200 bps (Standard mode).
 * The UART baud rate = (SCB clock frequency / Oversample).
 * For PeriClk = 50 MHz, select divider value 36 and get
 * SCB clock = (50 MHz / 36) = 1,389 MHz.
 * Select Oversample = 12.
 * These setting results UART data rate = 1,389 MHz / 12 = 115750 bps.
 */
#define UART_PSOC6_CONFIG_OVERSAMPLE      (12UL)
#define UART_PSOC6_CONFIG_BREAKWIDTH      (11UL)
#define UART_PSOC6_CONFIG_DATAWIDTH       (8UL)

/* Assign divider type and number for UART */
#define UART_PSOC6_UART_CLK_DIV_TYPE     (CY_SYSCLK_DIV_8_BIT)
#define UART_PSOC6_UART_CLK_DIV_NUMBER   (PERI_DIV_8_NR - 1u)
#define UART_PSOC6_UART_CLK_DIV_VAL      (35UL)

/*
 * Verify Kconfig configuration
 */

struct cypress_psoc6_config {
	CySCB_Type *base;
	GPIO_PRT_Type *port;
	uint32_t rx_num;
	uint32_t tx_num;
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

	.oversample                 = UART_PSOC6_CONFIG_OVERSAMPLE,

	.enableMsbFirst             = false,
	.dataWidth                  = UART_PSOC6_CONFIG_DATAWIDTH,
	.parity                     = CY_SCB_UART_PARITY_NONE,
	.stopBits                   = CY_SCB_UART_STOP_BITS_1,
	.enableInputFilter          = false,
	.breakWidth                 = UART_PSOC6_CONFIG_BREAKWIDTH,
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
static int uart_psoc6_init(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;

	/* Connect SCB5 UART function to pins */
	Cy_GPIO_SetHSIOM(config->port, config->rx_num, config->rx_val);
	Cy_GPIO_SetHSIOM(config->port, config->tx_num, config->tx_val);

	/* Configure pins for UART operation */
	Cy_GPIO_SetDrivemode(config->port, config->rx_num, CY_GPIO_DM_HIGHZ);
	Cy_GPIO_SetDrivemode(config->port, config->tx_num,
		CY_GPIO_DM_STRONG_IN_OFF);

	/* Connect assigned divider to be a clock source for UART */
	Cy_SysClk_PeriphAssignDivider(config->scb_clock,
		UART_PSOC6_UART_CLK_DIV_TYPE,
		UART_PSOC6_UART_CLK_DIV_NUMBER);

	Cy_SysClk_PeriphSetDivider(UART_PSOC6_UART_CLK_DIV_TYPE,
		UART_PSOC6_UART_CLK_DIV_NUMBER,
		UART_PSOC6_UART_CLK_DIV_VAL);
	Cy_SysClk_PeriphEnableDivider(UART_PSOC6_UART_CLK_DIV_TYPE,
		UART_PSOC6_UART_CLK_DIV_NUMBER);

	/* Configure UART to operate */
	(void) Cy_SCB_UART_Init(config->base, &uartConfig, NULL);
	Cy_SCB_UART_Enable(config->base);

	return 0;
}

static int uart_psoc6_poll_in(const struct device *dev, unsigned char *c)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t rec;

	rec = Cy_SCB_UART_Get(config->base);
	*c = (unsigned char)(rec & 0xff);

	return ((rec == CY_SCB_UART_RX_NO_DATA) ? -1 : 0);
}

static void uart_psoc6_poll_out(const struct device *dev, unsigned char c)
{
	const struct cypress_psoc6_config *config = dev->config;

	while (Cy_SCB_UART_Put(config->base, (uint32_t)c) != 1UL) {
	}
}

static const struct uart_driver_api uart_psoc6_driver_api = {
	.poll_in = uart_psoc6_poll_in,
	.poll_out = uart_psoc6_poll_out,
};

#ifdef CONFIG_UART_PSOC6_UART_5
static const struct cypress_psoc6_config cypress_psoc6_uart5_config = {
	.base = (CySCB_Type *)DT_REG_ADDR(DT_NODELABEL(uart5)),
	.port = P5_0_PORT,
	.rx_num = P5_0_NUM,
	.tx_num = P5_1_NUM,
	.rx_val = P5_0_SCB5_UART_RX,
	.tx_val = P5_1_SCB5_UART_TX,
	.scb_clock = PCLK_SCB5_CLOCK,
};

DEVICE_DT_DEFINE(DT_NODELABEL(uart5),
			uart_psoc6_init, device_pm_control_nop, NULL,
			&cypress_psoc6_uart5_config,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			(void *)&uart_psoc6_driver_api);
#endif /* CONFIG_UART_PSOC6_UART_5 */

#ifdef CONFIG_UART_PSOC6_UART_6
static const struct cypress_psoc6_config cypress_psoc6_uart6_config = {
	.base = (CySCB_Type *)DT_REG_ADDR(DT_NODELABEL(uart6)),
	.port = P12_0_PORT,
	.rx_num = P12_0_NUM,
	.tx_num = P12_1_NUM,
	.rx_val = P12_0_SCB6_UART_RX,
	.tx_val = P12_1_SCB6_UART_TX,
	.scb_clock = PCLK_SCB6_CLOCK,
};

DEVICE_DT_DEFINE(DT_NODELABEL(uart6),
			uart_psoc6_init, device_pm_control_nop, NULL,
			&cypress_psoc6_uart6_config,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			(void *)&uart_psoc6_driver_api);
#endif /* CONFIG_UART_PSOC6_UART_6 */
