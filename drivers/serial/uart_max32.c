/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>

#include <wrap_max32_uart.h>

#define DT_DRV_COMPAT adi_max32_uart

LOG_MODULE_REGISTER(uart_max32, CONFIG_UART_LOG_LEVEL);

struct max32_uart_config {
	mxc_uart_regs_t *regs;
	const struct pinctrl_dev_config *pctrl;
	const struct device *clock;
	struct max32_perclk perclk;
	struct uart_config uart_conf;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct max32_uart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /* Interrupt callback */
	void *cb_data;                    /* Interrupt callback arg */
	uint32_t flags;                   /* Cached interrupt flags */
	uint32_t status;                  /* Cached status flags */
#endif
	struct uart_config conf; /* baudrate, stopbits, ... */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_max32_isr(const struct device *dev);
#endif

static void api_poll_out(const struct device *dev, unsigned char c)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_WriteCharacter(cfg->regs, c);
}

static int api_poll_in(const struct device *dev, unsigned char *c)
{
	int val;
	const struct max32_uart_config *cfg = dev->config;

	val = MXC_UART_ReadCharacterRaw(cfg->regs);
	if (val >= 0) {
		*c = (unsigned char)val;
	} else {
		return -1;
	}

	return 0;
}

static int api_err_check(const struct device *dev)
{
	int err = 0;
	uint32_t flags;
	const struct max32_uart_config *cfg = dev->config;

	flags = MXC_UART_GetFlags(cfg->regs);

	if (flags & ADI_MAX32_UART_ERROR_FRAMING) {
		err |= UART_ERROR_FRAMING;
	}

	if (flags & ADI_MAX32_UART_ERROR_PARITY) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & ADI_MAX32_UART_ERROR_OVERRUN) {
		err |= UART_ERROR_OVERRUN;
	}

	return err;
}

static int api_configure(const struct device *dev, const struct uart_config *uart_cfg)
{
	int err;
	const struct max32_uart_config *const cfg = dev->config;
	mxc_uart_regs_t *regs = cfg->regs;
	struct max32_uart_data *data = dev->data;

	/*
	 *  Set parity
	 */
	if (data->conf.parity != uart_cfg->parity) {
		mxc_uart_parity_t mxc_parity;

		switch (uart_cfg->parity) {
		case UART_CFG_PARITY_NONE:
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_NONE;
			break;
		case UART_CFG_PARITY_ODD:
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_ODD;
			break;
		case UART_CFG_PARITY_EVEN:
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_EVEN;
			break;
		case UART_CFG_PARITY_MARK:
#if defined(ADI_MAX32_UART_CFG_PARITY_MARK)
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_MARK;
			break;
#else
			return -ENOTSUP;
#endif
		case UART_CFG_PARITY_SPACE:
#if defined(ADI_MAX32_UART_CFG_PARITY_SPACE)
			mxc_parity = ADI_MAX32_UART_CFG_PARITY_SPACE;
			break;
#else
			return -ENOTSUP;
#endif
		default:
			return -EINVAL;
		}

		err = MXC_UART_SetParity(regs, mxc_parity);
		if (err < 0) {
			return -ENOTSUP;
		}
		/* incase of success keep configuration */
		data->conf.parity = uart_cfg->parity;
	}

	/*
	 *  Set stop bit
	 */
	if (data->conf.stop_bits != uart_cfg->stop_bits) {
		if (uart_cfg->stop_bits == UART_CFG_STOP_BITS_1) {
			err = MXC_UART_SetStopBits(regs, MXC_UART_STOP_1);
		} else if (uart_cfg->stop_bits == UART_CFG_STOP_BITS_2) {
			err = MXC_UART_SetStopBits(regs, MXC_UART_STOP_2);
		} else {
			return -ENOTSUP;
		}
		if (err < 0) {
			return -ENOTSUP;
		}
		/* incase of success keep configuration */
		data->conf.stop_bits = uart_cfg->stop_bits;
	}

	/*
	 *  Set data bit
	 *  Valid data for MAX32  is 5-6-7-8
	 *  Valid data for Zepyhr is 0-1-2-3
	 *  Added +5 to index match.
	 */
	if (data->conf.data_bits != uart_cfg->data_bits) {
		err = MXC_UART_SetDataSize(regs, (5 + uart_cfg->data_bits));
		if (err < 0) {
			return -ENOTSUP;
		}
		/* incase of success keep configuration */
		data->conf.data_bits = uart_cfg->data_bits;
	}

	/*
	 *  Set flow control
	 *  Flow control not implemented yet so that only support no flow mode
	 */
	if (data->conf.flow_ctrl != uart_cfg->flow_ctrl) {
		if (uart_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
			return -ENOTSUP;
		}
		data->conf.flow_ctrl = uart_cfg->flow_ctrl;
	}

	/*
	 *  Set baudrate
	 */
	if (data->conf.baudrate != uart_cfg->baudrate) {
		err = Wrap_MXC_UART_SetFrequency(regs, uart_cfg->baudrate, cfg->perclk.clk_src);
		if (err < 0) {
			return -ENOTSUP;
		}
		/* In case of success keep configuration */
		data->conf.baudrate = uart_cfg->baudrate;
	}
	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int api_config_get(const struct device *dev, struct uart_config *uart_cfg)
{
	struct max32_uart_data *data = dev->data;

	/* copy configs from global setting */
	*uart_cfg = data->conf;

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_max32_init(const struct device *dev)
{
	int ret;
	const struct max32_uart_config *const cfg = dev->config;
	mxc_uart_regs_t *regs = cfg->regs;

	if (!device_is_ready(cfg->clock)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = MXC_UART_Shutdown(regs);
	if (ret) {
		return ret;
	}

	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret != 0) {
		LOG_ERR("Cannot enable UART clock");
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pctrl, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = api_configure(dev, &cfg->uart_conf);
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_UART_Init(regs);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Clear any pending UART RX/TX interrupts */
	MXC_UART_ClearFlags(regs, (ADI_MAX32_UART_INT_RX | ADI_MAX32_UART_INT_TX));
	cfg->irq_config_func(dev);
#endif

	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int api_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	unsigned int num_tx = 0;
	const struct max32_uart_config *cfg = dev->config;

	num_tx = MXC_UART_WriteTXFIFO(cfg->regs, (unsigned char *)tx_data, size);

	return (int)num_tx;
}

static int api_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	unsigned int num_rx = 0;
	const struct max32_uart_config *cfg = dev->config;

	num_rx = MXC_UART_ReadRXFIFO(cfg->regs, (unsigned char *)rx_data, size);
	if (num_rx == 0) {
		MXC_UART_ClearFlags(cfg->regs, ADI_MAX32_UART_INT_RX);
	}

	return num_rx;
}

static void api_irq_tx_enable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;
	unsigned int key;

	MXC_UART_EnableInt(cfg->regs, ADI_MAX32_UART_INT_TX | ADI_MAX32_UART_INT_TX_OEM);

	key = irq_lock();
	uart_max32_isr(dev);
	irq_unlock(key);
}

static void api_irq_tx_disable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_DisableInt(cfg->regs, ADI_MAX32_UART_INT_TX | ADI_MAX32_UART_INT_TX_OEM);
}

static int api_irq_tx_ready(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;
	const struct max32_uart_config *cfg = dev->config;
	uint32_t inten = Wrap_MXC_UART_GetRegINTEN(cfg->regs);

	return ((inten & (ADI_MAX32_UART_INT_TX | ADI_MAX32_UART_INT_TX_OEM)) &&
		!(data->status & MXC_F_UART_STATUS_TX_FULL));
}

static void api_irq_rx_enable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_EnableInt(cfg->regs, ADI_MAX32_UART_INT_RX);
}

static void api_irq_rx_disable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_DisableInt(cfg->regs, ADI_MAX32_UART_INT_RX);
}

static int api_irq_tx_complete(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	if (MXC_UART_GetActive(cfg->regs) == E_BUSY) {
		return 0;
	} else {
		return 1; /* transmission completed */
	}
}

static int api_irq_rx_ready(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;
	const struct max32_uart_config *cfg = dev->config;
	uint32_t inten = Wrap_MXC_UART_GetRegINTEN(cfg->regs);

	return ((inten & ADI_MAX32_UART_INT_RX) && !(data->status & ADI_MAX32_UART_RX_EMPTY));
}

static void api_irq_err_enable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_EnableInt(cfg->regs, ADI_MAX32_UART_ERROR_INTERRUPTS);
}

static void api_irq_err_disable(const struct device *dev)
{
	const struct max32_uart_config *cfg = dev->config;

	MXC_UART_DisableInt(cfg->regs, ADI_MAX32_UART_ERROR_INTERRUPTS);
}

static int api_irq_is_pending(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;

	return (data->flags & (ADI_MAX32_UART_INT_RX | ADI_MAX32_UART_INT_TX));
}

static int api_irq_update(const struct device *dev)
{
	struct max32_uart_data *const data = dev->data;
	const struct max32_uart_config *const cfg = dev->config;

	data->flags = MXC_UART_GetFlags(cfg->regs);
	data->status = MXC_UART_GetStatus(cfg->regs);

	MXC_UART_ClearFlags(cfg->regs, data->flags);

	return 1;
}

static void api_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				 void *cb_data)
{
	struct max32_uart_data *const data = dev->data;

	data->cb = cb;
	data->cb_data = cb_data;
}

static void uart_max32_isr(const struct device *dev)
{
	struct max32_uart_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_max32_driver_api) = {
	.poll_in = api_poll_in,
	.poll_out = api_poll_out,
	.err_check = api_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = api_configure,
	.config_get = api_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = api_fifo_fill,
	.fifo_read = api_fifo_read,
	.irq_tx_enable = api_irq_tx_enable,
	.irq_tx_disable = api_irq_tx_disable,
	.irq_tx_ready = api_irq_tx_ready,
	.irq_rx_enable = api_irq_rx_enable,
	.irq_rx_disable = api_irq_rx_disable,
	.irq_tx_complete = api_irq_tx_complete,
	.irq_rx_ready = api_irq_rx_ready,
	.irq_err_enable = api_irq_err_enable,
	.irq_err_disable = api_irq_err_disable,
	.irq_is_pending = api_irq_is_pending,
	.irq_update = api_irq_update,
	.irq_callback_set = api_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define MAX32_UART_INIT(_num)                                                                      \
	PINCTRL_DT_INST_DEFINE(_num);                                                              \
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                                   \
		   (static void uart_max32_irq_init_##_num(const struct device *dev) \
		   {             \
			   IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority),            \
				       uart_max32_isr, DEVICE_DT_INST_GET(_num), 0);               \
			   irq_enable(DT_INST_IRQN(_num));                                         \
		   }));                                                                            \
	static const struct max32_uart_config max32_uart_config_##_num = {                         \
		.regs = (mxc_uart_regs_t *)DT_INST_REG_ADDR(_num),                                 \
		.pctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(_num),                                     \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(_num)),                                 \
		.perclk.bus = DT_INST_CLOCKS_CELL(_num, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(_num, bit),                                      \
		.perclk.clk_src =                                                                  \
			DT_INST_PROP_OR(_num, clock_source, ADI_MAX32_PRPH_CLK_SRC_PCLK),          \
		.uart_conf.baudrate = DT_INST_PROP_OR(_num, current_speed, 115200),                \
		.uart_conf.parity = DT_INST_ENUM_IDX_OR(_num, parity, UART_CFG_PARITY_NONE),       \
		.uart_conf.data_bits = DT_INST_ENUM_IDX_OR(_num, data_bits, UART_CFG_DATA_BITS_8), \
		.uart_conf.stop_bits = DT_INST_ENUM_IDX_OR(_num, stop_bits, UART_CFG_STOP_BITS_1), \
		.uart_conf.flow_ctrl =                                                             \
			DT_INST_PROP_OR(_num, hw_flow_control, UART_CFG_FLOW_CTRL_NONE),           \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			   (.irq_config_func = uart_max32_irq_init_##_num,))};                     \
	static struct max32_uart_data max32_uart_data##_num = {                                    \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (.cb = NULL,))};                          \
	DEVICE_DT_INST_DEFINE(_num, uart_max32_init, NULL, &max32_uart_data##_num,                 \
			      &max32_uart_config_##_num, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, (void *)&uart_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX32_UART_INIT)
