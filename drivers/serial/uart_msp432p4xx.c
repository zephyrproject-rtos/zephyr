/*
 * Copyright (c) 2017, Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* See www.ti.com/lit/pdf/slau356f, Chapter 22, for MSP432P4XX UART info. */

/* include driverlib/gpio.h (from the msp432p4xx SDK) before Z's uart.h so
 * that the definition of BIT is not overridden */
#include <driverlib/gpio.h>

#include <drivers/uart.h>

/* Driverlib includes */
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>

struct uart_msp432p4xx_dev_data_t {
	/* UART config structure */
	eUSCI_UART_Config uartConfig;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *cb_data;  /**< Callback function arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_msp432p4xx_dev_data_t * const)(dev)->driver_data)

static struct device DEVICE_NAME_GET(uart_msp432p4xx_0);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_msp432p4xx_isr(void *arg);
#endif

static const struct uart_device_config uart_msp432p4xx_dev_cfg_0 = {
	.base = (void *)DT_UART_MSP432P4XX_BASE_ADDRESS,
	.sys_clk_freq = DT_UART_MSP432P4XX_CLOCK_FREQUENCY,
};

static struct uart_msp432p4xx_dev_data_t uart_msp432p4xx_dev_data_0 = {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif
};

static int baudrate_set(eUSCI_UART_Config *config, uint32_t baudrate)
{
	u16_t prescalar;
	u8_t first_mod_reg, second_mod_reg;

	switch (baudrate) {
	case 1200:
		prescalar = 2500U;
		first_mod_reg = 0U;
		second_mod_reg = 0U;
		break;
	case 2400:
		prescalar = 1250U;
		first_mod_reg = 0U;
		second_mod_reg = 0U;
		break;
	case 4800:
		prescalar = 625U;
		first_mod_reg = 0U;
		second_mod_reg = 0U;
		break;
	case 9600:
		prescalar = 312U;
		first_mod_reg = 8U;
		second_mod_reg = 0U;
		break;
	case 19200:
		prescalar = 156U;
		first_mod_reg = 4U;
		second_mod_reg = 0U;
		break;
	case 38400:
		prescalar = 78U;
		first_mod_reg = 2U;
		second_mod_reg = 0U;
		break;
	case 57600:
		prescalar = 52U;
		first_mod_reg = 1U;
		second_mod_reg = 37U;
		break;
	case 115200:
		prescalar = 26U;
		first_mod_reg = 0U;
		second_mod_reg = 111U;
		break;
	case 230400:
		prescalar = 13U;
		first_mod_reg = 0U;
		second_mod_reg = 37U;
		break;
	case 460800:
		prescalar = 6U;
		first_mod_reg = 8U;
		second_mod_reg = 32U;
		break;
	default:
		return -EINVAL;
	}

	config->clockPrescalar = prescalar;
	config->firstModReg = first_mod_reg;
	config->secondModReg = second_mod_reg;

	return 0;
}

static int uart_msp432p4xx_init(struct device *dev)
{
	int err;
	const struct uart_device_config *config = DEV_CFG(dev);
	eUSCI_UART_Config UartConfig;

	/* Select P1.2 and P1.3 in UART mode */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
		(GPIO_PIN2 | GPIO_PIN3), GPIO_PRIMARY_MODULE_FUNCTION);

	UartConfig.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
	UartConfig.parity = EUSCI_A_UART_NO_PARITY;
	UartConfig.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
	UartConfig.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
	UartConfig.uartMode = EUSCI_A_UART_MODE;
	UartConfig.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

	/* Baud rate settings calculated for 48MHz */
	err = baudrate_set(&UartConfig, DT_UART_MSP432P4XX_BAUD_RATE);
	if (err) {
		return err;
	}
	/* Configure UART Module */
	MAP_UART_initModule((unsigned long)config->base, &UartConfig);

	/* Enable UART module */
	MAP_UART_enableModule((unsigned long)config->base);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	IRQ_CONNECT(DT_TI_MSP432P4XX_UART_40001000_IRQ_0,
			DT_TI_MSP432P4XX_UART_40001000_IRQ_0_PRIORITY,
			uart_msp432p4xx_isr, DEVICE_GET(uart_msp432p4xx_0),
			0);
	irq_enable(DT_TI_MSP432P4XX_UART_40001000_IRQ_0);

#endif
	return 0;
}

static int uart_msp432p4xx_poll_in(struct device *dev, unsigned char *c)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	*c = MAP_UART_receiveData((unsigned long)config->base);

	return 0;
}

static void uart_msp432p4xx_poll_out(struct device *dev,
				     unsigned char c)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UART_transmitData((unsigned long)config->base, c);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_msp432p4xx_fifo_fill(struct device *dev,
						const u8_t *tx_data, int size)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int num_tx = 0U;

	while ((size - num_tx) > 0) {
		MAP_UART_transmitData((unsigned long)config->base,
							tx_data[num_tx]);
		if (MAP_UART_getInterruptStatus((unsigned long)config->base,
			EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT_FLAG)) {
			num_tx++;
		} else {
			break;
		}
	}

	return (int)num_tx;
}

static int uart_msp432p4xx_fifo_read(struct device *dev, u8_t *rx_data,
							const int size)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int num_rx = 0U;

	while (((size - num_rx) > 0) &&
		MAP_UART_getInterruptStatus((unsigned long)config->base,
					EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)) {

		rx_data[num_rx++] =
			MAP_UART_receiveData((unsigned long)config->base);
	}

	return num_rx;
}

static void uart_msp432p4xx_irq_tx_enable(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UART_enableInterrupt((unsigned long)config->base,
					EUSCI_A_UART_TRANSMIT_INTERRUPT);
}

static void uart_msp432p4xx_irq_tx_disable(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UART_disableInterrupt((unsigned long)config->base,
					EUSCI_A_UART_TRANSMIT_INTERRUPT);
}

static int uart_msp432p4xx_irq_tx_ready(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int int_status;

	int_status =  MAP_UART_getInterruptStatus((unsigned long)config->base,
					EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);

	return (int_status & EUSCI_A_IE_TXIE);
}

static void uart_msp432p4xx_irq_rx_enable(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UART_enableInterrupt((unsigned long)config->base,
				EUSCI_A_UART_RECEIVE_INTERRUPT);
}

static void uart_msp432p4xx_irq_rx_disable(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	MAP_UART_disableInterrupt((unsigned long)config->base,
				EUSCI_A_UART_RECEIVE_INTERRUPT);
}

static int uart_msp432p4xx_irq_tx_complete(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);

	return MAP_UART_getInterruptStatus((unsigned long)config->base,
				EUSCI_A_UART_TRANSMIT_COMPLETE_INTERRUPT_FLAG);
}

static int uart_msp432p4xx_irq_rx_ready(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int int_status;

	int_status = MAP_UART_getInterruptStatus((unsigned long)config->base,
					EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);

	return (int_status & EUSCI_A_IE_RXIE);
}

static void uart_msp432p4xx_irq_err_enable(struct device *dev)
{
	/* Not yet used in zephyr */
}

static void uart_msp432p4xx_irq_err_disable(struct device *dev)
{
	/* Not yet used in zephyr */
}

static int uart_msp432p4xx_irq_is_pending(struct device *dev)
{
	const struct uart_device_config *config = DEV_CFG(dev);
	unsigned int int_status;

	int_status = MAP_UART_getEnabledInterruptStatus(
						(unsigned long)config->base);

	return (int_status & (EUSCI_A_IE_TXIE | EUSCI_A_IE_RXIE));
}

static int uart_msp432p4xx_irq_update(struct device *dev)
{
	return 1;
}

static void uart_msp432p4xx_irq_callback_set(struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_msp432p4xx_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uart_msp432p4xx_isr(void *arg)
{
	struct device *dev = arg;
	const struct uart_device_config *config = DEV_CFG(dev);
	struct uart_msp432p4xx_dev_data_t * const dev_data = DEV_DATA(dev);
	unsigned int int_status;

	int_status = MAP_UART_getEnabledInterruptStatus(
						(unsigned long)config->base);

	if (dev_data->cb) {
		dev_data->cb(dev_data->cb_data);
	}
	/*
	 * Clear interrupts only after cb called, as Zephyr UART clients expect
	 * to check interrupt status during the callback.
	 */
	MAP_UART_disableInterrupt((unsigned long)config->base, int_status);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_msp432p4xx_driver_api = {
	.poll_in = uart_msp432p4xx_poll_in,
	.poll_out = uart_msp432p4xx_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill	  = uart_msp432p4xx_fifo_fill,
	.fifo_read	  = uart_msp432p4xx_fifo_read,
	.irq_tx_enable	  = uart_msp432p4xx_irq_tx_enable,
	.irq_tx_disable	  = uart_msp432p4xx_irq_tx_disable,
	.irq_tx_ready	  = uart_msp432p4xx_irq_tx_ready,
	.irq_rx_enable	  = uart_msp432p4xx_irq_rx_enable,
	.irq_rx_disable	  = uart_msp432p4xx_irq_rx_disable,
	.irq_tx_complete  = uart_msp432p4xx_irq_tx_complete,
	.irq_rx_ready	  = uart_msp432p4xx_irq_rx_ready,
	.irq_err_enable	  = uart_msp432p4xx_irq_err_enable,
	.irq_err_disable  = uart_msp432p4xx_irq_err_disable,
	.irq_is_pending	  = uart_msp432p4xx_irq_is_pending,
	.irq_update	  = uart_msp432p4xx_irq_update,
	.irq_callback_set = uart_msp432p4xx_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

DEVICE_AND_API_INIT(uart_msp432p4xx_0, DT_UART_MSP432P4XX_NAME,
			uart_msp432p4xx_init, &uart_msp432p4xx_dev_data_0,
			&uart_msp432p4xx_dev_cfg_0,
			PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			(void *)&uart_msp432p4xx_driver_api);
