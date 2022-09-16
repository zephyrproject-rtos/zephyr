/*
 * Copyright (c) 2018 Cypress
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cypress_psoc6_uart

/** @file
 * @brief UART driver for Cypress PSoC6 MCU family.
 *
 * Note:
 * - Error handling is not implemented.
 * - The driver works only in polling mode, interrupt mode is not implemented.
 */
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>

#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_scb_uart.h"
#include "cy_sysint.h"

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
	uint32_t periph_id;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t	irq_config_func;
#endif
	uint32_t num_pins;
	struct soc_gpio_pin pins[];
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct cypress_psoc6_data {
	uart_irq_callback_user_data_t irq_cb;	/* Interrupt Callback */
	void *irq_cb_data;			/* Interrupt Callback Arg */
};

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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
 *  Performs hardware initialization: debug UART.
 *
 */
static int uart_psoc6_init(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;

	soc_gpio_list_configure(config->pins, config->num_pins);

	/* Connect assigned divider to be a clock source for UART */
	Cy_SysClk_PeriphAssignDivider(config->periph_id,
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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

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

static int uart_psoc6_err_check(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t status = Cy_SCB_UART_GetRxFifoStatus(config->base);
	int errors = 0;

	if (status & CY_SCB_UART_RX_OVERFLOW) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (status & CY_SCB_UART_RX_ERR_PARITY) {
		errors |= UART_ERROR_PARITY;
	}

	if (status & CY_SCB_UART_RX_ERR_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_psoc6_fifo_fill(const struct device *dev,
				const uint8_t *tx_data,
				int size)
{
	const struct cypress_psoc6_config *config = dev->config;

	return Cy_SCB_UART_PutArray(config->base, (uint8_t *) tx_data, size);
}

static int uart_psoc6_fifo_read(const struct device *dev,
				uint8_t *rx_data,
				const int size)
{
	const struct cypress_psoc6_config *config = dev->config;

	return Cy_SCB_UART_GetArray(config->base, rx_data, size);
}

static void uart_psoc6_irq_tx_enable(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;

	Cy_SCB_SetTxInterruptMask(config->base, CY_SCB_UART_TX_EMPTY);
}

static void uart_psoc6_irq_tx_disable(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;

	Cy_SCB_SetTxInterruptMask(config->base, 0);
}

static int uart_psoc6_irq_tx_ready(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t status = Cy_SCB_UART_GetTxFifoStatus(config->base);

	Cy_SCB_UART_ClearTxFifoStatus(config->base, CY_SCB_UART_TX_INTR_MASK);

	return (status & CY_SCB_UART_TX_NOT_FULL);
}

static int uart_psoc6_irq_tx_complete(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t status = Cy_SCB_UART_GetTxFifoStatus(config->base);

	Cy_SCB_UART_ClearTxFifoStatus(config->base, CY_SCB_UART_TX_INTR_MASK);

	return (status & CY_SCB_UART_TX_DONE);
}

static void uart_psoc6_irq_rx_enable(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;

	Cy_SCB_SetRxInterruptMask(config->base, CY_SCB_UART_RX_NOT_EMPTY);
}

static void uart_psoc6_irq_rx_disable(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;

	Cy_SCB_SetRxInterruptMask(config->base, 0);
}

static int uart_psoc6_irq_rx_ready(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t status = Cy_SCB_UART_GetRxFifoStatus(config->base);

	Cy_SCB_UART_ClearRxFifoStatus(config->base, CY_SCB_UART_RX_INTR_MASK);

	return (status & CY_SCB_UART_RX_NOT_EMPTY);
}

static void uart_psoc6_irq_err_enable(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t intmask = Cy_SCB_GetRxInterruptMask(config->base) |
			   CY_SCB_UART_RECEIVE_ERR;

	Cy_SCB_SetRxInterruptMask(config->base, intmask);
}

static void uart_psoc6_irq_err_disable(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t intmask = Cy_SCB_GetRxInterruptMask(config->base) &
			    ~(CY_SCB_UART_RECEIVE_ERR);

	Cy_SCB_SetRxInterruptMask(config->base, intmask);
}

static int uart_psoc6_irq_is_pending(const struct device *dev)
{
	const struct cypress_psoc6_config *config = dev->config;
	uint32_t intcause = Cy_SCB_GetInterruptCause(config->base);

	return (intcause & (CY_SCB_TX_INTR | CY_SCB_RX_INTR));
}

static int uart_psoc6_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_psoc6_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct cypress_psoc6_data *const dev_data = dev->data;

	dev_data->irq_cb = cb;
	dev_data->irq_cb_data = cb_data;
}

static void uart_psoc6_isr(const struct device *dev)
{
	struct cypress_psoc6_data *const dev_data = dev->data;

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev, dev_data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_psoc6_driver_api = {
	.poll_in = uart_psoc6_poll_in,
	.poll_out = uart_psoc6_poll_out,
	.err_check = uart_psoc6_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_psoc6_fifo_fill,
	.fifo_read = uart_psoc6_fifo_read,
	.irq_tx_enable = uart_psoc6_irq_tx_enable,
	.irq_tx_disable = uart_psoc6_irq_tx_disable,
	.irq_tx_ready = uart_psoc6_irq_tx_ready,
	.irq_rx_enable = uart_psoc6_irq_rx_enable,
	.irq_rx_disable = uart_psoc6_irq_rx_disable,
	.irq_tx_complete = uart_psoc6_irq_tx_complete,
	.irq_rx_ready = uart_psoc6_irq_rx_ready,
	.irq_err_enable = uart_psoc6_irq_err_enable,
	.irq_err_disable = uart_psoc6_irq_err_disable,
	.irq_is_pending = uart_psoc6_irq_is_pending,
	.irq_update = uart_psoc6_irq_update,
	.irq_callback_set = uart_psoc6_irq_callback_set,
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define CY_PSOC6_UART_IRQ_FUNC(n)						\
	static void cy_psoc6_uart##n##_irq_config(const struct device *port)	\
	{									\
		CY_PSOC6_DT_INST_NVIC_INSTALL(n,				\
					      uart_psoc6_isr);			\
	};
#define CY_PSOC6_UART_IRQ_SET_FUNC(n)						\
	.irq_config_func = cy_psoc6_uart##n##_irq_config
#define CY_PSOC6_UART_DECL_DATA(n)						\
	static struct cypress_psoc6_data cy_psoc6_uart##n##_data = { 0 };
#define CY_PSOC6_UART_DECL_DATA_PTR(n) &cy_psoc6_uart##n##_data
#else
#define CY_PSOC6_UART_IRQ_FUNC(n)
#define CY_PSOC6_UART_IRQ_SET_FUNC(n)
#define CY_PSOC6_UART_DECL_DATA(n)
#define CY_PSOC6_UART_DECL_DATA_PTR(n) NULL
#endif

#define CY_PSOC6_UART_INIT(n)							\
	CY_PSOC6_UART_DECL_DATA(n)						\
	CY_PSOC6_UART_IRQ_FUNC(n)						\
	static const struct cypress_psoc6_config cy_psoc6_uart##n##_config = {	\
		.base = (CySCB_Type *)DT_INST_REG_ADDR(n),			\
		.periph_id = DT_INST_PROP(n, peripheral_id),			\
										\
		.num_pins = CY_PSOC6_DT_INST_NUM_PINS(n),			\
		.pins = CY_PSOC6_DT_INST_PINS(n),				\
										\
		CY_PSOC6_UART_IRQ_SET_FUNC(n)					\
	};									\
	DEVICE_DT_INST_DEFINE(n, &uart_psoc6_init, NULL,			\
			      CY_PSOC6_UART_DECL_DATA_PTR(n),			\
			      &cy_psoc6_uart##n##_config, PRE_KERNEL_1,		\
			      CONFIG_SERIAL_INIT_PRIORITY,			\
			      &uart_psoc6_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CY_PSOC6_UART_INIT)
