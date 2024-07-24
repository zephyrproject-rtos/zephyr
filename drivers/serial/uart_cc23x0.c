/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <errno.h>

#include <driverlib/uart.h>
#include <driverlib/clkctl.h>

struct uart_cc23x0_config {
	uint32_t reg;
	uint32_t sys_clk_freq;
	const struct pinctrl_dev_config *pcfg;
};

struct uart_cc23x0_data {
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_cc23x0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_cc23x0_config *config = dev->config;

	if (!UARTCharAvailable(config->reg)) {
		return -1;
	}

	*c = UARTGetCharNonBlocking(config->reg);

	return 0;
}

static void uart_cc23x0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTPutChar(config->reg, c);
}

static int uart_cc23x0_err_check(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;
	uint32_t flags = UARTGetRxError(config->reg);
	int error = 0;

	error |= (flags & UART_RXERROR_FRAMING) ? UART_ERROR_FRAMING : 0;
	error |= (flags & UART_RXERROR_PARITY) ? UART_ERROR_PARITY : 0;
	error |= (flags & UART_RXERROR_BREAK) ? UART_BREAK : 0;
	error |= (flags & UART_RXERROR_OVERRUN) ? UART_ERROR_OVERRUN : 0;

	UARTClearRxError(config->reg);

	return error;
}

static int uart_cc23x0_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_cc23x0_config *config = dev->config;
	struct uart_cc23x0_data *data = dev->data;
	uint32_t line_ctrl = 0;
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
	UARTConfigSetExpClk(config->reg, config->sys_clk_freq, cfg->baudrate, line_ctrl);

	if (flow_ctrl) {
		UARTEnableCTS(config->reg);
		UARTEnableRTS(config->reg);
	} else {
		UARTDisableCTS(config->reg);
		UARTDisableRTS(config->reg);
	}

	/* Re-enable UART */
	UARTEnable(config->reg);

	/* Make use of the FIFO to reduce chances of data being lost */
	UARTEnableFifo(config->reg);

	data->uart_config = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_cc23x0_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_cc23x0_data *data = dev->data;

	*cfg = data->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_cc23x0_fifo_fill(const struct device *dev, const uint8_t *buf, int len)
{
	const struct uart_cc23x0_config *config = dev->config;
	int n = 0;

	while (n < len) {
		if (!UARTSpaceAvailable(config->reg)) {
			break;
		}
		UARTPutCharNonBlocking(config->reg, buf[n]);
		n++;
	}

	return n;
}

static int uart_cc23x0_fifo_read(const struct device *dev, uint8_t *buf, const int len)
{
	const struct uart_cc23x0_config *config = dev->config;
	int c, n;

	n = 0;
	while (n < len) {
		if (!UARTCharAvailable(config->reg)) {
			break;
		}
		c = UARTGetCharNonBlocking(config->reg);
		buf[n++] = c;
	}

	return n;
}

static void uart_cc23x0_irq_tx_enable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTEnableInt(config->reg, UART_INT_TX);
}

static void uart_cc23x0_irq_tx_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTDisableInt(config->reg, UART_INT_TX);
}

static int uart_cc23x0_irq_tx_ready(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTSpaceAvailable(config->reg) ? 1 : 0;
}

static void uart_cc23x0_irq_rx_enable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	/* Trigger the ISR on both RX and Receive Timeout. This is to allow
	 * the use of the hardware FIFOs for more efficient operation
	 */
	UARTEnableInt(config->reg, UART_INT_RX | UART_INT_RT);
}

static void uart_cc23x0_irq_rx_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	UARTDisableInt(config->reg, UART_INT_RX | UART_INT_RT);
}

static int uart_cc23x0_irq_tx_complete(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTBusy(config->reg) ? 0 : 1;
}

static int uart_cc23x0_irq_rx_ready(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTCharAvailable(config->reg) ? 1 : 0;
}

static void uart_cc23x0_irq_err_enable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTEnableInt(config->reg, UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE);
}

static void uart_cc23x0_irq_err_disable(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	return UARTDisableInt(config->reg, UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE);
}

static int uart_cc23x0_irq_is_pending(const struct device *dev)
{
	const struct uart_cc23x0_config *config = dev->config;

	/* Read masked interrupt status */
	uint32_t status = UARTIntStatus(config->reg, true);

	return status ? 1 : 0;
}

static int uart_cc23x0_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_cc23x0_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_cc23x0_data *data = dev->data;

	data->callback = cb;
	data->user_data = user_data;
}

static void uart_cc23x0_isr(const struct device *dev)
{
	struct uart_cc23x0_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_cc23x0_driver_api = {
	.poll_in = uart_cc23x0_poll_in,
	.poll_out = uart_cc23x0_poll_out,
	.err_check = uart_cc23x0_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_cc23x0_configure,
	.config_get = uart_cc23x0_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_cc23x0_fifo_fill,
	.fifo_read = uart_cc23x0_fifo_read,
	.irq_tx_enable = uart_cc23x0_irq_tx_enable,
	.irq_tx_disable = uart_cc23x0_irq_tx_disable,
	.irq_tx_ready = uart_cc23x0_irq_tx_ready,
	.irq_rx_enable = uart_cc23x0_irq_rx_enable,
	.irq_rx_disable = uart_cc23x0_irq_rx_disable,
	.irq_tx_complete = uart_cc23x0_irq_tx_complete,
	.irq_rx_ready = uart_cc23x0_irq_rx_ready,
	.irq_err_enable = uart_cc23x0_irq_err_enable,
	.irq_err_disable = uart_cc23x0_irq_err_disable,
	.irq_is_pending = uart_cc23x0_irq_is_pending,
	.irq_update = uart_cc23x0_irq_update,
	.irq_callback_set = uart_cc23x0_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_CC23X0_IRQ_CFG(n)                                                                     \
	do {                                                                                       \
		UARTClearInt(config->reg, UART_INT_RX);                                            \
		UARTClearInt(config->reg, UART_INT_RT);                                            \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_cc23x0_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	} while (false)

#define UART_CC23X0_INT_FIELDS .callback = NULL, .user_data = NULL,
#else
#define UART_CC23X0_IRQ_CFG(n)
#define UART_CC23X0_INT_FIELDS
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_CC23X0_DEVICE_DEFINE(n)                                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_cc23x0_init_##n, NULL, &uart_cc23x0_data_##n,                \
			      &uart_cc23x0_config_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &uart_cc23x0_driver_api)

#define UART_CC23X0_INIT_FUNC(n)                                                                   \
	static int uart_cc23x0_init_##n(const struct device *dev)                                  \
	{                                                                                          \
		const struct uart_cc23x0_config *config = dev->config;                             \
		struct uart_cc23x0_data *data = dev->data;                                         \
		int ret;                                                                           \
                                                                                                   \
		CLKCTLEnable(CLKCTL_BASE, CLKCTL_UART0);                                           \
                                                                                                   \
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);                    \
		if (ret < 0) {                                                                     \
			return ret;                                                                \
		}                                                                                  \
                                                                                                   \
		/* Configure and enable UART */                                                    \
		ret = uart_cc23x0_configure(dev, &data->uart_config);                              \
                                                                                                   \
		/* Enable interrupts */                                                            \
		UART_CC23X0_IRQ_CFG(n);                                                            \
                                                                                                   \
		return ret;                                                                        \
	}

#define UART_CC23X0_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	UART_CC23X0_INIT_FUNC(n);                                                                  \
                                                                                                   \
	static struct uart_cc23x0_config uart_cc23x0_config_##n = {                                \
		.reg = DT_INST_REG_ADDR(n),                                                        \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		};                                                                                 \
                                                                                                   \
	static struct uart_cc23x0_data uart_cc23x0_data_##n = {                                    \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(n, current_speed),                        \
				.parity = DT_INST_ENUM_IDX(n, parity),                             \
				.stop_bits = DT_INST_ENUM_IDX(n, stop_bits),                       \
				.data_bits = DT_INST_ENUM_IDX(n, data_bits),                       \
				.flow_ctrl = DT_INST_PROP(n, hw_flow_control),                     \
			},                                                                         \
		UART_CC23X0_INT_FIELDS                                                             \
		};                                                                                 \
	UART_CC23X0_DEVICE_DEFINE(n);

DT_INST_FOREACH_STATUS_OKAY(UART_CC23X0_INIT)
