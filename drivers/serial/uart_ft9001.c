/*
 * Copyright (c) 2025, FocalTech Systems Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT focaltech_ft9001_usart

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(uart_ft9001, CONFIG_UART_LOG_LEVEL);

/* Control Register 1 bits */
#define SCICR1_PT_MASK 0x01 /* Parity type */
#define SCICR1_PE_MASK 0x02 /* Parity enable */
#define SCICR1_M_MASK  0x10 /* Frame length */

/* Control Register 2 bits */
#define SCICR2_RE_MASK 0x04 /* Receiver enable */
#define SCICR2_TE_MASK 0x08 /* Transmitter enable */

/* FIFO Control Register bits */
#define SCIFCR_TFEN        0x40 /* TX FIFO enable */
#define SCIFCR_RFEN        0x80 /* RX FIFO enable */
#define SCIFCR_RXFLSEL_1_8 0x00 /* RX FIFO trigger level */
#define SCIFCR_TXFLSEL_1_8 0x04 /* TX FIFO trigger level */

/* FIFO Control Register 2 bits */
#define SCIFCR2_RXFCLR  0x01 /* RX FIFO clear */
#define SCIFCR2_TXFCLR  0x02 /* TX FIFO clear */
#define SCIFCR2_RXFTOE  0x04 /* RX FIFO timeout enable */
#define SCIFCR2_RXFTOIE 0x08 /* RX FIFO timeout interrupt enable */
#define SCIFCR2_RXORIE  0x10 /* RX overrun interrupt enable */
#define SCIFCR2_RXFIE   0x20 /* RX FIFO interrupt enable */
#define SCIFCR2_TXFIE   0x80 /* TX FIFO interrupt enable */

/* FIFO Status Register bits */
#define SCIFSR_REMPTY_MASK 0x01 /* RX FIFO empty */
#define SCIFSR_TEMPTY_MASK 0x04 /* TX FIFO empty */
#define SCIFSR_TFULL_MASK  0x08 /* TX FIFO full */
#define SCIFSR_RFTS_MASK   0x20 /* RX FIFO trigger level reached */
#define SCIFSR_FTC_MASK    0x40 /* Frame transmission complete */

/* FIFO Status Register 2 bits */
#define SCIFSR2_FXPF_MASK 0x01 /* Parity error */
#define SCIFSR2_FXFE_MASK 0x02 /* Frame error */
#define SCIFSR2_FXNF_MASK 0x04 /* Noise error */
#define SCIFSR2_FXOR_MASK 0x08 /* FIFO overrun */
#define SCIFSR2_W1C_MASK                                                                           \
	(SCIFSR2_FXPF_MASK | SCIFSR2_FXFE_MASK | SCIFSR2_FXNF_MASK | SCIFSR2_FXOR_MASK)

#define UART_DATA_FRAME_LEN_10BIT 0
#define UART_DATA_FRAME_LEN_11BIT SCICR1_M_MASK

#define SCIBDL_OFFSET  0x00
#define SCIBDH_OFFSET  0x01
#define SCICR2_OFFSET  0x02
#define SCICR1_OFFSET  0x03
#define SCIDRL_OFFSET  0x06
#define SCIBRDF_OFFSET 0x0A
#define SCIFCR_OFFSET  0x0E
#define SCIFSR_OFFSET  0x11
#define SCIFCR2_OFFSET 0x13
#define SCIFSR2_OFFSET 0x15

struct uart_ft9001_config {
	mm_reg_t base;
	uint32_t clkid;
	struct reset_dt_spec reset;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif
};

struct uart_ft9001_data {
	struct uart_config uart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
};

#define DEV_CFG(dev)  ((const struct uart_ft9001_config *)(dev)->config)
#define DEV_DATA(dev) ((struct uart_ft9001_data *)(dev)->data)

/**
 * @brief Configure UART hardware
 */
static void uart_ft9001_hw_init(mm_reg_t base, const struct uart_config *cfg, uint32_t sys_freq)
{
	uint32_t scaled_baud;
	uint16_t bauddiv_i;
	uint8_t bauddiv_f;
	uint8_t bauddiv_h;
	uint8_t bauddiv_l;
	uint8_t value;

	/* Calculate baud rate divisors */
	scaled_baud = (uint32_t)((sys_freq * 4u) / cfg->baudrate);
	bauddiv_i = (uint16_t)(scaled_baud >> 6);
	bauddiv_f = (uint8_t)((((scaled_baud << 1) + 1u) >> 1) & 0x3f);
	bauddiv_h = (uint8_t)((bauddiv_i >> 8) & 0xff);
	bauddiv_l = (uint8_t)(bauddiv_i & 0xff);

	/* Disable UART */
	sys_write8(0, base + SCICR2_OFFSET);
	sys_write8(0, base + SCIFCR_OFFSET);

	/* Enable FIFO mode */
	sys_write8(SCIFCR_RFEN | SCIFCR_TFEN, base + SCIFCR_OFFSET);

	/* Set baud rate (write fraction before integer) */
	sys_write8(bauddiv_f, base + SCIBRDF_OFFSET);
	sys_write8(bauddiv_h, base + SCIBDH_OFFSET);
	sys_write8(bauddiv_l, base + SCIBDL_OFFSET);

	/* Configure frame format */
	value = 0;

	if (cfg->data_bits == UART_CFG_DATA_BITS_9) {
		value |= UART_DATA_FRAME_LEN_11BIT;
	} else {
		value |= UART_DATA_FRAME_LEN_10BIT;
	}

	/* Configure parity */
	if (cfg->parity != UART_CFG_PARITY_NONE) {
		value |= SCICR1_PE_MASK;

		if (cfg->parity == UART_CFG_PARITY_ODD) {
			value |= SCICR1_PT_MASK;
		}
	}

	sys_write8(value, base + SCICR1_OFFSET);

	/* Set FIFO waterlines */
	value = sys_read8(base + SCIFCR_OFFSET);
	value |= (SCIFCR_RXFLSEL_1_8 | SCIFCR_TXFLSEL_1_8);
	sys_write8(value, base + SCIFCR_OFFSET);

	/* Configure FIFO control 2 */
	value = (SCIFCR2_RXFTOE | SCIFCR2_RXFCLR | SCIFCR2_TXFCLR);
	sys_write8(value, base + SCIFCR2_OFFSET);

	/* Clear status registers */
	sys_write8(SCIFSR2_W1C_MASK, base + SCIFSR2_OFFSET);

	/* Enable transmitter and receiver */
	value = sys_read8(base + SCICR2_OFFSET);
	value |= (SCICR2_TE_MASK | SCICR2_RE_MASK);
	sys_write8(value, base + SCICR2_OFFSET);
}

/**
 * @brief Initialize the UART device
 */
static int uart_ft9001_init(const struct device *dev)
{
	const struct device *clk = DEVICE_DT_GET(DT_NODELABEL(cpm));
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	struct uart_ft9001_data *data = DEV_DATA(dev);
	uint32_t sys_freq;
	uint8_t value;
	int ret;

	/* Enable UART clock */
	ret = clock_control_on(clk, (clock_control_subsys_t *)(uintptr_t)config->clkid);
	if (ret < 0) {
		return ret;
	}

	/* Get system frequency */
	ret = clock_control_get_rate(clk, (clock_control_subsys_t *)(uintptr_t)config->clkid,
				     &sys_freq);
	if (ret < 0) {
		sys_freq = 160000000 / 2;
	}

	/* Toggle reset line */
	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		return ret;
	}

	/* Initialize hardware */
	uart_ft9001_hw_init(config->base, &data->uart_cfg, sys_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Configure interrupts */
	config->irq_config_func(dev);
	/* Initially disable all interrupts */
	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value &= ~(SCIFCR2_TXFIE | SCIFCR2_RXFIE | SCIFCR2_RXFTOIE | SCIFCR2_RXORIE);
	sys_write8(value, config->base + SCIFCR2_OFFSET);
#endif

	return 0;
}

/**
 * @brief Read a character from UART (polling mode)
 */
static int uart_ft9001_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	if (sys_read8(config->base + SCIFSR_OFFSET) & SCIFSR_REMPTY_MASK) {
		return -1; /* No data available */
	}

	*c = sys_read8(config->base + SCIDRL_OFFSET);

	return 0;
}

/**
 * @brief Send a character via UART (polling mode)
 */
static void uart_ft9001_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	/* Wait for TX FIFO not full */
	while (sys_read8(config->base + SCIFSR_OFFSET) & SCIFSR_TFULL_MASK) {
		/* Wait */
	}

	/* Send character */
	sys_write8(c, config->base + SCIDRL_OFFSET);

	/* Wait for transmission complete */
	while (1) {
		value = sys_read8(config->base + SCIFSR_OFFSET);
		if ((value & SCIFSR_TEMPTY_MASK) && (value & SCIFSR_FTC_MASK)) {
			break;
		}
	}
}

/**
 * @brief Check for UART errors
 */
static int uart_ft9001_err_check(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;
	int err = 0;

	/* Check error flags */
	value = sys_read8(config->base + SCIFSR2_OFFSET);
	if (value & SCIFSR2_FXOR_MASK) {
		err |= UART_ERROR_OVERRUN;
	}
	if (value & SCIFSR2_FXPF_MASK) {
		err |= UART_ERROR_PARITY;
	}
	if (value & SCIFSR2_FXFE_MASK) {
		err |= UART_ERROR_FRAMING;
	}
	if (value & SCIFSR2_FXNF_MASK) {
		err |= UART_ERROR_NOISE;
	}

	/* Clear error flags */
	sys_write8(value & SCIFSR2_W1C_MASK, config->base + SCIFSR2_OFFSET);

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/**
 * @brief Configure UART parameters at runtime
 */
static int uart_ft9001_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	struct uart_ft9001_data *data = DEV_DATA(dev);
	const struct device *clk = DEVICE_DT_GET(DT_NODELABEL(cpm));
	uint32_t sys_freq;
	int ret;

	/* Validate configuration */
	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}
	if (cfg->data_bits != UART_CFG_DATA_BITS_8 && cfg->data_bits != UART_CFG_DATA_BITS_9) {
		return -ENOTSUP;
	}
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	/* Get clock frequency */
	ret = clock_control_get_rate(clk, (clock_control_subsys_t *)(uintptr_t)config->clkid,
				     &sys_freq);
	if (ret < 0) {
		sys_freq = 160000000 / 2;
	}

	/* Reconfigure hardware */
	uart_ft9001_hw_init(config->base, cfg, sys_freq);

	/* Update stored configuration */
	data->uart_cfg = *cfg;

	return 0;
}

/**
 * @brief Get current UART configuration
 */
static int uart_ft9001_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_ft9001_data *data = DEV_DATA(dev);

	*cfg = data->uart_cfg;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Fill TX FIFO with data
 */
static int uart_ft9001_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	int num_tx;

	for (num_tx = 0; num_tx < size; num_tx++) {
		if (sys_read8(config->base + SCIFSR_OFFSET) & SCIFSR_TFULL_MASK) {
			break; /* TX FIFO full */
		}
		sys_write8(tx_data[num_tx], config->base + SCIDRL_OFFSET);
	}

	return num_tx;
}

/**
 * @brief Read data from RX FIFO
 */
static int uart_ft9001_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	int i = 0;

	while (!(sys_read8(config->base + SCIFSR_OFFSET) & SCIFSR_REMPTY_MASK) && (i < size)) {
		rx_data[i] = sys_read8(config->base + SCIDRL_OFFSET);
		i++;
	}

	return i;
}

/**
 * @brief Enable TX interrupt
 */
static void uart_ft9001_irq_tx_enable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value |= SCIFCR2_TXFIE;
	sys_write8(value, config->base + SCIFCR2_OFFSET);
}

/**
 * @brief Disable TX interrupt
 */
static void uart_ft9001_irq_tx_disable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value &= ~SCIFCR2_TXFIE;
	sys_write8(value, config->base + SCIFCR2_OFFSET);
}

/**
 * @brief Check if TX is ready
 */
static int uart_ft9001_irq_tx_ready(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFSR_OFFSET);
	value &= SCIFSR_TFULL_MASK;

	return !value;
}

/**
 * @brief Enable RX interrupt
 */
static void uart_ft9001_irq_rx_enable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value |= (SCIFCR2_RXFIE | SCIFCR2_RXFTOIE);
	sys_write8(value, config->base + SCIFCR2_OFFSET);
}

/**
 * @brief Disable RX interrupt
 */
static void uart_ft9001_irq_rx_disable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value &= ~(SCIFCR2_RXFIE | SCIFCR2_RXFTOIE);
	sys_write8(value, config->base + SCIFCR2_OFFSET);
}

/**
 * @brief Check if TX is complete
 */
static int uart_ft9001_irq_tx_complete(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFSR_OFFSET);

	return ((value & SCIFSR_TEMPTY_MASK) && (value & SCIFSR_FTC_MASK));
}

/**
 * @brief Check if RX data is ready
 */
static int uart_ft9001_irq_rx_ready(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFSR_OFFSET);

	return ((value & SCIFSR_RFTS_MASK) || !(value & SCIFSR_REMPTY_MASK));
}

/**
 * @brief Enable error interrupt
 */
static void uart_ft9001_irq_err_enable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value |= (SCIFCR2_RXORIE | SCIFCR2_RXFTOIE);
	sys_write8(value, config->base + SCIFCR2_OFFSET);
}

/**
 * @brief Disable error interrupt
 */
static void uart_ft9001_irq_err_disable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t value;

	value = sys_read8(config->base + SCIFCR2_OFFSET);
	value &= ~(SCIFCR2_RXORIE | SCIFCR2_RXFTOIE);
	sys_write8(value, config->base + SCIFCR2_OFFSET);
}

/**
 * @brief Check if interrupt is pending
 */
static int uart_ft9001_irq_is_pending(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);
	uint8_t fsr, fcr2;

	fsr = sys_read8(config->base + SCIFSR_OFFSET);
	fcr2 = sys_read8(config->base + SCIFCR2_OFFSET);

	/* Check if TX or RX interrupts are pending */
	return ((!(fsr & SCIFSR_TFULL_MASK) && (fcr2 & SCIFCR2_TXFIE)) ||
		(((fsr & SCIFSR_RFTS_MASK) || !(fsr & SCIFSR_REMPTY_MASK)) &&
		 (fcr2 & SCIFCR2_RXFIE)));
}

/**
 * @brief Update interrupt status
 */
static int uart_ft9001_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set interrupt callback
 */
static void uart_ft9001_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_ft9001_data *data = DEV_DATA(dev);

	data->cb = cb;
	data->cb_data = cb_data;
}

/**
 * @brief UART interrupt service routine
 */
static void uart_ft9001_isr(const struct device *dev)
{
	struct uart_ft9001_data *data = DEV_DATA(dev);

	if (data->cb) {
		data->cb(dev, data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/* UART driver API structure */
static DEVICE_API(uart, uart_ft9001_driver_api) = {
	.poll_in = uart_ft9001_poll_in,
	.poll_out = uart_ft9001_poll_out,
	.err_check = uart_ft9001_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ft9001_configure,
	.config_get = uart_ft9001_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ft9001_fifo_fill,
	.fifo_read = uart_ft9001_fifo_read,
	.irq_tx_enable = uart_ft9001_irq_tx_enable,
	.irq_tx_disable = uart_ft9001_irq_tx_disable,
	.irq_tx_ready = uart_ft9001_irq_tx_ready,
	.irq_rx_enable = uart_ft9001_irq_rx_enable,
	.irq_rx_disable = uart_ft9001_irq_rx_disable,
	.irq_tx_complete = uart_ft9001_irq_tx_complete,
	.irq_rx_ready = uart_ft9001_irq_rx_ready,
	.irq_err_enable = uart_ft9001_irq_err_enable,
	.irq_err_disable = uart_ft9001_irq_err_disable,
	.irq_is_pending = uart_ft9001_irq_is_pending,
	.irq_update = uart_ft9001_irq_update,
	.irq_callback_set = uart_ft9001_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define FOCALTECH_UART_IRQ_HANDLER(idx)                                                            \
	static void uart_ft9001_cfg_func_##idx(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), uart_ft9001_isr,        \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
	}
#define FOCALTECH_UART_IRQ_HANDLER_FUNC_INIT(idx) .irq_config_func = uart_ft9001_cfg_func_##idx,
#else
#define FOCALTECH_UART_IRQ_HANDLER(idx)
#define FOCALTECH_UART_IRQ_HANDLER_FUNC_INIT(idx)
#endif

/**
 * @brief Macro to define a FocalTech FT9001 UART device instance
 */
#define UART_FOCALTECH_FT9001_DEVICE(idx)                                                          \
	FOCALTECH_UART_IRQ_HANDLER(idx)                                                            \
                                                                                                   \
	static struct uart_ft9001_data uart_ft9001_data_##idx = {                                  \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_PROP(idx, current_speed),                      \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const struct uart_ft9001_config uart_ft9001_cfg_##idx = {                           \
		.base = (mm_reg_t)DT_INST_REG_ADDR(idx),                                           \
		.clkid = DT_INST_CLOCKS_CELL(idx, id),                                             \
		.reset = RESET_DT_SPEC_INST_GET(idx),                                              \
		FOCALTECH_UART_IRQ_HANDLER_FUNC_INIT(idx)};                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_ft9001_init, NULL, &uart_ft9001_data_##idx,                \
			      &uart_ft9001_cfg_##idx, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,   \
			      (void *)&uart_ft9001_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_FOCALTECH_FT9001_DEVICE);
