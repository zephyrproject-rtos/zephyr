/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <errno.h>
#include <sys/__assert.h>
#include <drivers/uart.h>

#include <driverlib/ioc.h>
#include <driverlib/prcm.h>
#include <driverlib/uart.h>

struct uart_cc13xx_cc26xx_data {
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_CC13XX_CC26XX_0
DEVICE_DECLARE(uart_cc13xx_cc26xx_0);
#endif /* CONFIG_UART_CC13XX_CC26XX_0 */

#ifdef CONFIG_UART_CC13XX_CC26XX_1
DEVICE_DECLARE(uart_cc13xx_cc26xx_1);
#endif /* CONFIG_UART_CC13XX_CC26XX_1 */

static inline struct uart_cc13xx_cc26xx_data *get_dev_data(struct device *dev)
{
	return dev->driver_data;
}

static inline const struct uart_device_config *get_dev_conf(struct device *dev)
{
	return dev->config->config_info;
}

static int uart_cc13xx_cc26xx_poll_in(struct device *dev, unsigned char *c)
{
	if (!UARTCharsAvail(get_dev_conf(dev)->regs)) {
		return -1;
	}

	*c = UARTCharGetNonBlocking(get_dev_conf(dev)->regs);

	return 0;
}

static void uart_cc13xx_cc26xx_poll_out(struct device *dev, unsigned char c)
{
	UARTCharPut(get_dev_conf(dev)->regs, c);
}

static int uart_cc13xx_cc26xx_err_check(struct device *dev)
{
	u32_t flags = UARTRxErrorGet(get_dev_conf(dev)->regs);

	int error = (flags & UART_RXERROR_FRAMING ? UART_ERROR_FRAMING : 0) |
		    (flags & UART_RXERROR_PARITY ? UART_ERROR_PARITY : 0) |
		    (flags & UART_RXERROR_BREAK ? UART_BREAK : 0) |
		    (flags & UART_RXERROR_OVERRUN ? UART_ERROR_OVERRUN : 0);

	UARTRxErrorClear(get_dev_conf(dev)->regs);

	return error;
}

static int uart_cc13xx_cc26xx_configure(struct device *dev,
					const struct uart_config *cfg)
{
	u32_t line_ctrl = 0;
	bool flow_ctrl;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		line_ctrl |= UART_CONFIG_PAR_NONE;
		break;
	case UART_CFG_PARITY_ODD:
		line_ctrl |= UART_CONFIG_PAR_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		line_ctrl |= UART_CONFIG_PAR_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		line_ctrl |= UART_CONFIG_PAR_ONE;
		break;
	case UART_CFG_PARITY_SPACE:
		line_ctrl |= UART_CONFIG_PAR_ZERO;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		line_ctrl |= UART_CONFIG_STOP_ONE;
		break;
	case UART_CFG_STOP_BITS_2:
		line_ctrl |= UART_CONFIG_STOP_TWO;
		break;
	case UART_CFG_STOP_BITS_0_5:
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		line_ctrl |= UART_CONFIG_WLEN_5;
		break;
	case UART_CFG_DATA_BITS_6:
		line_ctrl |= UART_CONFIG_WLEN_6;
		break;
	case UART_CFG_DATA_BITS_7:
		line_ctrl |= UART_CONFIG_WLEN_7;
		break;
	case UART_CFG_DATA_BITS_8:
		line_ctrl |= UART_CONFIG_WLEN_8;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		flow_ctrl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		flow_ctrl = true;
		break;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	/* Disables UART before setting control registers */
	UARTConfigSetExpClk(get_dev_conf(dev)->regs,
			    get_dev_conf(dev)->sys_clk_freq, cfg->baudrate,
			    line_ctrl);

	if (flow_ctrl) {
		UARTHwFlowControlEnable(get_dev_conf(dev)->regs);
	} else {
		UARTHwFlowControlDisable(get_dev_conf(dev)->regs);
	}

	/* Re-enable UART */
	UARTEnable(get_dev_conf(dev)->regs);

	/* Disabled FIFOs act as 1-byte-deep holding registers (character mode) */
	UARTFIFODisable(get_dev_conf(dev)->regs);

	get_dev_data(dev)->uart_config = *cfg;

	return 0;
}

static int uart_cc13xx_cc26xx_config_get(struct device *dev,
					 struct uart_config *cfg)
{
	*cfg = get_dev_data(dev)->uart_config;
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cc13xx_cc26xx_fifo_fill(struct device *dev, const u8_t *buf,
					int len)
{
	int n = 0;

	while (n < len) {
		if (!UARTCharPutNonBlocking(get_dev_conf(dev)->regs, buf[n])) {
			break;
		}
		n++;
	}

	return n;
}

static int uart_cc13xx_cc26xx_fifo_read(struct device *dev, u8_t *buf,
					const int len)
{
	int c, n;

	n = 0;
	while (n < len) {
		c = UARTCharGetNonBlocking(get_dev_conf(dev)->regs);
		if (c == -1) {
			break;
		}
		buf[n++] = c;
	}

	return n;
}

static void uart_cc13xx_cc26xx_irq_tx_enable(struct device *dev)
{
	UARTIntEnable(get_dev_conf(dev)->regs, UART_INT_TX);
}

static void uart_cc13xx_cc26xx_irq_tx_disable(struct device *dev)
{
	UARTIntDisable(get_dev_conf(dev)->regs, UART_INT_TX);
}

static int uart_cc13xx_cc26xx_irq_tx_ready(struct device *dev)
{
	return UARTSpaceAvail(get_dev_conf(dev)->regs) ? 1 : 0;
}

static void uart_cc13xx_cc26xx_irq_rx_enable(struct device *dev)
{
	UARTIntEnable(get_dev_conf(dev)->regs, UART_INT_RX);
}

static void uart_cc13xx_cc26xx_irq_rx_disable(struct device *dev)
{
	UARTIntDisable(get_dev_conf(dev)->regs, UART_INT_RX);
}

static int uart_cc13xx_cc26xx_irq_tx_complete(struct device *dev)
{
	return UARTBusy(get_dev_conf(dev)->regs) ? 0 : 1;
}

static int uart_cc13xx_cc26xx_irq_rx_ready(struct device *dev)
{
	return UARTCharsAvail(get_dev_conf(dev)->regs) ? 1 : 0;
}

static void uart_cc13xx_cc26xx_irq_err_enable(struct device *dev)
{
	return UARTIntEnable(get_dev_conf(dev)->regs,
			     UART_INT_OE | UART_INT_BE | UART_INT_PE |
				     UART_INT_FE);
}

static void uart_cc13xx_cc26xx_irq_err_disable(struct device *dev)
{
	return UARTIntDisable(get_dev_conf(dev)->regs,
			      UART_INT_OE | UART_INT_BE | UART_INT_PE |
				      UART_INT_FE);
}

static int uart_cc13xx_cc26xx_irq_is_pending(struct device *dev)
{
	u32_t status = UARTIntStatus(get_dev_conf(dev)->regs, true);

	return status & (UART_INT_TX | UART_INT_RX) ? 1 : 0;
}

static int uart_cc13xx_cc26xx_irq_update(struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_cc13xx_cc26xx_irq_callback_set(
	struct device *dev, uart_irq_callback_user_data_t cb, void *user_data)
{
	struct uart_cc13xx_cc26xx_data *data = get_dev_data(dev);

	data->callback = cb;
	data->user_data = user_data;
}

static void uart_cc13xx_cc26xx_isr(void *arg)
{
	struct uart_cc13xx_cc26xx_data *data = get_dev_data(arg);

	if (data->callback) {
		data->callback(data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_cc13xx_cc26xx_driver_api = {
	.poll_in = uart_cc13xx_cc26xx_poll_in,
	.poll_out = uart_cc13xx_cc26xx_poll_out,
	.err_check = uart_cc13xx_cc26xx_err_check,
	.configure = uart_cc13xx_cc26xx_configure,
	.config_get = uart_cc13xx_cc26xx_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cc13xx_cc26xx_fifo_fill,
	.fifo_read = uart_cc13xx_cc26xx_fifo_read,
	.irq_tx_enable = uart_cc13xx_cc26xx_irq_tx_enable,
	.irq_tx_disable = uart_cc13xx_cc26xx_irq_tx_disable,
	.irq_tx_ready = uart_cc13xx_cc26xx_irq_tx_ready,
	.irq_rx_enable = uart_cc13xx_cc26xx_irq_rx_enable,
	.irq_rx_disable = uart_cc13xx_cc26xx_irq_rx_disable,
	.irq_tx_complete = uart_cc13xx_cc26xx_irq_tx_complete,
	.irq_rx_ready = uart_cc13xx_cc26xx_irq_rx_ready,
	.irq_err_enable = uart_cc13xx_cc26xx_irq_err_enable,
	.irq_err_disable = uart_cc13xx_cc26xx_irq_err_disable,
	.irq_is_pending = uart_cc13xx_cc26xx_irq_is_pending,
	.irq_update = uart_cc13xx_cc26xx_irq_update,
	.irq_callback_set = uart_cc13xx_cc26xx_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_CC13XX_CC26XX_0
static int uart_cc13xx_cc26xx_init_0(struct device *dev)
{
	int ret;

	/* Enable UART power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_SERIAL);

	/* Enable UART peripherals */
	PRCMPeripheralRunEnable(PRCM_PERIPH_UART0);
	PRCMPeripheralSleepEnable(PRCM_PERIPH_UART0);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* UART should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_SERIAL) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	/* Configure IOC module to map UART signals to pins */
	IOCPortConfigureSet(DT_TI_CC13XX_CC26XX_UART_40001000_TX_PIN,
			    IOC_PORT_MCU_UART0_TX, IOC_STD_OUTPUT);
	IOCPortConfigureSet(DT_TI_CC13XX_CC26XX_UART_40001000_RX_PIN,
			    IOC_PORT_MCU_UART0_RX, IOC_STD_INPUT);

	/* Configure and enable UART */
	ret = uart_cc13xx_cc26xx_configure(dev,
					   &get_dev_data(dev)->uart_config);

	/* Enable interrupts */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	UARTIntClear(get_dev_conf(dev)->regs, UART_INT_RX);

	IRQ_CONNECT(DT_TI_CC13XX_CC26XX_UART_40001000_IRQ_0,
		    DT_TI_CC13XX_CC26XX_UART_40001000_IRQ_0_PRIORITY,
		    uart_cc13xx_cc26xx_isr, DEVICE_GET(uart_cc13xx_cc26xx_0),
		    0);
	irq_enable(DT_TI_CC13XX_CC26XX_UART_40001000_IRQ_0);

	/* Causes an initial TX ready interrupt when TX interrupt is enabled */
	UARTCharPutNonBlocking(get_dev_conf(dev)->regs, '\0');
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return ret;
}

static const struct uart_device_config uart_cc13xx_cc26xx_config_0 = {
	.regs = DT_TI_CC13XX_CC26XX_UART_40001000_BASE_ADDRESS,
	.sys_clk_freq = DT_TI_CC13XX_CC26XX_UART_40001000_CLOCKS_CLOCK_FREQUENCY,
};

static struct uart_cc13xx_cc26xx_data uart_cc13xx_cc26xx_data_0 = {
	.uart_config = {
		.baudrate = DT_TI_CC13XX_CC26XX_UART_40001000_CURRENT_SPEED,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	},
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.callback = NULL,
	.user_data = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

DEVICE_AND_API_INIT(uart_cc13xx_cc26xx_0,
		    DT_TI_CC13XX_CC26XX_UART_40001000_LABEL,
		    uart_cc13xx_cc26xx_init_0, &uart_cc13xx_cc26xx_data_0,
		    &uart_cc13xx_cc26xx_config_0, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cc13xx_cc26xx_driver_api);
#endif /* CONFIG_UART_CC13XX_CC26XX_0 */

#ifdef CONFIG_UART_CC13XX_CC26XX_1
static int uart_cc13xx_cc26xx_init_1(struct device *dev)
{
	int ret;

	/* Enable UART power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

	/* Enable UART peripherals */
	PRCMPeripheralRunEnable(PRCM_PERIPH_UART1);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* UART should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	/* Configure IOC module to map UART signals to pins */
	IOCPortConfigureSet(DT_TI_CC13XX_CC26XX_UART_4000B000_TX_PIN,
			    IOC_PORT_MCU_UART1_TX, IOC_STD_OUTPUT);
	IOCPortConfigureSet(DT_TI_CC13XX_CC26XX_UART_4000B000_RX_PIN,
			    IOC_PORT_MCU_UART1_RX, IOC_STD_INPUT);

	/* Configure and enable UART */
	ret = uart_cc13xx_cc26xx_configure(dev,
					   &get_dev_data(dev)->uart_config);

	/* Enable interrupts */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	UARTIntClear(get_dev_conf(dev)->regs, UART_INT_RX);

	IRQ_CONNECT(DT_TI_CC13XX_CC26XX_UART_4000B000_IRQ_0,
		    DT_TI_CC13XX_CC26XX_UART_4000B000_IRQ_0_PRIORITY,
		    uart_cc13xx_cc26xx_isr, DEVICE_GET(uart_cc13xx_cc26xx_1),
		    0);
	irq_enable(DT_TI_CC13XX_CC26XX_UART_4000B000_IRQ_0);

	/* Causes an initial TX ready interrupt when TX interrupt is enabled */
	UARTCharPutNonBlocking(get_dev_conf(dev)->regs, '\0');
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return ret;
}

static const struct uart_device_config uart_cc13xx_cc26xx_config_1 = {
	.regs = DT_TI_CC13XX_CC26XX_UART_4000B000_BASE_ADDRESS,
	.sys_clk_freq = DT_TI_CC13XX_CC26XX_UART_4000B000_CLOCKS_CLOCK_FREQUENCY,
};

static struct uart_cc13xx_cc26xx_data uart_cc13xx_cc26xx_data_1 = {
	.uart_config = {
		.baudrate = DT_TI_CC13XX_CC26XX_UART_4000B000_CURRENT_SPEED,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	},
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.callback = NULL,
	.user_data = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

DEVICE_AND_API_INIT(uart_cc13xx_cc26xx_1,
		    DT_TI_CC13XX_CC26XX_UART_4000B000_LABEL,
		    uart_cc13xx_cc26xx_init_1, &uart_cc13xx_cc26xx_data_1,
		    &uart_cc13xx_cc26xx_config_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_cc13xx_cc26xx_driver_api);
#endif /* CONFIG_UART_CC13XX_CC26XX_1 */
