/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_uart

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include <rtl_uart.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_bee, CONFIG_UART_LOG_LEVEL);

struct uart_bee_config {
	UART_TypeDef *uart;
	uint16_t clkid;
	uint8_t rx_threshold;
	bool hw_flow_ctrl;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

struct uart_bee_data {
	const struct device *dev;
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
	bool tx_int_en;
	bool rx_int_en;
#endif
};

/* clang-format off */
static const struct {
	uint16_t div;
	uint16_t ovsr;
	uint16_t ovsr_adj;
	uint32_t baudrate;
} uart_bee_baudrate_table[] = {
	{271, 10, 0x24A, 9600},
	{150, 8, 0x3EF, 19200},
	{20, 12, 0x252, 115200},
	{11, 10, 0x3BB, 230400},
	{11, 9, 0x084, 256000},
	{7, 9, 0x3EF, 384000},
	{6, 9, 0x0AA, 460800},
	{3, 9, 0x0AA, 921600},
	{4, 5, 0, 1000000},
	{2, 5, 0, 2000000},
	{1, 8, 0x292, 3000000},
};
/* clang-format on */

static int uart_bee_cfg2idx_baudrate(uint32_t baudrate)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(uart_bee_baudrate_table); i++) {
		if (uart_bee_baudrate_table[i].baudrate == baudrate) {
			return i;
		}
	}

	return -ENOTSUP;
}

static int uart_bee_cfg2mac_data_bits(enum uart_config_data_bits data_bits)
{
	switch (data_bits) {
	case UART_CFG_DATA_BITS_7:
		return UART_WORD_LENGTH_7BIT;
	case UART_CFG_DATA_BITS_8:
		return UART_WORD_LENGTH_8BIT;
	default:
		return -ENOTSUP;
	}
}

static int uart_bee_cfg2mac_stopbits(enum uart_config_stop_bits stop_bits)
{
	switch (stop_bits) {
	case UART_CFG_STOP_BITS_1:
		return UART_STOP_BITS_1;
	case UART_CFG_STOP_BITS_2:
		return UART_STOP_BITS_2;
	default:
		return -ENOTSUP;
	}
}

static int uart_bee_cfg2mac_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		return UART_PARITY_NO_PARTY;
	case UART_CFG_PARITY_ODD:
		return UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return UART_PARITY_EVEN;
	default:
		return -ENOTSUP;
	}
}

static int uart_bee_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;
	UART_InitTypeDef uart_init_struct;

	int baudrate_idx;
	int wordlen;
	int stopbits;
	int parity;

	baudrate_idx = uart_bee_cfg2idx_baudrate(cfg->baudrate);
	if (baudrate_idx < 0) {
		LOG_ERR("Unspported baudrate: %d", cfg->baudrate);
		return -ENOTSUP;
	}

	wordlen = uart_bee_cfg2mac_data_bits(cfg->data_bits);
	if (wordlen < 0) {
		LOG_ERR("Unspported data_bits: %d", cfg->data_bits);
		return -ENOTSUP;
	}

	stopbits = uart_bee_cfg2mac_stopbits(cfg->stop_bits);
	if (stopbits < 0) {
		LOG_ERR("Unspported stop_bits: %d", cfg->stop_bits);
		return -ENOTSUP;
	}

	parity = uart_bee_cfg2mac_parity(cfg->parity);
	if (parity < 0) {
		LOG_ERR("Unspported parity: %d", cfg->parity);
		return -ENOTSUP;
	}

	LOG_DBG("baudrate_idx=%d, wordlen=%d, stopbits=%d, parity=%d, "
		"hw_flow_ctrl=%d",
		baudrate_idx, wordlen, stopbits, parity, config->hw_flow_ctrl);

	UART_StructInit(&uart_init_struct);
	uart_init_struct.UART_Div = uart_bee_baudrate_table[baudrate_idx].div;
	uart_init_struct.UART_Ovsr = uart_bee_baudrate_table[baudrate_idx].ovsr;
	uart_init_struct.UART_OvsrAdj = uart_bee_baudrate_table[baudrate_idx].ovsr_adj;
	uart_init_struct.UART_IdleTime = UART_RX_IDLE_1BYTE;
	uart_init_struct.UART_WordLen = wordlen;
	uart_init_struct.UART_StopBits = stopbits;
	uart_init_struct.UART_Parity = parity;
	uart_init_struct.UART_HardwareFlowControl = cfg->flow_ctrl;
	uart_init_struct.UART_RxThdLevel = config->rx_threshold;
	uart_init_struct.UART_TxThdLevel = UART_TX_FIFO_SIZE / 2;

	UART_Init(uart, &uart_init_struct);

	data->uart_config = *cfg;
	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_bee_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_bee_data *data = dev->data;

	*cfg = data->uart_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_bee_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	if (!UART_GetFlagStatus(uart, UART_FLAG_RX_DATA_AVA)) {
		return -1;
	}
	LOG_DBG("c=%c", *c);

	*c = (unsigned char)UART_ReceiveByte(uart);

	return 0;
}

static void uart_bee_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	while (!(UART_GetTxFIFODataLen(uart) < UART_TX_FIFO_SIZE)) {
	}

	UART_SendByte(uart, (uint8_t)c);
}

static int uart_bee_err_check(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	uint32_t err = 0U;

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_OVERRUN)) {
		err |= UART_ERROR_OVERRUN;
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_PARITY_ERR)) {
		err |= UART_ERROR_PARITY;
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_FRAME_ERR)) {
		err |= UART_ERROR_FRAMING;
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_BREAK_ERR)) {
		err |= UART_BREAK;
	}

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_bee_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint8_t num_tx = 0U;
	unsigned int key;

	if (!(UART_GetTxFIFODataLen(uart) < UART_TX_FIFO_SIZE)) {
		return num_tx;
	}

	key = irq_lock();

	while ((size - num_tx > 0) && (UART_GetTxFIFODataLen(uart) < UART_TX_FIFO_SIZE)) {
		UART_SendByte(uart, (uint8_t)tx_data[num_tx++]);
	}

	irq_unlock(key);

	LOG_DBG("num_tx=%d", num_tx);

	return num_tx;
}

static int uart_bee_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	uint8_t num_rx = 0U;

	while ((size - num_rx > 0) && UART_GetFlagStatus(uart, UART_FLAG_RX_DATA_AVA)) {
		rx_data[num_rx++] = UART_ReceiveByte(uart);
	}

	LOG_DBG("num_rx=%d", num_rx);

	return num_rx;
}

static void uart_bee_irq_tx_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->tx_int_en = true;
	UART_INTConfig(uart, UART_INT_TX_FIFO_EMPTY, ENABLE);
}

static void uart_bee_irq_tx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->tx_int_en = false;
	UART_INTConfig(uart, UART_INT_TX_FIFO_EMPTY, DISABLE);
}

static int uart_bee_irq_tx_ready(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return UART_GetFlagStatus(uart, UART_FLAG_TX_EMPTY) && data->tx_int_en;
}

static int uart_bee_irq_tx_complete(const struct device *dev)
{
	return uart_bee_irq_tx_ready(dev);
}

static void uart_bee_irq_rx_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->rx_int_en = true;
	UART_INTConfig(uart, UART_INT_RD_AVA, ENABLE);
	UART_INTConfig(uart, UART_INT_RX_IDLE, ENABLE);
}

static void uart_bee_irq_rx_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data = dev->data;

	data->rx_int_en = false;
	UART_INTConfig(uart, UART_INT_RD_AVA, DISABLE);
	UART_INTConfig(uart, UART_INT_RX_IDLE, DISABLE);
}

static int uart_bee_irq_rx_ready(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data;
	int status;

	status = UART_GetFlagStatus(uart, UART_FLAG_RX_DATA_AVA);

	data = dev->data;

	return status;
}

static void uart_bee_irq_err_enable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data;

	data = dev->data;

	UART_INTConfig(uart, UART_INT_RX_LINE_STS, ENABLE);
}

static void uart_bee_irq_err_disable(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;
	struct uart_bee_data *data;

	data = dev->data;

	UART_INTConfig(uart, UART_INT_RX_LINE_STS, DISABLE);
}

static int uart_bee_irq_is_pending(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	UART_TypeDef *uart = config->uart;

	return ((UART_GetFlagStatus(uart, UART_FLAG_TX_EMPTY) && data->tx_int_en) ||
		(UART_GetFlagStatus(uart, UART_INT_RD_AVA) && data->rx_int_en));
}

static int uart_bee_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_bee_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_bee_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
int uart_bee_line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
	LOG_ERR("Unsupport line_ctrl_set function");
	return -ENOTSUP;
}

int uart_bee_line_ctrl_get(const struct device *dev, uint32_t ctrl, uint32_t *val)
{
	LOG_ERR("Unsupport line_ctrl_get function");
	return -ENOTSUP;
}
#endif

#ifdef CONFIG_UART_DRV_CMD
int uart_bee_drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p)
{
	LOG_ERR("Unsupport drv_cmd function");
	return -ENOTSUP;
}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static void uart_bee_isr(const struct device *dev)
{
	struct uart_bee_data *data = dev->data;
	const struct uart_bee_config *config = dev->config;
	UART_TypeDef *uart = config->uart;

	UART_GetIID(uart);
	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}

	if (UART_GetFlagStatus(uart, UART_FLAG_RX_IDLE)) {
		UART_INTConfig(uart, UART_INT_RX_IDLE, DISABLE);
		UART_INTConfig(uart, UART_INT_RX_IDLE, ENABLE);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_bee_init(const struct device *dev)
{
	const struct uart_bee_config *config = dev->config;
	struct uart_bee_data *data = dev->data;
	int err;

	data->dev = dev;

	/* Configure pinmux  */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&config->clkid);

	/* Configure peripheral  */
	err = uart_bee_configure(dev, &data->uart_config);
	if (err) {
		return err;
	}

	/* Enable nvic */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

static DEVICE_API(uart, uart_bee_driver_api) = {
	.poll_in = uart_bee_poll_in,
	.poll_out = uart_bee_poll_out,
	.err_check = uart_bee_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_bee_configure,
	.config_get = uart_bee_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_bee_fifo_fill,
	.fifo_read = uart_bee_fifo_read,
	.irq_tx_enable = uart_bee_irq_tx_enable,
	.irq_tx_disable = uart_bee_irq_tx_disable,
	.irq_tx_ready = uart_bee_irq_tx_ready,
	.irq_tx_complete = uart_bee_irq_tx_complete,
	.irq_rx_enable = uart_bee_irq_rx_enable,
	.irq_rx_disable = uart_bee_irq_rx_disable,
	.irq_rx_ready = uart_bee_irq_rx_ready,
	.irq_err_enable = uart_bee_irq_err_enable,
	.irq_err_disable = uart_bee_irq_err_disable,
	.irq_is_pending = uart_bee_irq_is_pending,
	.irq_update = uart_bee_irq_update,
	.irq_callback_set = uart_bee_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = uart_bee_line_ctrl_set,
	.line_ctrl_get = uart_bee_line_ctrl_get,
#endif

#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = uart_bee_drv_cmd,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define BEE_UART_IRQ_HANDLER_DECL(index)                                                           \
	static void uart_bee_irq_config_func_##index(const struct device *dev);
#define BEE_UART_IRQ_HANDLER(index)                                                                \
	static void uart_bee_irq_config_func_##index(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), uart_bee_isr,       \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#else
#define BEE_UART_IRQ_HANDLER_DECL(index) /* Not used */
#define BEE_UART_IRQ_HANDLER(index)      /* Not used */
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define BEE_UART_IRQ_HANDLER_FUNC(index) .irq_config_func = uart_bee_irq_config_func_##index,
#else
#define BEE_UART_IRQ_HANDLER_FUNC(index) /* Not used */
#endif

#define UART_DMA_CHANNEL(index, dir)

#define BEE_UART_INIT(index)                                                                       \
	BEE_UART_IRQ_HANDLER_DECL(index)                                                           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct uart_bee_config uart_bee_cfg_##index = {                               \
		.uart = (UART_TypeDef *)DT_INST_REG_ADDR(index),                                   \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.rx_threshold = DT_INST_PROP_OR(index, rx_threshold, 10),                          \
		.hw_flow_ctrl = DT_INST_PROP_OR(index, flow_ctrl, false),                          \
		BEE_UART_IRQ_HANDLER_FUNC(index)};                                                 \
                                                                                                   \
	static struct uart_bee_data uart_bee_data_##index = {                                      \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity =                                                          \
					DT_INST_ENUM_IDX_OR(index, parity, UART_CFG_PARITY_NONE),  \
				.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits,                 \
								 UART_CFG_STOP_BITS_1),            \
				.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits,                 \
								 UART_CFG_DATA_BITS_8),            \
				.flow_ctrl = DT_INST_PROP(index, hw_flow_control),                 \
			},                                                                         \
		UART_DMA_CHANNEL(index, rx) UART_DMA_CHANNEL(index, tx)};                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &uart_bee_init, NULL, &uart_bee_data_##index,                 \
			      &uart_bee_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,    \
			      &uart_bee_driver_api);                                               \
                                                                                                   \
	BEE_UART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(BEE_UART_INIT)
