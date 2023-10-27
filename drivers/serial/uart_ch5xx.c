/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch5xx_uart

#include <zephyr/arch/cpu.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_ch5xx, CONFIG_UART_LOG_LEVEL);

#define UART_R8_MCR(base) (base + 0x00)
#define UART_R8_IER(base) (base + 0x01)
#define UART_R8_FCR(base) (base + 0x02)
#define UART_R8_LCR(base) (base + 0x03)
#define UART_R8_IIR(base) (base + 0x04)
#define UART_R8_LSR(base) (base + 0x05)
#define UART_R8_MSR(base) (base + 0x06)
#define UART_R8_RBR(base) (base + 0x08)
#define UART_R8_THR(base) (base + 0x08)
#define UART_R8_RFC(base) (base + 0x0A)
#define UART_R8_TFC(base) (base + 0x0B)
#define UART_R16_DL(base) (base + 0x0C)
#define UART_R8_DIV(base) (base + 0x0E)

/* UART_R8_MCR */
#define MCR_DTR        BIT(0)
#define MCR_RTS        BIT(1)
#define MCR_OUT1       BIT(2)
#define MCR_INT_OE     BIT(3)
#define MCR_LOOP       BIT(4)
#define MCR_AU_FLOW_EN BIT(5)
#define MCR_TNOW       BIT(6)
#define MCR_HALF       BIT(7)

/* UART_R8_IER */
#define IER_RECV_RDY  BIT(0)
#define IER_THR_EMPTY BIT(1)
#define IER_LINE_STAT BIT(2)
#define IER_MODEM_CHG BIT(3)
#define IER_DTR_EN    BIT(4)
#define IER_RTS_EN    BIT(5)
#define IER_TXD_EN    BIT(6)
#define IER_RESET     BIT(7)

/* UART_R8_FCR */
#define FCR_FIFO_EN        BIT(0)
#define FCR_RX_FIFO_CLR    BIT(1)
#define FCR_TX_FIFO_CLR    BIT(2)
#define FCR_FIFO_TRIG_1    (0 << 6)
#define FCR_FIFO_TRIG_2    (1 << 6)
#define FCR_FIFO_TRIG_4    (2 << 6)
#define FCR_FIFO_TRIG_7    (3 << 6)
#define FCR_FIFO_TRIG_MASK (BIT_MASK(2) << 6)

/* UART_R8_LCR */
#define LCR_WORD_SZ_5     (0 << 0)
#define LCR_WORD_SZ_6     (1 << 0)
#define LCR_WORD_SZ_7     (2 << 0)
#define LCR_WORD_SZ_8     (3 << 0)
#define LCR_WORD_SZ_MASK  (BIT_MASK(2) << 0)
#define LCR_STOP_BIT      BIT(2)
#define LCR_PAR_EN        BIT(3)
#define LCR_PAR_MOD_ODD   (0 << 4)
#define LCR_PAR_MOD_EVEN  (1 << 4)
#define LCR_PAR_MOD_MASK  (2 << 4)
#define LCR_PAR_MOD_SPACE (3 << 4)
#define LCR_PAR_MOD_MARK  (BIT_MASK(2) << 4)
#define LCR_BREAK_EN      BIT(6)

/* UART_R8_IIR */
#define IIR_INT_MASK        (BIT_MASK(4) << 0)
#define IIR_INT_NOINT       (0x1)
#define IIR_INT_ADDR        (0xe)
#define IIR_INT_LSR         (0x6)
#define IIR_INT_RBR_AVAIL   (0x4)
#define IIR_INT_RBR_TIMEOUT (0xc)
#define IIR_INT_THR_EMPTY   (0x2)
#define IIR_INT_MSR_CHG     (0x0)

/* UART_R8_LSR */
#define LSR_DATA_RDY    BIT(0)
#define LSR_OVER_ERR    BIT(1)
#define LSR_PAR_ERR     BIT(2)
#define LSR_FRAME_ERR   BIT(3)
#define LSR_BREAK_ERR   BIT(4)
#define LSR_TX_FIFO_EMP BIT(5)
#define LSR_TX_ALL_EMP  BIT(6)
#define LSR_ERR_RX_FIFO BIT(7)

#define UART_FIFO_SIZE 8

struct uart_ch5xx_config {
	void (*irq_config_func)(void);
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
	uint32_t sys_clk_freq;
	uint32_t baud_rate;
};

struct uart_ch5xx_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
};

static int uart_ch5xx_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_ch5xx_config *cfg = dev->config;

	/* Wait for RX FIFO available */
	while (sys_read8(UART_R8_RFC(cfg->base)) == 0) {
	}

	*p_char = sys_read8(UART_R8_RBR(cfg->base));

	return 0;
}

static void uart_ch5xx_poll_out(const struct device *dev, unsigned char out_char)
{
	const struct uart_ch5xx_config *cfg = dev->config;

	/* Wait for TX FIFO available */
	while (sys_read8(UART_R8_TFC(cfg->base)) == UART_FIFO_SIZE) {
	}

	sys_write8(out_char, UART_R8_THR(cfg->base));
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static int uart_ch5xx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	int sent;

	for (sent = 0; sent < len && sys_read8(UART_R8_TFC(cfg->base)) < UART_FIFO_SIZE; sent++) {
		sys_write8(tx_data[sent], UART_R8_THR(cfg->base));
	}

	return sent;
}

static int uart_ch5xx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	int read;

	for (read = 0; read < size && sys_read8(UART_R8_RFC(cfg->base)) > 0; read++) {
		rx_data[read] = sys_read8(UART_R8_RBR(cfg->base));
	}

	return read;
}

static void uart_ch5xx_irq_tx_enable(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	uint32_t regval;

	regval = sys_read8(UART_R8_IER(cfg->base));
	regval |= IER_THR_EMPTY;
	sys_write8(regval, UART_R8_IER(cfg->base));
}

static void uart_ch5xx_irq_tx_disable(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	uint32_t regval;

	regval = sys_read8(UART_R8_IER(cfg->base));
	regval &= ~IER_THR_EMPTY;
	sys_write8(regval, UART_R8_IER(cfg->base));
}

static int uart_ch5xx_irq_tx_ready(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;

	return sys_read8(UART_R8_TFC(cfg->base)) < UART_FIFO_SIZE;
}

static void uart_ch5xx_irq_rx_enable(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	uint32_t regval;

	regval = sys_read8(UART_R8_IER(cfg->base));
	regval |= IER_RECV_RDY;
	sys_write8(regval, UART_R8_IER(cfg->base));
}

static void uart_ch5xx_irq_rx_disable(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	uint32_t regval;

	regval = sys_read8(UART_R8_IER(cfg->base));
	regval &= ~IER_RECV_RDY;
	sys_write8(regval, UART_R8_IER(cfg->base));
}

static int uart_ch5xx_irq_rx_ready(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;

	return sys_read8(UART_R8_RFC(cfg->base)) > 0;
}

static int uart_ch5xx_irq_is_pending(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	uint32_t regval;

	regval = sys_read8(UART_R8_IIR(cfg->base)) & IIR_INT_MASK;

	return regval != IIR_INT_NOINT;
}

static int uart_ch5xx_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_ch5xx_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *user_data)
{
	struct uart_ch5xx_data *data = dev->data;

	data->cb = cb;
	data->cb_data = user_data;
}

static void uart_ch5xx_isr(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	struct uart_ch5xx_data *data = dev->data;
	uint32_t regval;

	if (data->cb) {
		data->cb(dev, data->cb_data);
	}

	regval = sys_read8(UART_R8_IIR(cfg->base));
	switch (regval & IIR_INT_MASK) {
	case IIR_INT_NOINT:
	case IIR_INT_ADDR:
		break;
	case IIR_INT_LSR:
		if (sys_read8(UART_R8_LSR(cfg->base)) & LSR_ERR_RX_FIFO) {
			uart_irq_err_enable(dev);
		}
		break;
	case IIR_INT_RBR_AVAIL:
		if (sys_read8(UART_R8_LSR(cfg->base)) & LSR_DATA_RDY) {
			uart_irq_rx_ready(dev);
		}
		break;
	case IIR_INT_RBR_TIMEOUT:
		if (sys_read8(UART_R8_LSR(cfg->base)) & LSR_DATA_RDY) {
			uart_irq_rx_ready(dev);
		}
		break;
	case IIR_INT_THR_EMPTY:
		uart_irq_tx_ready(dev);
		break;
	default:
		break;
	}
}
#endif

static int uart_ch5xx_init(const struct device *dev)
{
	const struct uart_ch5xx_config *cfg = dev->config;
	uint32_t regval;
	int ret;

	regval = 10 * cfg->sys_clk_freq * 2 / 16 / cfg->baud_rate;
	regval = (regval + 5) / 10;
	sys_write8(1, UART_R8_DIV(cfg->base));
	sys_write16(regval, UART_R16_DL(cfg->base));

	sys_write8(FCR_FIFO_EN | FCR_RX_FIFO_CLR | FCR_TX_FIFO_CLR, UART_R8_FCR(cfg->base));

	sys_write8(LCR_WORD_SZ_8, UART_R8_LCR(cfg->base));

	sys_write8(IER_TXD_EN, UART_R8_IER(cfg->base));

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	regval = sys_read8(UART_R8_MCR(cfg->base));
	regval |= MCR_INT_OE;
	sys_write8(regval, UART_R8_MCR(cfg->base));

	cfg->irq_config_func();
#endif

	return 0;
}

static const struct uart_driver_api uart_ch5xx_api = {
	.poll_in = uart_ch5xx_poll_in,
	.poll_out = uart_ch5xx_poll_out,
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	.fifo_fill = uart_ch5xx_fifo_fill,
	.fifo_read = uart_ch5xx_fifo_read,
	.irq_tx_enable = uart_ch5xx_irq_tx_enable,
	.irq_tx_disable = uart_ch5xx_irq_tx_disable,
	.irq_tx_ready = uart_ch5xx_irq_tx_ready,
	.irq_rx_enable = uart_ch5xx_irq_rx_enable,
	.irq_rx_disable = uart_ch5xx_irq_rx_disable,
	.irq_rx_ready = uart_ch5xx_irq_rx_ready,
	.irq_is_pending = uart_ch5xx_irq_is_pending,
	.irq_update = uart_ch5xx_irq_update,
	.irq_callback_set = uart_ch5xx_irq_callback_set,
#endif
};

#define UART_CH5XX_INST(n)                                                                         \
	static struct uart_ch5xx_data uart_ch5xx_data_##n;                                         \
                                                                                                   \
	static void uart_ch5xx_irq_config_func_##n(void)                                           \
	{                                                                                          \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			   (IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_ch5xx_isr, \
					DEVICE_DT_INST_GET(n), 0);                                 \
			    irq_enable(DT_INST_IRQN(n));))                                         \
	}                                                                                          \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct uart_ch5xx_config uart_ch5xx_cfg_##n = {                               \
		.irq_config_func = uart_ch5xx_irq_config_func_##n,                                 \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.sys_clk_freq = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
		.baud_rate = DT_INST_PROP(n, current_speed),                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_ch5xx_init, NULL, &uart_ch5xx_data_##n, &uart_ch5xx_cfg_##n, \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_ch5xx_api);

DT_INST_FOREACH_STATUS_OKAY(UART_CH5XX_INST)
