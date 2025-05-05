/*
 * Copyright (c) 2021-2025 ATL Electronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_uart

/**
 * @brief UART driver for Bouffalo Lab MCU family.
 */
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/irq.h>
#include <soc.h>

#include <bflb_pinctrl.h>
#include <bflb_uart.h>
#include <bflb_glb.h>

#define UART_CTS_FLOWCONTROL_ENABLE (0)
#define UART_RTS_FLOWCONTROL_ENABLE (0)
#define UART_MSB_FIRST_ENABLE	    (0)
#define UART_DEFAULT_RTO_TIMEOUT    (255)
#define UART_CLOCK_DIV		    (0)

struct bflb_config {
	uint32_t *reg;
	const struct pinctrl_dev_config *pinctrl_cfg;
	uint32_t periph_id;
	UART_CFG_Type uart_cfg;
	UART_FifoCfg_Type fifo_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct bflb_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_bflb_init(const struct device *dev)
{
	const struct bflb_config *cfg = dev->config;

	pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_DEFAULT);

	GLB_Set_UART_CLK(1, HBN_UART_CLK_160M, UART_CLOCK_DIV);

	UART_IntMask(cfg->periph_id, UART_INT_ALL, 1);
	UART_Disable(cfg->periph_id, UART_TXRX);

	UART_Init(cfg->periph_id, (UART_CFG_Type *)&cfg->uart_cfg);
	UART_TxFreeRun(cfg->periph_id, 1);
	UART_SetRxTimeoutValue(cfg->periph_id, UART_DEFAULT_RTO_TIMEOUT);
	UART_FifoConfig(cfg->periph_id, (UART_FifoCfg_Type *)&cfg->fifo_cfg);
	UART_Enable(cfg->periph_id, UART_TXRX);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static int uart_bflb_poll_in(const struct device *dev, unsigned char *c)
{
	const struct bflb_config *cfg = dev->config;

	return UART_ReceiveData(cfg->periph_id, (uint8_t *)c, 1) ? 0 : -1;
}

static void uart_bflb_poll_out(const struct device *dev, unsigned char c)
{
	const struct bflb_config *cfg = dev->config;

	while (UART_GetTxFifoCount(cfg->periph_id) == 0) {
		;
	}

	(void)UART_SendData(cfg->periph_id, (uint8_t *)&c, 1);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_bflb_err_check(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t status = BL_RD_REG(cfg->reg, UART_INT_STS);
	uint32_t tmp = BL_RD_REG(cfg->reg, UART_INT_CLEAR);
	int errors = 0;

	if (status & BIT(UART_INT_RX_FER)) {
		tmp |= BIT(UART_INT_RX_FER);

		errors |= UART_ERROR_OVERRUN;
	}

	if (status & BIT(UART_INT_TX_FER)) {
		tmp |= BIT(UART_INT_TX_FER);

		errors |= UART_ERROR_OVERRUN;
	}

	if (status & BIT(UART_INT_PCE)) {
		tmp |= BIT(UART_INT_PCE);

		errors |= UART_ERROR_PARITY;
	}

	BL_WR_REG(cfg->reg, UART_INT_CLEAR, tmp);

	return errors;
}

int uart_bflb_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct bflb_config *const cfg = dev->config;
	uint8_t num_tx = 0U;

	while ((len - num_tx > 0) && (UART_GetTxFifoCount(cfg->periph_id) > 0)) {
		BL_WR_BYTE(cfg->reg + UART_FIFO_WDATA_OFFSET, tx_data[num_tx++]);
	}

	return num_tx;
}

int uart_bflb_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct bflb_config *const cfg = dev->config;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && (UART_GetRxFifoCount(cfg->periph_id) > 0)) {
		rx_data[num_rx++] = BL_RD_BYTE(cfg->reg + UART_FIFO_RDATA_OFFSET);
	}

	return num_rx;
}

void uart_bflb_irq_tx_enable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	UART_IntMask(cfg->periph_id, UART_INT_TX_FIFO_REQ, 1);
}

void uart_bflb_irq_tx_disable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	UART_IntMask(cfg->periph_id, UART_INT_TX_FIFO_REQ, 0);
}

int uart_bflb_irq_tx_ready(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t maskVal = BL_RD_REG(cfg->reg, UART_INT_MASK);

	return (UART_GetTxFifoCount(cfg->periph_id) > 0)
	       && BL_IS_REG_BIT_SET(maskVal, UART_CR_UTX_FIFO_MASK);
}

int uart_bflb_irq_tx_complete(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	return !UART_GetTxBusBusyStatus(cfg->periph_id);
}

void uart_bflb_irq_rx_enable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	UART_IntMask(cfg->periph_id, UART_INT_RX_FIFO_REQ, 1);
}

void uart_bflb_irq_rx_disable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	UART_IntMask(cfg->periph_id, UART_INT_RX_FIFO_REQ, 0);
}

int uart_bflb_irq_rx_ready(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	return UART_GetRxFifoCount(cfg->periph_id) > 0;
}

void uart_bflb_irq_err_enable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	UART_IntMask(cfg->periph_id, UART_INT_PCE
				   | UART_INT_TX_FER
				   | UART_INT_RX_FER, 1);
}

void uart_bflb_irq_err_disable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;

	UART_IntMask(cfg->periph_id, UART_INT_PCE
				   | UART_INT_TX_FER
				   | UART_INT_RX_FER, 0);
}

int uart_bflb_irq_is_pending(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = BL_RD_REG(cfg->reg, UART_INT_STS);
	uint32_t maskVal = BL_RD_REG(cfg->reg, UART_INT_MASK);

	return ((BL_IS_REG_BIT_SET(tmp,  UART_URX_FIFO_INT) &&
		 BL_IS_REG_BIT_SET(maskVal, UART_CR_URX_FIFO_MASK)) ||
		(BL_IS_REG_BIT_SET(tmp,  UART_UTX_FIFO_INT) &&
		 BL_IS_REG_BIT_SET(maskVal, UART_CR_UTX_FIFO_MASK)));
}

int uart_bflb_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

void uart_bflb_irq_callback_set(const struct device *dev,
			      uart_irq_callback_user_data_t cb,
			      void *user_data)
{
	struct bflb_data *const data = dev->data;

	data->user_cb = cb;
	data->user_data = user_data;
}

static void uart_bflb_isr(const struct device *dev)
{
	struct bflb_data *const data = dev->data;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_PM_DEVICE
static int uart_bflb_pm_control(const struct device *dev,
				enum pm_device_action action)
{
	const struct bflb_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		(void)pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_DEFAULT);
		UART_Enable(cfg->periph_id, UART_TXRX);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_SLEEP)) {
			return -ENOTSUP;
		}
		UART_Disable(cfg->periph_id, UART_TXRX);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct uart_driver_api uart_bflb_driver_api = {
	.poll_in = uart_bflb_poll_in,
	.poll_out = uart_bflb_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.err_check = uart_bflb_err_check,
	.fifo_fill = uart_bflb_fifo_fill,
	.fifo_read = uart_bflb_fifo_read,
	.irq_tx_enable = uart_bflb_irq_tx_enable,
	.irq_tx_disable = uart_bflb_irq_tx_disable,
	.irq_tx_ready = uart_bflb_irq_tx_ready,
	.irq_tx_complete = uart_bflb_irq_tx_complete,
	.irq_rx_enable = uart_bflb_irq_rx_enable,
	.irq_rx_disable = uart_bflb_irq_rx_disable,
	.irq_rx_ready = uart_bflb_irq_rx_ready,
	.irq_err_enable = uart_bflb_irq_err_enable,
	.irq_err_disable = uart_bflb_irq_err_disable,
	.irq_is_pending = uart_bflb_irq_is_pending,
	.irq_update = uart_bflb_irq_update,
	.irq_callback_set = uart_bflb_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define BFLB_UART_IRQ_HANDLER_DECL(n)						\
	static void uart_bflb_config_func_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    uart_bflb_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}
#define BFLB_UART_IRQ_HANDLER_FUNC(n)						\
	.irq_config_func = uart_bflb_config_func_##n,
#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define BFLB_UART_IRQ_HANDLER_DECL(n)
#define BFLB_UART_IRQ_HANDLER_FUNC(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define BFLB_UART_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	PM_DEVICE_DT_INST_DEFINE(n, uart_bflb_pm_control);			\
	BFLB_UART_IRQ_HANDLER_DECL(n);						\
										\
	static struct bflb_data bflb_uart##n##_data;				\
	static const struct bflb_config bflb_uart##n##_config = {		\
		.reg = (uint32_t *)DT_INST_REG_ADDR(n),				\
		.pinctrl_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		.periph_id = DT_INST_PROP(n, peripheral_id),			\
										\
		.uart_cfg.baudRate = DT_INST_PROP(n, current_speed),		\
		.uart_cfg.dataBits = UART_DATABITS_8,				\
		.uart_cfg.stopBits = UART_STOPBITS_1,				\
		.uart_cfg.parity = UART_PARITY_NONE,				\
		.uart_cfg.uartClk = SOC_BOUFFALOLAB_BL_PLL160_FREQ_HZ,		\
		.uart_cfg.ctsFlowControl = UART_CTS_FLOWCONTROL_ENABLE,		\
		.uart_cfg.rtsSoftwareControl = UART_RTS_FLOWCONTROL_ENABLE,	\
		.uart_cfg.byteBitInverse = UART_MSB_FIRST_ENABLE,		\
										\
		.fifo_cfg.txFifoDmaThreshold = 1,				\
		.fifo_cfg.rxFifoDmaThreshold = 1,				\
		.fifo_cfg.txFifoDmaEnable = 0,					\
		.fifo_cfg.rxFifoDmaEnable = 0,					\
										\
		BFLB_UART_IRQ_HANDLER_FUNC(n)					\
	};									\
	DEVICE_DT_INST_DEFINE(n, &uart_bflb_init,				\
			      PM_DEVICE_DT_INST_GET(n),				\
			      &bflb_uart##n##_data,				\
			      &bflb_uart##n##_config, PRE_KERNEL_1,		\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &uart_bflb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BFLB_UART_INIT)
