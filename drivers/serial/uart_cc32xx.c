/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc32xx_uart

#include <kernel.h>
#include <arch/cpu.h>
#include <drivers/uart.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/prcm.h>
#include <driverlib/uart.h>

struct uart_cc32xx_dev_data_t {
	uint32_t prcm;
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *cb_data; /**< Callback function arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct uart_cc32xx_dev_data_t * const)(dev)->data)

#define PRIME_CHAR '\r'

/* Forward decls: */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_cc32xx_isr(const struct device *dev);
#endif

/*
 *  CC32XX UART has a configurable FIFO length, from 1 to 8 characters.
 *  However, the Zephyr console driver, and the Zephyr uart sample test, assume
 *  a RX FIFO depth of one: meaning, one interrupt == one character received.
 *  Keeping with this assumption, this driver leaves the FIFOs disabled,
 *  and at depth 1.
 */
static int uart_cc32xx_init(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	const struct uart_cc32xx_dev_data_t *data = DEV_DATA(dev);

	MAP_PRCMPeripheralClkEnable(data->prcm,
		    PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);

	MAP_PRCMPeripheralReset(data->prcm);

	/* This also calls MAP_UARTEnable() to enable the FIFOs: */
	MAP_UARTConfigSetExpClk((unsigned long)config->base,
				MAP_PRCMPeripheralClockGet(data->prcm),
				data->baud_rate,
				(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE
				 | UART_CONFIG_PAR_NONE));
	MAP_UARTFlowControlSet((unsigned long)config->base,
			       UART_FLOWCONTROL_NONE);
	/* Re-disable the FIFOs: */
	MAP_UARTFIFODisable((unsigned long)config->base);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Clear any pending UART RX interrupts: */
	MAP_UARTIntClear((unsigned long)config->base, UART_INT_RX);

	config->irq_config_func(dev);

	/* Fill the tx fifo, so Zephyr console & shell subsystems get "primed"
	 * with first tx fifo empty interrupt when they first call
	 * uart_irq_tx_enable().
	 */
	MAP_UARTCharPutNonBlocking((unsigned long)config->base, PRIME_CHAR);
#endif
	return 0;
}

static int uart_cc32xx_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	if (MAP_UARTCharsAvail((unsigned long)config->base)) {
		*c = MAP_UARTCharGetNonBlocking((unsigned long)config->base);
	} else {
		return (-1);
	}
	return 0;
}

static void uart_cc32xx_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UARTCharPut((unsigned long)config->base, c);
}

static int uart_cc32xx_err_check(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned long cc32xx_errs = 0L;
	unsigned int z_err = 0U;

	cc32xx_errs = MAP_UARTRxErrorGet((unsigned long)config->base);

	/* Map cc32xx SDK uart.h defines to zephyr uart.h defines */
	z_err = ((cc32xx_errs & UART_RXERROR_OVERRUN) ?
		  UART_ERROR_OVERRUN : 0) |
		((cc32xx_errs & UART_RXERROR_BREAK) ? UART_BREAK : 0) |
		((cc32xx_errs & UART_RXERROR_PARITY) ? UART_ERROR_PARITY : 0) |
		((cc32xx_errs & UART_RXERROR_FRAMING) ? UART_ERROR_FRAMING : 0);

	MAP_UARTRxErrorClear((unsigned long)config->base);

	return (int)z_err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cc32xx_fifo_fill(const struct device *dev,
				 const uint8_t *tx_data,
				 int size)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int num_tx = 0U;

	while ((size - num_tx) > 0) {
		/* Send a character */
		if (MAP_UARTCharPutNonBlocking((unsigned long)config->base,
					       tx_data[num_tx])) {
			num_tx++;
		} else {
			break;
		}
	}

	return (int)num_tx;
}

static int uart_cc32xx_fifo_read(const struct device *dev, uint8_t *rx_data,
				 const int size)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int num_rx = 0U;

	while (((size - num_rx) > 0) &&
		MAP_UARTCharsAvail((unsigned long)config->base)) {

		/* Receive a character */
		rx_data[num_rx++] =
			MAP_UARTCharGetNonBlocking((unsigned long)config->base);
	}

	return num_rx;
}

static void uart_cc32xx_irq_tx_enable(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UARTIntEnable((unsigned long)config->base, UART_INT_TX);
}

static void uart_cc32xx_irq_tx_disable(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UARTIntDisable((unsigned long)config->base, UART_INT_TX);
}

static int uart_cc32xx_irq_tx_ready(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int int_status;

	int_status = MAP_UARTIntStatus((unsigned long)config->base, 1);

	return (int_status & UART_INT_TX);
}

static void uart_cc32xx_irq_rx_enable(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	/* FIFOs are left disabled from reset, so UART_INT_RT flag not used. */
	MAP_UARTIntEnable((unsigned long)config->base, UART_INT_RX);
}

static void uart_cc32xx_irq_rx_disable(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UARTIntDisable((unsigned long)config->base, UART_INT_RX);
}

static int uart_cc32xx_irq_tx_complete(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	return (!MAP_UARTBusy((unsigned long)config->base));
}

static int uart_cc32xx_irq_rx_ready(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int int_status;

	int_status = MAP_UARTIntStatus((unsigned long)config->base, 1);

	return (int_status & UART_INT_RX);
}

static void uart_cc32xx_irq_err_enable(const struct device *dev)
{
	/* Not yet used in zephyr */
}

static void uart_cc32xx_irq_err_disable(const struct device *dev)
{
	/* Not yet used in zephyr */
}

static int uart_cc32xx_irq_is_pending(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int int_status;

	int_status = MAP_UARTIntStatus((unsigned long)config->base, 1);

	return (int_status & (UART_INT_TX | UART_INT_RX));
}

static int uart_cc32xx_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_cc32xx_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_cc32xx_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * Note: CC32XX UART Tx interrupts when ready to send; Rx interrupts when char
 * received.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uart_cc32xx_isr(const struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	struct uart_cc32xx_dev_data_t * const dev_data = DEV_DATA(dev);

	unsigned long intStatus = MAP_UARTIntStatus((unsigned long)config->base,
						    1);

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
	/*
	 * RX/TX interrupt should have been implicitly cleared by Zephyr UART
	 * clients calling uart_fifo_read() or uart_fifo_write().
	 * Still, clear any error interrupts here, as they're not yet handled.
	 */
	MAP_UARTIntClear((unsigned long)config->base,
			 intStatus & ~(UART_INT_RX | UART_INT_TX));
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_cc32xx_driver_api = {
	.poll_in = uart_cc32xx_poll_in,
	.poll_out = uart_cc32xx_poll_out,
	.err_check = uart_cc32xx_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill	  = uart_cc32xx_fifo_fill,
	.fifo_read	  = uart_cc32xx_fifo_read,
	.irq_tx_enable	  = uart_cc32xx_irq_tx_enable,
	.irq_tx_disable	  = uart_cc32xx_irq_tx_disable,
	.irq_tx_ready	  = uart_cc32xx_irq_tx_ready,
	.irq_rx_enable	  = uart_cc32xx_irq_rx_enable,
	.irq_rx_disable	  = uart_cc32xx_irq_rx_disable,
	.irq_tx_complete  = uart_cc32xx_irq_tx_complete,
	.irq_rx_ready	  = uart_cc32xx_irq_rx_ready,
	.irq_err_enable	  = uart_cc32xx_irq_err_enable,
	.irq_err_disable  = uart_cc32xx_irq_err_disable,
	.irq_is_pending	  = uart_cc32xx_irq_is_pending,
	.irq_update	  = uart_cc32xx_irq_update,
	.irq_callback_set = uart_cc32xx_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define UART_32XX_DEVICE(idx) \
IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, \
	(static void uart_cc32xx_cfg_func_##idx(const struct device *dev) \
	{ \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, ( \
			IRQ_CONNECT(DT_INST_IRQN(idx), \
			    DT_INST_IRQ(idx, priority), \
			    uart_cc32xx_isr, DEVICE_DT_INST_GET(idx), \
			    0); \
			irq_enable(DT_INST_IRQN(idx))) \
		); \
	})); \
static const struct uart_device_config uart_cc32xx_dev_cfg_##idx = { \
	.base = (void *)DT_INST_REG_ADDR(idx), \
	.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(idx, clocks, clock_frequency),\
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, \
		    (.irq_config_func = uart_cc32xx_cfg_func_##idx,)) \
}; \
static struct uart_cc32xx_dev_data_t uart_cc32xx_dev_data_##idx = { \
	.prcm = PRCM_UARTA##idx, \
	.baud_rate = DT_INST_PROP(idx, current_speed), \
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (.cb = NULL,)) \
}; \
DEVICE_DT_INST_DEFINE(idx, uart_cc32xx_init, \
	device_pm_control_nop, &uart_cc32xx_dev_data_##idx, \
	&uart_cc32xx_dev_cfg_##idx, \
	PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
	(void *)&uart_cc32xx_driver_api); \

DT_INST_FOREACH_STATUS_OKAY(UART_32XX_DEVICE);
