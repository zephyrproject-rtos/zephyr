/*
 * Copyright (c) 2018 Foundries.io
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_lpuart

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_lpuart.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>

struct rv32m1_lpuart_config {
	LPUART_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	clock_ip_name_t clock_ip_name;
	uint32_t clock_ip_src;
	uint32_t baud_rate;
	uint8_t hw_flow_control;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
	const struct pinctrl_dev_config *pincfg;
};

struct rv32m1_lpuart_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int rv32m1_lpuart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int ret = -1;

	if (flags & kLPUART_RxDataRegFullFlag) {
		*c = LPUART_ReadByte(config->base);
		ret = 0;
	}

	return ret;
}

static void rv32m1_lpuart_poll_out(const struct device *dev, unsigned char c)
{
	const struct rv32m1_lpuart_config *config = dev->config;

	while (!(LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {
	}

	LPUART_WriteByte(config->base, c);
}

static int rv32m1_lpuart_err_check(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);
	int err = 0;

	if (flags & kLPUART_RxOverrunFlag) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & kLPUART_ParityErrorFlag) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & kLPUART_FramingErrorFlag) {
		err |= UART_ERROR_FRAMING;
	}

	LPUART_ClearStatusFlags(config->base, kLPUART_RxOverrunFlag |
					      kLPUART_ParityErrorFlag |
					      kLPUART_FramingErrorFlag);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int rv32m1_lpuart_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data,
				   int len)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	int num_tx = 0U;

	while ((len - num_tx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_TxDataRegEmptyFlag)) {

		LPUART_WriteByte(config->base, tx_data[num_tx++]);
	}

	return num_tx;
}

static int rv32m1_lpuart_fifo_read(const struct device *dev, uint8_t *rx_data,
				   const int len)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	int num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (LPUART_GetStatusFlags(config->base)
		& kLPUART_RxDataRegFullFlag)) {

		rx_data[num_rx++] = LPUART_ReadByte(config->base);
	}

	return num_rx;
}

static void rv32m1_lpuart_irq_tx_enable(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void rv32m1_lpuart_irq_tx_disable(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int rv32m1_lpuart_irq_tx_complete(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_TxDataRegEmptyFlag) != 0U;
}

static int rv32m1_lpuart_irq_tx_ready(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_TxDataRegEmptyInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& rv32m1_lpuart_irq_tx_complete(dev);
}

static void rv32m1_lpuart_irq_rx_enable(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void rv32m1_lpuart_irq_rx_disable(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int rv32m1_lpuart_irq_rx_full(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t flags = LPUART_GetStatusFlags(config->base);

	return (flags & kLPUART_RxDataRegFullFlag) != 0U;
}

static int rv32m1_lpuart_irq_rx_pending(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_RxDataRegFullInterruptEnable;

	return (LPUART_GetEnabledInterrupts(config->base) & mask)
		&& rv32m1_lpuart_irq_rx_full(dev);
}

static void rv32m1_lpuart_irq_err_enable(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_EnableInterrupts(config->base, mask);
}

static void rv32m1_lpuart_irq_err_disable(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	uint32_t mask = kLPUART_NoiseErrorInterruptEnable |
			kLPUART_FramingErrorInterruptEnable |
			kLPUART_ParityErrorInterruptEnable;

	LPUART_DisableInterrupts(config->base, mask);
}

static int rv32m1_lpuart_irq_is_pending(const struct device *dev)
{
	return (rv32m1_lpuart_irq_tx_ready(dev)
		|| rv32m1_lpuart_irq_rx_pending(dev));
}

static int rv32m1_lpuart_irq_update(const struct device *dev)
{
	return 1;
}

static void rv32m1_lpuart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
	struct rv32m1_lpuart_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void rv32m1_lpuart_isr(const struct device *dev)
{
	struct rv32m1_lpuart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int rv32m1_lpuart_init(const struct device *dev)
{
	const struct rv32m1_lpuart_config *config = dev->config;
	lpuart_config_t uart_config;
	uint32_t clock_freq;
	int err;

	/* set clock source */
	/* TODO: Don't change if another core has configured */
	CLOCK_SetIpSrc(config->clock_ip_name, config->clock_ip_src);

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &clock_freq)) {
		return -EINVAL;
	}

	LPUART_GetDefaultConfig(&uart_config);
	uart_config.enableTx = true;
	uart_config.enableRx = true;
	if (config->hw_flow_control) {
		uart_config.enableRxRTS = true;
		uart_config.enableTxCTS = true;
	}
	uart_config.baudRate_Bps = config->baud_rate;

	LPUART_Init(config->base, &uart_config, clock_freq);

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

static DEVICE_API(uart, rv32m1_lpuart_driver_api) = {
	.poll_in = rv32m1_lpuart_poll_in,
	.poll_out = rv32m1_lpuart_poll_out,
	.err_check = rv32m1_lpuart_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = rv32m1_lpuart_fifo_fill,
	.fifo_read = rv32m1_lpuart_fifo_read,
	.irq_tx_enable = rv32m1_lpuart_irq_tx_enable,
	.irq_tx_disable = rv32m1_lpuart_irq_tx_disable,
	.irq_tx_complete = rv32m1_lpuart_irq_tx_complete,
	.irq_tx_ready = rv32m1_lpuart_irq_tx_ready,
	.irq_rx_enable = rv32m1_lpuart_irq_rx_enable,
	.irq_rx_disable = rv32m1_lpuart_irq_rx_disable,
	.irq_rx_ready = rv32m1_lpuart_irq_rx_full,
	.irq_err_enable = rv32m1_lpuart_irq_err_enable,
	.irq_err_disable = rv32m1_lpuart_irq_err_disable,
	.irq_is_pending = rv32m1_lpuart_irq_is_pending,
	.irq_update = rv32m1_lpuart_irq_update,
	.irq_callback_set = rv32m1_lpuart_irq_callback_set,
#endif
};

#define RV32M1_LPUART_DECLARE_CFG(n, IRQ_FUNC_INIT)			\
	static const struct rv32m1_lpuart_config rv32m1_lpuart_##n##_cfg = {\
		.base = (LPUART_Type *)DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),\
		.clock_ip_name = INST_DT_CLOCK_IP_NAME(n),		\
		.clock_ip_src = kCLOCK_IpSrcFircAsync,			\
		.baud_rate = DT_INST_PROP(n, current_speed),		\
		.hw_flow_control = DT_INST_PROP(n, hw_flow_control),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		IRQ_FUNC_INIT						\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define RV32M1_LPUART_CONFIG_FUNC(n)					\
	static void rv32m1_lpuart_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), 0, rv32m1_lpuart_isr,	\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define RV32M1_LPUART_IRQ_CFG_FUNC_INIT(n)				\
	.irq_config_func = rv32m1_lpuart_config_func_##n,
#define RV32M1_LPUART_INIT_CFG(n)					\
	RV32M1_LPUART_DECLARE_CFG(n, RV32M1_LPUART_IRQ_CFG_FUNC_INIT(n))
#else
#define RV32M1_LPUART_CONFIG_FUNC(n)
#define RV32M1_LPUART_IRQ_CFG_FUNC_INIT
#define RV32M1_LPUART_INIT_CFG(n)					\
	RV32M1_LPUART_DECLARE_CFG(n, RV32M1_LPUART_IRQ_CFG_FUNC_INIT)
#endif

#define RV32M1_LPUART_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static struct rv32m1_lpuart_data rv32m1_lpuart_##n##_data;	\
									\
	static const struct rv32m1_lpuart_config rv32m1_lpuart_##n##_cfg;\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    rv32m1_lpuart_init,				\
			    NULL,					\
			    &rv32m1_lpuart_##n##_data,			\
			    &rv32m1_lpuart_##n##_cfg,			\
			    PRE_KERNEL_1,				\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &rv32m1_lpuart_driver_api);			\
									\
	RV32M1_LPUART_CONFIG_FUNC(n)					\
									\
	RV32M1_LPUART_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(RV32M1_LPUART_INIT)
