/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch5xx_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <hal_ch32fun.h>

struct uart_wch_config {
	UART_TypeDef *regs;
	const struct pinctrl_dev_config *pin_cfg;
	uint32_t clock_frequency;
	uint32_t current_speed;
	uint8_t parity;
	uint8_t stop_bits;
	uint8_t data_bits;
	uint8_t prescaler_divisor;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_wch_data {
	uart_irq_callback_user_data_t cb;
	void *user_data;
};

static uint16_t uart_wch_get_baud_rate_divisor_latch(const struct uart_wch_config *config)
{
	uint32_t dl;

	/* Compute divisor latch value rounded to nearest integer */
	dl = 20 * config->clock_frequency / config->prescaler_divisor / 16 / config->current_speed;
	dl = (dl + 5) / 10;

	return (uint16_t)dl;
}

static int uart_wch_init(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;
	int err;
	uint8_t lcr = config->regs->LCR;

	/* LCR: Set stop bit field */
	lcr &= ~RB_LCR_STOP_BIT;
	switch (config->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		lcr |= FIELD_PREP(RB_LCR_STOP_BIT, 0);
		break;
	case UART_CFG_STOP_BITS_2:
		lcr |= FIELD_PREP(RB_LCR_STOP_BIT, 1);
		break;
	default:
		return -ENOTSUP;
	}

	/* LCR: Set word size field */
	lcr &= ~RB_LCR_WORD_SZ;
	switch (config->data_bits) {
	case UART_CFG_DATA_BITS_5:
		lcr |= FIELD_PREP(RB_LCR_WORD_SZ, 0b00);
		break;
	case UART_CFG_DATA_BITS_6:
		lcr |= FIELD_PREP(RB_LCR_WORD_SZ, 0b01);
		break;
	case UART_CFG_DATA_BITS_7:
		lcr |= FIELD_PREP(RB_LCR_WORD_SZ, 0b10);
		break;
	case UART_CFG_DATA_BITS_8:
		lcr |= FIELD_PREP(RB_LCR_WORD_SZ, 0b11);
		break;
	default:
		return -ENOTSUP;
	}

	/* LCR: Parity field */
	if (config->parity == UART_CFG_PARITY_NONE) {
		lcr &= ~RB_LCR_PAR_EN;
	} else {
		lcr |= RB_LCR_PAR_EN;
		lcr &= ~RB_LCR_PAR_MOD;

		switch (config->parity) {
		case UART_CFG_PARITY_ODD:
			lcr |= FIELD_PREP(RB_LCR_PAR_MOD, 0b00);
			break;
		case UART_CFG_PARITY_EVEN:
			lcr |= FIELD_PREP(RB_LCR_PAR_MOD, 0b01);
			break;
		case UART_CFG_PARITY_MARK:
			lcr |= FIELD_PREP(RB_LCR_PAR_MOD, 0b10);
			break;
		case UART_CFG_PARITY_SPACE:
			lcr |= FIELD_PREP(RB_LCR_PAR_MOD, 0b11);
			break;
		default:
			return -ENOTSUP;
		}
	}

	regs->IER = RB_IER_TXD_EN;
	regs->LCR = lcr;

	regs->DL = uart_wch_get_baud_rate_divisor_latch(config);
	regs->DIV = config->prescaler_divisor;

	err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* enable FIFO and set trigger level to 7 bytes */
	regs->FCR = RB_FCR_FIFO_TRIG | RB_FCR_FIFO_EN;
	/* enable interrupt output in MODEM Control Register */
	regs->MCR = RB_MCR_INT_OE;
	/* connect and enable the interrupt in PFIC and CPU */
	config->irq_config_func(dev);
#endif
	return 0;
}

static int uart_wch_poll_in(const struct device *dev, unsigned char *ch)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	if ((regs->LSR & RB_LSR_DATA_RDY) == 0) {
		return -1;
	}

	*ch = regs->THR_RBR;
	return 0;
}

static void uart_wch_poll_out(const struct device *dev, unsigned char ch)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	while (regs->TFC == UART_FIFO_SIZE) {
	}
#else
	while ((regs->LSR & RB_LSR_TX_FIFO_EMP) == 0) {
	}
#endif

	regs->THR_RBR = ch;
}

static int uart_wch_err_check(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;
	uint32_t lsr = regs->LSR;
	enum uart_rx_stop_reason errors = 0;

	if (lsr & RB_LSR_OVER_ERR) {
		errors |= UART_ERROR_OVERRUN;
	}
	if (lsr & RB_LSR_PAR_ERR) {
		errors |= UART_ERROR_PARITY;
	}
	if (lsr & RB_LSR_FRAME_ERR) {
		errors |= UART_ERROR_FRAMING;
	}
	if (lsr & RB_LSR_BREAK_ERR) {
		errors |= UART_BREAK;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static void uart_wch_isr(const struct device *dev)
{
	struct uart_wch_data *data = dev->data;

	if (data->cb) {
		data->cb(dev, data->user_data);
	}
}

static int uart_wch_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;
	int bytes_written;

	bytes_written = 0;
	while (regs->TFC != UART_FIFO_SIZE && bytes_written < len) {
		regs->THR_RBR = tx_data[bytes_written++];
	}

	return bytes_written;
}

static int uart_wch_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;
	int bytes_read;

	bytes_read = 0;
	while (bytes_read < size && regs->RFC != 0) {
		rx_data[bytes_read++] = regs->THR_RBR;
	}

	return bytes_read;
}

static void uart_wch_irq_tx_enable(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	regs->IER |= RB_IER_THR_EMPTY;
}

static void uart_wch_irq_tx_disable(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	regs->IER &= ~RB_IER_THR_EMPTY;
}

static int uart_wch_irq_tx_ready(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	return UART_FIFO_SIZE - regs->TFC;
}

static int uart_wch_irq_tx_complete(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	return (regs->LSR & RB_LSR_TX_ALL_EMP) > 0;
}

static void uart_wch_irq_rx_enable(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	regs->IER |= RB_IER_RECV_RDY;
}

static void uart_wch_irq_rx_disable(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	regs->IER &= ~RB_IER_RECV_RDY;
}

static int uart_wch_irq_rx_ready(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	return (regs->LSR & RB_LSR_DATA_RDY) > 0;
}

static void uart_wch_irq_err_enable(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	regs->IER |= RB_IER_LINE_STAT;
}

static void uart_wch_irq_err_disable(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	regs->IER &= ~RB_IER_LINE_STAT;
}

static int uart_wch_irq_is_pending(const struct device *dev)
{
	const struct uart_wch_config *config = dev->config;
	UART_TypeDef *regs = config->regs;

	if (regs->IIR & RB_IIR_NO_INT) {
		return false;
	}

	switch ((regs->IIR & RB_IIR_INT_MASK) >> 1) {
	case 0b001:
		return regs->IER & RB_IER_THR_EMPTY;

	case 0b010:
	case 0b110:
		return regs->IER & RB_IER_RECV_RDY;

	case 0b011:
		return regs->IER & RB_IER_LINE_STAT;

	default:
		return false;
	}
}

static void uart_wch_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				      void *user_data)
{
	struct uart_wch_data *data = dev->data;

	data->cb = cb;
	data->user_data = user_data;
}
#endif /*CONFIG_UART_INTERRUPT_DRIVEN*/

static DEVICE_API(uart, uart_wch_driver_api) = {
	.poll_in = uart_wch_poll_in,
	.poll_out = uart_wch_poll_out,
	.err_check = uart_wch_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_wch_fifo_fill,
	.fifo_read = uart_wch_fifo_read,
	.irq_tx_enable = uart_wch_irq_tx_enable,
	.irq_tx_disable = uart_wch_irq_tx_disable,
	.irq_tx_ready = uart_wch_irq_tx_ready,
	.irq_tx_complete = uart_wch_irq_tx_complete,
	.irq_rx_enable = uart_wch_irq_rx_enable,
	.irq_rx_disable = uart_wch_irq_rx_disable,
	.irq_rx_ready = uart_wch_irq_rx_ready,
	.irq_err_enable = uart_wch_irq_err_enable,
	.irq_err_disable = uart_wch_irq_err_disable,
	.irq_is_pending = uart_wch_irq_is_pending,
	.irq_callback_set = uart_wch_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_WCH_IRQ_HANDLER_DECL(idx)                                                             \
	static void uart_wch_irq_config_func_##idx(const struct device *dev);

#define UART_WCH_IRQ_HANDLER_FUNC(idx) .irq_config_func = uart_wch_irq_config_func_##idx,

#define UART_WCH_IRQ_HANDLER(idx)                                                                  \
	static void uart_wch_irq_config_func_##idx(const struct device *dev)                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), uart_wch_isr,           \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
	}
#else
#define UART_WCH_IRQ_HANDLER_DECL(idx)
#define UART_WCH_IRQ_HANDLER_FUNC(idx)
#define UART_WCH_IRQ_HANDLER(idx)
#endif

#define UART_WCH_INIT(idx)                                                                         \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	UART_WCH_IRQ_HANDLER_DECL(idx)                                                             \
	static struct uart_wch_data uart_wch_##idx##_data;                                         \
	static const struct uart_wch_config uart_wch_##idx##_config = {                            \
		.regs = (UART_TypeDef *)DT_INST_REG_ADDR(idx),                                     \
		.clock_frequency = DT_PROP(DT_NODELABEL(cpu0), clock_frequency),                   \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                    \
		.current_speed = DT_INST_PROP(idx, current_speed),                                 \
		.parity = DT_INST_ENUM_IDX(idx, parity),                                           \
		.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                                     \
		.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                                     \
		.prescaler_divisor = DT_INST_PROP(idx, prescaler_divisor),                         \
		UART_WCH_IRQ_HANDLER_FUNC(idx)};                                                   \
	DEVICE_DT_INST_DEFINE(idx, &uart_wch_init, NULL, &uart_wch_##idx##_data,                   \
			      &uart_wch_##idx##_config, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_wch_driver_api);                                               \
	UART_WCH_IRQ_HANDLER(idx)

DT_INST_FOREACH_STATUS_OKAY(UART_WCH_INIT)
