/*
 * Copyright (c) Core Devices LLC
 * Copyright (c) Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_usart

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

#include <register.h>

#define UART_CR1   offsetof(USART_TypeDef, CR1)
#define UART_CR2   offsetof(USART_TypeDef, CR2)
#define UART_CR3   offsetof(USART_TypeDef, CR3)
#define UART_BRR   offsetof(USART_TypeDef, BRR)
#define UART_ISR   offsetof(USART_TypeDef, ISR)
#define UART_ICR   offsetof(USART_TypeDef, ICR)
#define UART_RDR   offsetof(USART_TypeDef, RDR)
#define UART_TDR   offsetof(USART_TypeDef, TDR)
#define UART_MISCR offsetof(USART_TypeDef, MISCR)

#define UART_CR1_M_6B FIELD_PREP(USART_CR1_M_Msk, 0U)
#define UART_CR1_M_7B FIELD_PREP(USART_CR1_M_Msk, 1U)
#define UART_CR1_M_8B FIELD_PREP(USART_CR1_M_Msk, 2U)
#define UART_CR1_M_9B FIELD_PREP(USART_CR1_M_Msk, 3U)

#define UART_CR2_STOP_1B FIELD_PREP(USART_CR2_STOP_Msk, 0U)
#define UART_CR2_STOP_2B FIELD_PREP(USART_CR2_STOP_Msk, 1U)

/* minimal BRR: INT=1, FRAC=0 (0x10) */
#define UART_BRR_MIN 0x10U

struct uart_sf32lb_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_callback;
	void *cb_data;
#endif
};

struct uart_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sf32lb_isr(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	struct uart_sf32lb_data *data = dev->data;

	if (data->irq_callback) {
		data->irq_callback(dev, data->cb_data);
	}

	sys_write32(USART_ISR_TXE | USART_ICR_TCCF | USART_ISR_RXNE, config->base + UART_ICR);
}
#endif

static int uart_sf32lb_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_sf32lb_config *config = dev->config;
	enum uart_config_data_bits data_bits = cfg->data_bits;
	uint32_t cr1, cr2, cr3, brr, miscr;

	/* CR1: disable USART */
	cr1 = sys_read32(config->base + UART_CR1);
	cr1 &= ~USART_CR1_UE;
	sys_write32(cr1, config->base + UART_CR1);

	/* CR1: data bits, parity, oversampling */
	cr1 &= ~(USART_CR1_M_Msk | USART_CR1_PCE_Msk | USART_CR1_PS_Msk | USART_CR1_OVER8_Msk);

	/* data bits include parity bit */
	if (cfg->parity != UART_CFG_PARITY_NONE) {
		data_bits++;
		if (data_bits > UART_CFG_DATA_BITS_9) {
			return -ENOTSUP;
		}
	}

	switch (data_bits) {
	case UART_CFG_DATA_BITS_6:
		cr1 |= UART_CR1_M_6B;
		break;
	case UART_CFG_DATA_BITS_7:
		cr1 |= UART_CR1_M_7B;
		break;
	case UART_CFG_DATA_BITS_8:
		cr1 |= UART_CR1_M_8B;
		break;
	case UART_CFG_DATA_BITS_9:
		cr1 |= UART_CR1_M_9B;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		cr1 |= (USART_CR1_PCE | USART_CR1_PS);
		break;
	case UART_CFG_PARITY_EVEN:
		cr1 |= USART_CR1_PCE;
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(cr1, config->base + UART_CR1);

	/* CR2: stop bits */
	cr2 = sys_read32(config->base + UART_CR2);
	cr2 &= ~USART_CR2_STOP_Msk;

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		cr2 |= UART_CR2_STOP_1B;
		break;
	case UART_CFG_STOP_BITS_2:
		cr2 |= UART_CR2_STOP_2B;
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(cr2, config->base + UART_CR2);

	/* CR3: flow control */
	cr3 = sys_read32(config->base + UART_CR3);
	cr3 &= ~(USART_CR3_RTSE_Msk | USART_CR3_CTSE_Msk);

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		cr3 |= (USART_CR3_RTSE_Msk | USART_CR3_CTSE_Msk);
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(cr3, config->base + UART_CR3);

	/* enable USART */
	cr1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
	sys_write32(cr1, config->base + UART_CR1);

	/* BRR: baudrate */
	miscr = sys_read32(config->base + UART_MISCR);
	miscr &= ~USART_MISCR_SMPLINI_Msk;

	brr = 48000000UL / config->uart_cfg.baudrate;
	if (brr < UART_BRR_MIN) {
		cr1 |= USART_CR1_OVER8;
		sys_write32(cr1, config->base + UART_CR1);
		/* recalculate brr with reduced oversampling */
		brr = (48000000UL * 2U) / config->uart_cfg.baudrate;
		miscr |= FIELD_PREP(USART_MISCR_SMPLINI_Msk, 2U);
	} else {
		miscr |= FIELD_PREP(USART_MISCR_SMPLINI_Msk, 6U);
	}

	sys_write32(miscr, config->base + UART_MISCR);
	sys_write32(brr, config->base + UART_BRR);

	return 0;
}

static int uart_sf32lb_poll_in(const struct device *dev, uint8_t *c)
{
	const struct uart_sf32lb_config *config = dev->config;

	if ((sys_read32(config->base + UART_ISR) & USART_ISR_RXNE) != 0U) {
		*c = sys_read32(config->base + UART_RDR) & 0xFFU;
		return 0;
	}

	return -1;
}

static void uart_sf32lb_poll_out(const struct device *dev, uint8_t c)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_write32(USART_ISR_TC, config->base + UART_ICR);
	sys_write8(c, config->base + UART_TDR);

	while ((sys_read32(config->base + UART_ISR) & USART_ISR_TC) == 0U) {
	}
}

static int uart_sf32lb_err_check(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	uint32_t isr = sys_read32(config->base + UART_ISR);
	int err = 0;

	if (isr & USART_ISR_ORE) {
		err |= UART_ERROR_OVERRUN;
	}

	if (isr & USART_ISR_PE) {
		err |= UART_ERROR_PARITY;
	}

	if (isr & USART_ISR_FE) {
		err |= UART_ERROR_FRAMING;
	}

	if (isr & USART_ISR_NF) {
		err |= UART_ERROR_NOISE;
	}

	/* clear error flags */
	sys_write32(USART_ICR_ORECF | USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF,
		    config->base + UART_ICR);

	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_sf32lb_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_sf32lb_config *config = dev->config;
	int i;

	for (i = 0; i < len; i++) {
		if (!sys_test_bit(config->base + UART_ISR, USART_ISR_TXE_Pos)) {
			break;
		}
		sys_write8(tx_data[i], config->base + UART_TDR);
	}

	return i;
}

static int uart_sf32lb_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_sf32lb_config *config = dev->config;
	int i;

	for (i = 0; i < size; i++) {
		if (!sys_test_bit(config->base + UART_ISR, USART_ISR_RXNE_Pos)) {
			break;
		}
		rx_data[i] = sys_read8(config->base + UART_RDR);
	}

	return i;
}

static void uart_sf32lb_irq_tx_enable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + UART_CR1, USART_CR1_TXEIE_Pos);
}

static void uart_sf32lb_irq_tx_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + UART_CR1, USART_CR1_TXEIE_Pos);
}

static int uart_sf32lb_irq_tx_ready(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_test_bit(config->base + UART_ISR, USART_ISR_TXE_Pos);
}

static int uart_sf32lb_irq_tx_complete(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_test_bit(config->base + UART_ISR, USART_ISR_TC_Pos);
}

static int uart_sf32lb_irq_rx_ready(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_test_bit(config->base + UART_ISR, USART_ISR_RXNE_Pos);
}

static void uart_sf32lb_irq_err_enable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + UART_CR1, USART_CR1_PEIE_Pos);
	sys_set_bit(config->base + UART_CR3, USART_CR3_EIE_Pos);
}

static void uart_sf32lb_irq_err_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + UART_CR1, USART_CR1_PEIE_Pos);
	sys_clear_bit(config->base + UART_CR3, USART_CR3_EIE_Pos);
}

static int uart_sf32lb_irq_is_pending(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	return sys_read32(config->base + UART_ISR) == 0U ? 0 : 1;
}

static int uart_sf32lb_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

static void uart_sf32lb_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_sf32lb_data *data = dev->data;

	data->irq_callback = cb;
	data->cb_data = user_data;
}

static void uart_sf32lb_irq_rx_enable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + UART_CR1, USART_CR1_RXNEIE_Pos);
}

static void uart_sf32lb_irq_rx_disable(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + UART_CR1, USART_CR1_RXNEIE_Pos);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_sf32lb_api = {
	.poll_in = uart_sf32lb_poll_in,
	.poll_out = uart_sf32lb_poll_out,
	.err_check = uart_sf32lb_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_sf32lb_fifo_fill,
	.fifo_read = uart_sf32lb_fifo_read,
	.irq_tx_enable = uart_sf32lb_irq_tx_enable,
	.irq_tx_disable = uart_sf32lb_irq_tx_disable,
	.irq_tx_complete = uart_sf32lb_irq_tx_complete,
	.irq_tx_ready = uart_sf32lb_irq_tx_ready,
	.irq_rx_enable = uart_sf32lb_irq_rx_enable,
	.irq_rx_disable = uart_sf32lb_irq_rx_disable,
	.irq_rx_ready = uart_sf32lb_irq_rx_ready,
	.irq_err_enable = uart_sf32lb_irq_err_enable,
	.irq_err_disable = uart_sf32lb_irq_err_disable,
	.irq_is_pending = uart_sf32lb_irq_is_pending,
	.irq_update = uart_sf32lb_irq_update,
	.irq_callback_set = uart_sf32lb_irq_callback_set,
#endif
};

static int uart_sf32lb_init(const struct device *dev)
{
	const struct uart_sf32lb_config *config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (config->clock.dev != NULL) {
		if (!sf32lb_clock_is_ready_dt(&config->clock)) {
			return -ENODEV;
		}

		ret = sf32lb_clock_control_on_dt(&config->clock);
		if (ret < 0) {
			return ret;
		}
	}

	ret = uart_sf32lb_configure(dev, &config->uart_cfg);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#define SF32LB_UART_DEFINE(index)                                                                  \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                                   \
	(static void uart_sf32lb_irq_config_func_##index(const struct device *dev)                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), uart_sf32lb_isr,    \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}));                                                                                       \
                                                                                                   \
	static const struct uart_sf32lb_config uart_sf32lb_cfg_##index = {                         \
		.base = DT_INST_REG_ADDR(index),                                                   \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET_OR(index, {}),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity =                                                          \
					DT_INST_ENUM_IDX_OR(index, parity, UART_CFG_PARITY_NONE),  \
				.stop_bits = DT_INST_ENUM_IDX_OR(index, stop_bits,                 \
								 UART_CFG_STOP_BITS_1),            \
				.data_bits = DT_INST_ENUM_IDX_OR(index, data_bits,                 \
								 UART_CFG_DATA_BITS_8),            \
				.flow_ctrl = DT_INST_PROP(index, hw_flow_control)                  \
						     ? UART_CFG_FLOW_CTRL_RTS_CTS                  \
						     : UART_CFG_FLOW_CTRL_NONE,                    \
			},                                                                         \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			(.irq_config_func = uart_sf32lb_irq_config_func_##index,))                 \
	};                                                                                         \
                                                                                                   \
	static struct uart_sf32lb_data uart_sf32lb_data_##index;                                   \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, uart_sf32lb_init, NULL,                                       \
		&uart_sf32lb_data_##index, &uart_sf32lb_cfg_##index, PRE_KERNEL_1,                 \
		CONFIG_SERIAL_INIT_PRIORITY, &uart_sf32lb_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_UART_DEFINE)
