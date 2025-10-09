/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xhsc_hc32_uart

/**
 * @brief Driver for UART port on HC32 family processor.
 * @note
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/intc_hc32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/hc32_clock_control.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>

#include <zephyr/linker/sections.h>
#include "uart_hc32.h"

#include <hc32_ll_usart.h>
#include <hc32_ll_interrupts.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(uart_hc32, CONFIG_UART_LOG_LEVEL);

/* This symbol takes the value 1 if one of the device instances */
/* is configured in dts with a domain clock */
#if HC32_DT_INST_DEV_DOMAIN_CLOCK_SUPPORT
#define HC32_UART_DOMAIN_CLOCK_SUPPORT 1
#else
#define HC32_UART_DOMAIN_CLOCK_SUPPORT 0
#endif

static inline void uart_hc32_set_parity(const struct device *dev, uint32_t parity)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetParity(config->usart, parity);
}

static inline uint32_t uart_hc32_get_parity(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetParity(config->usart);
}

static inline void uart_hc32_set_stopbits(const struct device *dev, uint32_t stopbits)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetStopBit(config->usart, stopbits);
}

static inline uint32_t uart_hc32_get_stopbits(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStopBit(config->usart);
}

static inline void uart_hc32_set_databits(const struct device *dev, uint32_t databits)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetDataWidth(config->usart, databits);
}

static inline uint32_t uart_hc32_get_databits(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetDataWidth(config->usart);
}

static inline void uart_hc32_set_hwctrl(const struct device *dev, uint32_t hwctrl)
{
	const struct uart_hc32_config *config = dev->config;

	USART_SetHWFlowControl(config->usart, hwctrl);
}

static inline uint32_t uart_hc32_get_hwctrl(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetHWFlowControl(config->usart);
}

static inline void uart_hc32_set_baudrate(const struct device *dev, uint32_t baud_rate)
{
	const struct uart_hc32_config *config = dev->config;

	(void)USART_SetBaudrate(config->usart, baud_rate, NULL);
}

static inline uint32_t uart_hc32_cfg2ll_parity(enum uart_config_parity parity)
{
	uint32_t ret_ll_parity = USART_PARITY_NONE;

	switch (parity) {
	case UART_CFG_PARITY_ODD:
		ret_ll_parity = USART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		ret_ll_parity = USART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_NONE:
	default:
		break;
	}

	return ret_ll_parity;
}

static inline enum uart_config_parity uart_hc32_ll2cfg_parity(uint32_t parity)
{
	enum uart_config_parity ret_cfg_parity = UART_CFG_PARITY_NONE;

	switch (parity) {
	case USART_PARITY_ODD:
		ret_cfg_parity = UART_CFG_PARITY_ODD;
		break;
	case USART_PARITY_EVEN:
		ret_cfg_parity = UART_CFG_PARITY_EVEN;
		break;
	case USART_PARITY_NONE:
	default:
		break;
	}

	return ret_cfg_parity;
}

static inline uint32_t uart_hc32_cfg2ll_stopbits(enum uart_config_stop_bits sb)
{
	uint32_t ret_ll_stopbits = USART_STOPBIT_1BIT;

#ifdef USART_STOPBIT_2BIT
	if (sb == UART_CFG_STOP_BITS_2) {
		ret_ll_stopbits = USART_STOPBIT_2BIT;
	}
#endif /* USART_STOPBIT_2BIT */

	return ret_ll_stopbits;
}

static inline enum uart_config_stop_bits uart_hc32_ll2cfg_stopbits(uint32_t sb)
{
	enum uart_config_stop_bits ret_cfg_stopbits = UART_CFG_STOP_BITS_1;

#ifdef USART_STOPBIT_2BIT
	if (sb == USART_STOPBIT_2BIT) {
		ret_cfg_stopbits = UART_CFG_STOP_BITS_2;
	}
#endif /* USART_STOPBIT_2BIT */

	return ret_cfg_stopbits;
}

static inline uint32_t uart_hc32_cfg2ll_databits(enum uart_config_data_bits db)
{
	uint32_t ret_ll_databits = USART_DATA_WIDTH_8BIT;

/* Some MCU's don't support 9B datawidth */
#ifdef USART_DATA_WIDTH_9BIT
	if (db == UART_CFG_DATA_BITS_9) {
		ret_ll_databits = USART_DATA_WIDTH_9BIT;
	}
#endif /* USART_DATA_WIDTH_9BIT */

	return ret_ll_databits;
}

static inline enum uart_config_data_bits uart_hc32_ll2cfg_databits(uint32_t db)
{
	enum uart_config_data_bits ret_cfg_databits = UART_CFG_DATA_BITS_8;

/* Some MCU's don't support 9B datawidth */
#ifdef USART_DATA_WIDTH_9BIT
	if (db == USART_DATA_WIDTH_9BIT) {
		ret_cfg_databits = UART_CFG_DATA_BITS_9;
	}
#endif /* USART_DATA_WIDTH_9BIT */

	return ret_cfg_databits;
}

/**
 * @brief  Get DDL hardware flow control define from
 *         Zephyr hardware flow control option.
 * @note   Supports only UART_CFG_FLOW_CTRL_RTS_CTS and UART_CFG_FLOW_CTRL_RS485.
 * @param  fc: Zephyr hardware flow control option.
 * @retval USART_HW_FLOWCTRL_RTS, for device under supporting RTS_CTS.
 */
static inline uint32_t uart_hc32_cfg2ll_hwctrl(enum uart_config_flow_control fc)
{
	/* default config */
	return USART_HW_FLOWCTRL_RTS;
}

/**
 * @brief  Get Zephyr hardware flow control option from
 *         DDL hardware flow control define.
 * @note
 * @param  fc: DDL hardware flow control definition.
 * @retval UART_CFG_FLOW_CTRL_RTS_CTS, or UART_CFG_FLOW_CTRL_NONE.
 */
static inline enum uart_config_flow_control uart_hc32_ll2cfg_hwctrl(uint32_t fc)
{
	/* DDL driver compatible with cfg, just return none */
	return UART_CFG_FLOW_CTRL_NONE;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_hc32_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	const uint32_t parity = uart_hc32_cfg2ll_parity(cfg->parity);
	const uint32_t stopbits = uart_hc32_cfg2ll_stopbits(cfg->stop_bits);
	const uint32_t databits = uart_hc32_cfg2ll_databits(cfg->data_bits);

	/* Hardware doesn't support mark or space parity */
	if ((cfg->parity == UART_CFG_PARITY_MARK) || (cfg->parity == UART_CFG_PARITY_SPACE)) {
		return -ENOTSUP;
	}

	/* Driver does not supports parity + 9 databits */
	if ((cfg->parity != UART_CFG_PARITY_NONE) && (cfg->data_bits == USART_DATA_WIDTH_9BIT)) {
		return -ENOTSUP;
	}

	/* When the transformed ddl parity don't match with what was requested,
	 * then it's not supported
	 */
	if (uart_hc32_ll2cfg_parity(parity) != cfg->parity) {
		return -ENOTSUP;
	}

	/* When the transformed ddl stop bits don't match with what was requested,
	 * then it's not supported
	 */
	if (uart_hc32_ll2cfg_stopbits(stopbits) != cfg->stop_bits) {
		return -ENOTSUP;
	}

	/* When the transformed ddl databits don't match with what was requested,
	 * then it's not supported
	 */
	if (uart_hc32_ll2cfg_databits(databits) != cfg->data_bits) {
		return -ENOTSUP;
	}

	USART_FuncCmd(config->usart, USART_TX | USART_RX, DISABLE);
	uart_hc32_set_parity(dev, parity);
	uart_hc32_set_stopbits(dev, stopbits);
	uart_hc32_set_databits(dev, databits);
	uart_hc32_set_baudrate(dev, uart_cfg->baudrate);
	USART_FuncCmd(config->usart, USART_TX | USART_RX, ENABLE);

	return 0;
}

static int uart_hc32_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_hc32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;

	cfg->baudrate = uart_cfg->baudrate;
	cfg->parity = uart_hc32_ll2cfg_parity(uart_hc32_get_parity(dev));
	cfg->stop_bits = uart_hc32_ll2cfg_stopbits(uart_hc32_get_stopbits(dev));
	cfg->data_bits = uart_hc32_ll2cfg_databits(uart_hc32_get_databits(dev));
	cfg->flow_ctrl = uart_hc32_ll2cfg_hwctrl(uart_hc32_get_hwctrl(dev));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_hc32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_hc32_config *config = dev->config;

	if (USART_GetStatus(config->usart, USART_FLAG_OVERRUN | USART_FLAG_RX_TIMEOUT)) {

		USART_ClearStatus(config->usart, USART_FLAG_OVERRUN | USART_FLAG_FRAME_ERR);
	}

	if (SET != USART_GetStatus(config->usart, USART_FLAG_RX_FULL)) {
		return EIO;
	}

	*c = (unsigned char)USART_ReadData(config->usart);
	return 0;
}

static void uart_hc32_poll_out(const struct device *dev, unsigned char c)
{
	unsigned int key;
	const struct uart_hc32_config *config = dev->config;

	key = irq_lock();
	while (1) {
		if (USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY)) {
			break;
		}
	}

	USART_WriteData(config->usart, (uint16_t)c);
	irq_unlock(key);
}

static int uart_hc32_err_check(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	uint32_t err = 0U;

	if (USART_GetStatus(config->usart, USART_FLAG_OVERRUN)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (USART_GetStatus(config->usart, USART_FLAG_FRAME_ERR)) {
		err |= UART_ERROR_FRAMING;
	}

	if (USART_GetStatus(config->usart, USART_FLAG_PARITY_ERR)) {
		err |= UART_ERROR_PARITY;
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

typedef void (*fifo_fill_fn)(const struct device *dev, const void *tx_data, const uint8_t offset);

static int uart_hc32_fifo_fill_visitor(const struct device *dev, const void *tx_data, int size,
				       fifo_fill_fn fill_fn)
{
	const struct uart_hc32_config *config = dev->config;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY)) {
		return num_tx;
	}

	/* Lock interrupts to prevent nested interrupts or thread switch */
	key = irq_lock();

	/* TXE flag will be set by hardware when moving data */
	while ((size - num_tx > 0) && USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY)) {
		/* Send a character */
		fill_fn(dev, tx_data, num_tx);
		num_tx++;
	}

	irq_unlock(key);

	return num_tx;
}

static void fifo_fill_with_u8(const struct device *dev, const void *tx_data, const uint8_t offset)
{
	const uint8_t *data = (const uint8_t *)tx_data;
	/* Send a character (8bit) */
	uart_hc32_poll_out(dev, data[offset]);
}

static int uart_hc32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	if (UART_CFG_DATA_BITS_9 == uart_hc32_ll2cfg_databits(uart_hc32_get_databits(dev))) {
		return -ENOTSUP;
	}
	return uart_hc32_fifo_fill_visitor(dev, (const void *)tx_data, size, fifo_fill_with_u8);
}

typedef void (*fifo_read_fn)(const struct uart_hc32_config *config, void *rx_data,
			     const uint8_t offset);

static int uart_hc32_fifo_read_visitor(const struct device *dev, void *rx_data, const int size,
				       fifo_read_fn read_fn)
{
	const struct uart_hc32_config *config = dev->config;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && USART_GetStatus(config->usart, USART_FLAG_RX_FULL)) {
		/* RXNE flag will be cleared upon read from RDR register */

		read_fn(config, rx_data, num_rx);
		num_rx++;

		/* Clear overrun error flag */
		if (USART_GetStatus(config->usart, USART_FLAG_OVERRUN)) {
			USART_ClearStatus(config->usart, USART_FLAG_OVERRUN);
		}
	}

	return num_rx;
}

static void fifo_read_with_u8(const struct uart_hc32_config *config, void *rx_data,
			      const uint8_t offset)
{
	uint8_t *data = (uint8_t *)rx_data;

	USART_UART_Receive(config->usart, &data[offset], 1U, 0xFFU);
}

static int uart_hc32_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	if (UART_CFG_DATA_BITS_9 == uart_hc32_ll2cfg_databits(uart_hc32_get_databits(dev))) {

		return -ENOTSUP;
	}
	return uart_hc32_fifo_read_visitor(dev, (void *)rx_data, size, fifo_read_with_u8);
}

static void uart_hc32_irq_tx_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_INT_TX_EMPTY | USART_INT_TX_CPLT, ENABLE);
}

static void uart_hc32_irq_tx_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_INT_TX_EMPTY | USART_INT_TX_CPLT, DISABLE);
}

static int uart_hc32_irq_tx_ready(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_TX_EMPTY);
}

static int uart_hc32_irq_tx_complete(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_TX_CPLT);
}

static void uart_hc32_irq_rx_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_INT_RX, ENABLE);
}

static void uart_hc32_irq_rx_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_INT_RX, DISABLE);
}

static int uart_hc32_irq_rx_ready(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_RX_FULL);
}

static void uart_hc32_irq_err_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	/* Enable FE, ORE parity error interruption */
	USART_FuncCmd(config->usart, USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT, ENABLE);
}

static void uart_hc32_irq_err_disable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	USART_FuncCmd(config->usart, USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT, DISABLE);
}

static int uart_hc32_irq_is_pending(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;

	return USART_GetStatus(config->usart, USART_FLAG_ALL);
}

static int uart_hc32_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

/*
 *  user may use void *user_data = x to specify irq type, the x may be:
 *  0：usart_hc32f4_rx_error_isr
 *  1：usart_hc32f4_rx_full_isr
 *  2：usart_hc32f4_tx_empty_isr
 *  3：usart_hc32f4_tx_complete_isr
 *  4：usart_hc32f4_rx_timeout_isr
 *  others or user_data = NULL：set cb for all interrupt handlers
 */
static void uart_hc32_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *user_data)
{
	uint32_t i;
	struct uart_hc32_data *data = dev->data;

	if ((user_data == NULL) || (*(uint32_t *)user_data >= HC32_UART_INT_NUM)) {
		for (i = 0; i < HC32_UART_INT_NUM; i++) {
			data->cb[i].user_cb = cb;
			data->cb[i].user_data = user_data;
		}
	} else {
		i = *(uint32_t *)user_data;
		data->cb[i].user_cb = cb;
		data->cb[i].user_data = user_data;
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_hc32_driver_api = {
	.poll_in = uart_hc32_poll_in,
	.poll_out = uart_hc32_poll_out,
	.err_check = uart_hc32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_hc32_configure,
	.config_get = uart_hc32_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_hc32_fifo_fill,
	.fifo_read = uart_hc32_fifo_read,
	.irq_tx_enable = uart_hc32_irq_tx_enable,
	.irq_tx_disable = uart_hc32_irq_tx_disable,
	.irq_tx_ready = uart_hc32_irq_tx_ready,
	.irq_tx_complete = uart_hc32_irq_tx_complete,
	.irq_rx_enable = uart_hc32_irq_rx_enable,
	.irq_rx_disable = uart_hc32_irq_rx_disable,
	.irq_rx_ready = uart_hc32_irq_rx_ready,
	.irq_err_enable = uart_hc32_irq_err_enable,
	.irq_err_disable = uart_hc32_irq_err_disable,
	.irq_is_pending = uart_hc32_irq_is_pending,
	.irq_update = uart_hc32_irq_update,
	.irq_callback_set = uart_hc32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_hc32_registers_configure(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	struct uart_config *uart_cfg = data->uart_cfg;
	stc_usart_uart_init_t stcUartInit;

	USART_FuncCmd(config->usart, HC32_UART_FUNC, DISABLE);

	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV16;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	stcUartInit.u32Baudrate = uart_cfg->baudrate;
	stcUartInit.u32StopBit = uart_hc32_cfg2ll_stopbits(uart_cfg->stop_bits);
	stcUartInit.u32Parity = uart_hc32_cfg2ll_parity(uart_cfg->parity);

	(void)USART_UART_Init(config->usart, &stcUartInit, NULL);
	USART_FuncCmd(config->usart, USART_TX | USART_RX, ENABLE);

	return 0;
}

static inline void __uart_hc32_get_clock(const struct device *dev)
{
	struct uart_hc32_data *data = dev->data;
	const struct device *const clk = DEVICE_DT_GET(HC32_CLOCK_CONTROL_NODE);

	data->clock = clk;
}

static int uart_hc32_clocks_enable(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	struct uart_hc32_data *data = dev->data;
	int err;

	__uart_hc32_get_clock(dev);

	if (!device_is_ready(data->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* enable clock */
	err = clock_control_on(data->clock, (clock_control_subsys_t)(config->clk_cfg));
	if (err != 0) {
		LOG_ERR("Could not enable UART clock");
		return err;
	}
	return 0;
}

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct
 *
 * @return 0
 */
static int uart_hc32_init(const struct device *dev)
{
	const struct uart_hc32_config *config = dev->config;
	int err;

	err = uart_hc32_clocks_enable(dev);
	if (err < 0) {
		return err;
	}

	/* Configure dt provided device signals when available */
	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	err = uart_hc32_registers_configure(dev);
	if (err < 0) {
		return err;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	if ((uart_irq_config_func_t)NULL != config->irq_config_func) {
		config->irq_config_func(dev);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#define DT_USART_HAS_INTR(node_id) IS_ENABLED(DT_CAT4(node_id, _P_, interrupts, _EXISTS))

#define DT_IS_INTR_EXIST(index) DT_USART_HAS_INTR(DT_INST(index, xhsc_hc32_uart))

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define DT_IS_HC32_USART_INT_DEF(node_id, isr_name)                                                \
	IS_ENABLED(DT_CAT4(node_id, _IRQ_NAME_, isr_name, _VAL_irq_EXISTS))

#define DT_IS_HC32_UART_INTR_EXIST(index, isr_name)                                                \
	DT_IS_HC32_USART_INT_DEF(DT_INST(index, xhsc_hc32_uart), isr_name)

#define USART_IRQ_ISR_CONFIG(isr_name, index)                                                      \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, isr_name, irq),                                     \
		    DT_INST_IRQ_BY_NAME(index, isr_name, priority),                                \
		    DT_CAT4(usart_hc32_, isr_name, _isr_, index), DEVICE_DT_INST_GET(index), 0);   \
	hc32_intc_irq_signin(DT_INST_IRQ_BY_NAME(index, isr_name, irq),                            \
			     DT_INST_IRQ_BY_NAME(index, isr_name, int_src));                       \
	irq_enable(DT_INST_IRQ_BY_NAME(index, isr_name, irq));

#define HC32_UART_IRQ_HANDLER_DECL(index)                                                          \
	static void usart_hc32_config_func_##index(const struct device *dev);                      \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, ore),                                        \
		(static void usart_hc32_ore_isr_##index(const struct device *dev);)                \
		, ()) \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, rxne),                                       \
		(static void usart_hc32_rxne_isr_##index(const struct device *dev);)               \
		, ()) \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, tc),                                         \
		(static void usart_hc32_tc_isr_##index(const struct device *dev);)                 \
		, ()) \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, txe),                                        \
		(static void usart_hc32_txe_isr_##index(const struct device *dev);)                \
		, ())

#define HC32_UART_IRQ_HANDLER_DEF_BY_NAME(index, name, cb_idx)                                     \
	static void usart_hc32_##name##_isr_##index(const struct device *dev)                      \
	{                                                                                          \
		struct uart_hc32_data *const data = dev->data;                                     \
                                                                                                   \
		if (data->cb[cb_idx].user_cb) {                                                    \
			data->cb[cb_idx].user_cb(dev, data->cb[cb_idx].user_data);                 \
		}                                                                                  \
	}

#define HC32_UART_IRQ_HANDLER_DEF(index)                                                           \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, ore),                                        \
		(HC32_UART_IRQ_HANDLER_DEF_BY_NAME(index, ore, HC32_UART_IDX_EI)),                 \
		()) \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, rxne),                                       \
		(HC32_UART_IRQ_HANDLER_DEF_BY_NAME(index, rxne, HC32_UART_IDX_EI)),                \
		()) \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, tc),                                         \
		(HC32_UART_IRQ_HANDLER_DEF_BY_NAME(index, tc, HC32_UART_IDX_EI)),                  \
		()) \
	COND_CODE_1(DT_IS_HC32_UART_INTR_EXIST(index, txe),                                        \
		(HC32_UART_IRQ_HANDLER_DEF_BY_NAME(index, txe, HC32_UART_IDX_EI)),                 \
		()) \
                                                                                                   \
	static void usart_hc32_config_func_##index(const struct device *dev)                       \
	{                                                                                          \
		COND_CODE_1(                                                                       \
			DT_IS_HC32_UART_INTR_EXIST(index, ore),                                    \
			(USART_IRQ_ISR_CONFIG(ore, index)), ()) \
		COND_CODE_1(                                                                       \
			DT_IS_HC32_UART_INTR_EXIST(index, rxne),                                   \
			(USART_IRQ_ISR_CONFIG(rxne, index)), ()) \
		COND_CODE_1(                                                                       \
			DT_IS_HC32_UART_INTR_EXIST(index, tc),                                     \
			(USART_IRQ_ISR_CONFIG(tc, index)), ()) \
		COND_CODE_1(                                                                       \
			DT_IS_HC32_UART_INTR_EXIST(index, txe),                                    \
			(USART_IRQ_ISR_CONFIG(txe, index)), ()) \
	}

#define HC32_UART_IRQ_HANDLER_FUNC(index) .irq_config_func = usart_hc32_config_func_##index,
#define HC32_UART_IRQ_HANDLER_NULL(index) .irq_config_func = (uart_irq_config_func_t)NULL,

#define HC32_UART_IRQ_HANDLER_PRE_FUNC(index)                                                      \
	COND_CODE_1(                                                                               \
			DT_IS_INTR_EXIST(index),                                                   \
			(HC32_UART_IRQ_HANDLER_FUNC(index)),                                       \
			(HC32_UART_IRQ_HANDLER_NULL(index)))

#define HC32_UART_IRQ_HANDLER_PRE_DECL(index)                                                      \
	COND_CODE_1(                                                                               \
			DT_IS_INTR_EXIST(index),                                                   \
			(HC32_UART_IRQ_HANDLER_DECL(index)),                                       \
			())

#define HC32_UART_IRQ_HANDLER_PRE_DEF(index)                                                       \
	COND_CODE_1(                                                                               \
			DT_IS_INTR_EXIST(index),                                                   \
			(HC32_UART_IRQ_HANDLER_DEF(index)),                                        \
			())
#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define HC32_UART_IRQ_HANDLER_PRE_DECL(index)
#define HC32_UART_IRQ_HANDLER_PRE_DEF(index)
#define HC32_UART_IRQ_HANDLER_PRE_FUNC(index)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define HC32_UART_CLOCK_DECL(index)                                                                \
	static struct hc32_modules_clock_sys uart_fcg_config_##index[] =                           \
		HC32_MODULES_CLOCKS(DT_DRV_INST(index));

#define HC32_UART_INIT(index)                                                                      \
	HC32_UART_CLOCK_DECL(index)                                                                \
	HC32_UART_IRQ_HANDLER_PRE_DECL(index)                                                      \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static struct uart_config uart_cfg_##index = {                                             \
		.baudrate = DT_INST_PROP_OR(index, current_speed, HC32_UART_DEFAULT_BAUDRATE),     \
		.parity = DT_INST_ENUM_IDX_OR(index, parity, HC32_UART_DEFAULT_PARITY),            \
		.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits, HC32_UART_DEFAULT_STOP_BITS),   \
		.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits, HC32_UART_DEFAULT_DATA_BITS),   \
		.flow_ctrl = DT_INST_PROP(index, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS     \
								  : UART_CFG_FLOW_CTRL_NONE,       \
	};                                                                                         \
	static const struct uart_hc32_config uart_hc32_cfg_##index = {                             \
		.usart = (CM_USART_TypeDef *)DT_INST_REG_ADDR(index),                              \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                  \
		.clk_cfg = uart_fcg_config_##index,                                                \
		HC32_UART_IRQ_HANDLER_PRE_FUNC(index)};                                            \
	static struct uart_hc32_data uart_hc32_data_##index = {                                    \
		.uart_cfg = &uart_cfg_##index,                                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &uart_hc32_init, NULL, &uart_hc32_data_##index,               \
			      &uart_hc32_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,   \
			      &uart_hc32_driver_api);                                              \
	HC32_UART_IRQ_HANDLER_PRE_DEF(index)

DT_INST_FOREACH_STATUS_OKAY(HC32_UART_INIT)
