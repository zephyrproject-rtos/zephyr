/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_usart

/** @file
 * @brief USART driver for LPC54XXX and LPC55xxx families.
 *
 * Note:
 * - The driver is implemented for only one device, multiple instances
 * will be implemented in the future.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_usart.h>
#include <soc.h>
#include <fsl_device_registers.h>
#include <zephyr/drivers/pinctrl.h>

struct mcux_flexcomm_config {
	USART_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t baud_rate;
	uint8_t parity;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
	const struct pinctrl_dev_config *pincfg;
};

struct mcux_flexcomm_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct uart_config uart_config;
#endif
};

static int mcux_flexcomm_poll_in(const struct device *dev, unsigned char *c)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t flags = USART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kUSART_RxFifoNotEmptyFlag) {
		*c = USART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void mcux_flexcomm_poll_out(const struct device *dev,
					     unsigned char c)
{
	const struct mcux_flexcomm_config *config = dev->config;

	/* Wait until space is available in TX FIFO */
	while (!(USART_GetStatusFlags(config->base) & kUSART_TxFifoEmptyFlag)) {
	}

	USART_WriteByte(config->base, c);
}

static int mcux_flexcomm_err_check(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t flags = USART_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kStatus_USART_RxRingBufferOverrun) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kStatus_USART_ParityError) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kStatus_USART_FramingError) {
		err |= UART_ERROR_FRAMING;
	}

	USART_ClearStatusFlags(config->base,
			       kStatus_USART_RxRingBufferOverrun |
			       kStatus_USART_ParityError |
			       kStatus_USART_FramingError);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int mcux_flexcomm_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data,
				   int len)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (USART_GetStatusFlags(config->base)
		& kUSART_TxFifoNotFullFlag)) {

		USART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int mcux_flexcomm_fifo_read(const struct device *dev, uint8_t *rx_data,
				   const int len)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (USART_GetStatusFlags(config->base)
		& kUSART_RxFifoNotEmptyFlag)) {

		rx_data[num_rx++] = USART_ReadByte(config->base);
	}

	return num_rx;
}

static void mcux_flexcomm_irq_tx_enable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_TxLevelInterruptEnable;

	USART_EnableInterrupts(config->base, mask);
}

static void mcux_flexcomm_irq_tx_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_TxLevelInterruptEnable;

	USART_DisableInterrupts(config->base, mask);
}

static int mcux_flexcomm_irq_tx_complete(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;

	return (config->base->STAT & USART_STAT_TXIDLE_MASK) != 0;
}

static int mcux_flexcomm_irq_tx_ready(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_TxLevelInterruptEnable;
	uint32_t flags = USART_GetStatusFlags(config->base);

	return (USART_GetEnabledInterrupts(config->base) & mask)
		&& (flags & kUSART_TxFifoEmptyFlag);
}

static void mcux_flexcomm_irq_rx_enable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_RxLevelInterruptEnable;

	USART_EnableInterrupts(config->base, mask);
}

static void mcux_flexcomm_irq_rx_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_RxLevelInterruptEnable;

	USART_DisableInterrupts(config->base, mask);
}

static int mcux_flexcomm_irq_rx_full(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t flags = USART_GetStatusFlags(config->base);

	return (flags & kUSART_RxFifoNotEmptyFlag) != 0U;
}

static int mcux_flexcomm_irq_rx_pending(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kUSART_RxLevelInterruptEnable;

	return (USART_GetEnabledInterrupts(config->base) & mask)
		&& mcux_flexcomm_irq_rx_full(dev);
}

static void mcux_flexcomm_irq_err_enable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kStatus_USART_NoiseError |
			kStatus_USART_FramingError |
			kStatus_USART_ParityError;

	USART_EnableInterrupts(config->base, mask);
}

static void mcux_flexcomm_irq_err_disable(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
	uint32_t mask = kStatus_USART_NoiseError |
			kStatus_USART_FramingError |
			kStatus_USART_ParityError;

	USART_DisableInterrupts(config->base, mask);
}

static int mcux_flexcomm_irq_is_pending(const struct device *dev)
{
	return (mcux_flexcomm_irq_tx_ready(dev)
		|| mcux_flexcomm_irq_rx_pending(dev));
}

static int mcux_flexcomm_irq_update(const struct device *dev)
{
	return 1;
}

static void mcux_flexcomm_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
	struct mcux_flexcomm_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void mcux_flexcomm_isr(const struct device *dev)
{
	struct mcux_flexcomm_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int mcux_flexcomm_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct mcux_flexcomm_config *config = dev->config;
	struct mcux_flexcomm_data *data = dev->data;
	struct uart_config *uart_config = &data->uart_config;
	usart_config_t usart_config;
	usart_parity_mode_t parity_mode;
	usart_stop_bit_count_t stop_bits;
	usart_data_len_t data_bits = kUSART_8BitsPerChar;
	bool nine_bit_mode = false;
	uint32_t clock_freq;

	/* Set up structure to reconfigure UART */
	USART_GetDefaultConfig(&usart_config);

	/* Set parity */
	if (cfg->parity == UART_CFG_PARITY_ODD) {
		parity_mode = kUSART_ParityOdd;
	} else if (cfg->parity == UART_CFG_PARITY_EVEN) {
		parity_mode = kUSART_ParityEven;
	} else if (cfg->parity == UART_CFG_PARITY_NONE) {
		parity_mode = kUSART_ParityDisabled;
	} else {
		return -ENOTSUP;
	}
	usart_config.parityMode = parity_mode;

	/* Set baudrate */
	usart_config.baudRate_Bps = cfg->baudrate;

	/* Set stop bits */
	if (cfg->stop_bits == UART_CFG_STOP_BITS_1) {
		stop_bits = kUSART_OneStopBit;
	} else if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		stop_bits = kUSART_TwoStopBit;
	} else {
		return -ENOTSUP;
	}
	usart_config.stopBitCount = stop_bits;

	/* Set data bits */
	if (cfg->data_bits == UART_CFG_DATA_BITS_5 ||
	    cfg->data_bits == UART_CFG_DATA_BITS_6) {
		return -ENOTSUP;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_7) {
		data_bits = kUSART_7BitsPerChar;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_8) {
		data_bits = kUSART_8BitsPerChar;
	} else if (cfg->data_bits == UART_CFG_DATA_BITS_9) {
		nine_bit_mode = true;
	} else {
		return -EINVAL;
	}
	usart_config.bitCountPerChar = data_bits;

	/* Set flow control */
	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		usart_config.enableHardwareFlowControl = false;
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		usart_config.enableHardwareFlowControl = true;
	} else {
		return -ENOTSUP;
	}

	/* Wait for USART to finish transmission and turn off */
	USART_Deinit(config->base);

	/* Get UART clock frequency */
	clock_control_get_rate(config->clock_dev,
		config->clock_subsys, &clock_freq);

	/* Handle 9 bit mode */
	USART_Enable9bitMode(config->base, nine_bit_mode);

	/* Reconfigure UART */
	USART_Init(config->base, &usart_config, clock_freq);

	/* Update driver device data */
	uart_config->parity = cfg->parity;
	uart_config->baudrate = cfg->baudrate;
	uart_config->stop_bits = cfg->stop_bits;
	uart_config->data_bits = cfg->data_bits;
	uart_config->flow_ctrl = cfg->flow_ctrl;

	return 0;
}

static int mcux_flexcomm_uart_config_get(const struct device *dev,
					struct uart_config *cfg)
{
	struct mcux_flexcomm_data *data = dev->data;
	*cfg = data->uart_config;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int mcux_flexcomm_init(const struct device *dev)
{
	const struct mcux_flexcomm_config *config = dev->config;
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	struct mcux_flexcomm_data *data = dev->data;
	struct uart_config *cfg = &data->uart_config;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
	usart_config_t usart_config;
	usart_parity_mode_t parity_mode;
	uint32_t clock_freq;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	/* Get the clock frequency */
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	if (config->parity == UART_CFG_PARITY_ODD) {
		parity_mode = kUSART_ParityOdd;
	} else if (config->parity == UART_CFG_PARITY_EVEN) {
		parity_mode = kUSART_ParityEven;
	} else {
		parity_mode = kUSART_ParityDisabled;
	}

	USART_GetDefaultConfig(&usart_config);
	usart_config.enableTx = true;
	usart_config.enableRx = true;
	usart_config.parityMode = parity_mode;
	usart_config.baudRate_Bps = config->baud_rate;

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	cfg->baudrate = config->baud_rate;
	cfg->parity = config->parity;
	 /* From USART_GetDefaultConfig */
	cfg->stop_bits = UART_CFG_STOP_BITS_1;
	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

	USART_Init(config->base, &usart_config, clock_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static const struct uart_driver_api mcux_flexcomm_driver_api = {
	.poll_in = mcux_flexcomm_poll_in,
	.poll_out = mcux_flexcomm_poll_out,
	.err_check = mcux_flexcomm_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = mcux_flexcomm_uart_configure,
	.config_get = mcux_flexcomm_uart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mcux_flexcomm_fifo_fill,
	.fifo_read = mcux_flexcomm_fifo_read,
	.irq_tx_enable = mcux_flexcomm_irq_tx_enable,
	.irq_tx_disable = mcux_flexcomm_irq_tx_disable,
	.irq_tx_complete = mcux_flexcomm_irq_tx_complete,
	.irq_tx_ready = mcux_flexcomm_irq_tx_ready,
	.irq_rx_enable = mcux_flexcomm_irq_rx_enable,
	.irq_rx_disable = mcux_flexcomm_irq_rx_disable,
	.irq_rx_ready = mcux_flexcomm_irq_rx_full,
	.irq_err_enable = mcux_flexcomm_irq_err_enable,
	.irq_err_disable = mcux_flexcomm_irq_err_disable,
	.irq_is_pending = mcux_flexcomm_irq_is_pending,
	.irq_update = mcux_flexcomm_irq_update,
	.irq_callback_set = mcux_flexcomm_irq_callback_set,
#endif
};


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_MCUX_FLEXCOMM_CONFIG_FUNC(n)				\
	static void mcux_flexcomm_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mcux_flexcomm_isr, DEVICE_DT_INST_GET(n), 0);\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = mcux_flexcomm_config_func_##n
#define UART_MCUX_FLEXCOMM_INIT_CFG(n)					\
	UART_MCUX_FLEXCOMM_DECLARE_CFG(n,				\
				       UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_MCUX_FLEXCOMM_CONFIG_FUNC(n)
#define UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT
#define UART_MCUX_FLEXCOMM_INIT_CFG(n)					\
	UART_MCUX_FLEXCOMM_DECLARE_CFG(n, UART_MCUX_FLEXCOMM_IRQ_CFG_FUNC_INIT)
#endif

#define UART_MCUX_FLEXCOMM_DECLARE_CFG(n, IRQ_FUNC_INIT)		\
static const struct mcux_flexcomm_config mcux_flexcomm_##n##_config = {	\
	.base = (USART_Type *)DT_INST_REG_ADDR(n),			\
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
	.clock_subsys =				\
	(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
	.baud_rate = DT_INST_PROP(n, current_speed),			\
	.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE), \
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	IRQ_FUNC_INIT							\
}

#define UART_MCUX_FLEXCOMM_INIT(n)					\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static struct mcux_flexcomm_data mcux_flexcomm_##n##_data;	\
									\
	static const struct mcux_flexcomm_config mcux_flexcomm_##n##_config;\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    &mcux_flexcomm_init,			\
			    NULL,					\
			    &mcux_flexcomm_##n##_data,			\
			    &mcux_flexcomm_##n##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &mcux_flexcomm_driver_api);			\
									\
	UART_MCUX_FLEXCOMM_CONFIG_FUNC(n)				\
									\
	UART_MCUX_FLEXCOMM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_MCUX_FLEXCOMM_INIT)
