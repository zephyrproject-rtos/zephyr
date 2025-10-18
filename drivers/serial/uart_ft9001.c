/*
 * Copyright (c) 2025, FocalTech Systems Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UART driver for FocalTech FT9001 SoC (register-based implementation)
 *
 * This driver provides UART support using direct register operations based on
 * the FocalTech FT9001 UART hardware specification.
 */

#define DT_DRV_COMPAT focaltech_ft9001_usart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include <errno.h>
#include <soc.h>

LOG_MODULE_REGISTER(uart_ft9001, CONFIG_UART_LOG_LEVEL);

/* FT9001 UART register structure (based on HAL) */
struct ft9001_uart_regs {
	__IO uint8_t SCIBDL;     /* 0x00 - Baud Rate Low */
	__IO uint8_t SCIBDH;     /* 0x01 - Baud Rate High */
	__IO uint8_t SCICR2;     /* 0x02 - Control Register 2 */
	__IO uint8_t SCICR1;     /* 0x03 - Control Register 1 */
	__IO uint8_t SCISR2;     /* 0x04 - Status Register 2 */
	__IO uint8_t SCISR1;     /* 0x05 - Status Register 1 */
	__IO uint8_t SCIDRL;     /* 0x06 - Data Register Low */
	__IO uint8_t SCIDRH;     /* 0x07 - Data Register High */
	__IO uint8_t SCIPORT;    /* 0x08 - Port Register */
	__IO uint8_t SCIPURD;    /* 0x09 - Pull-up Disable Register */
	__IO uint8_t SCIBRDF;    /* 0x0A - Baud Rate Fraction */
	__IO uint8_t SCIDDR;     /* 0x0B - Data Direction Register */
	__IO uint8_t SCIIRCR;    /* 0x0C - IrDA Control Register */
	__IO uint8_t SCITR;      /* 0x0D - Test Register */
	__IO uint8_t SCIFCR;     /* 0x0E - FIFO Control Register */
	__IO uint8_t SCIIRDR;    /* 0x0F - IrDA Data Register */
	__IO uint8_t SCIDCR;     /* 0x10 - DMA Control Register */
	__IO uint8_t SCIFSR;     /* 0x11 - FIFO Status Register */
	__IO uint8_t SCIRXTOCTR; /* 0x12 - RX Timeout Counter */
	__IO uint8_t SCIFCR2;    /* 0x13 - FIFO Control Register 2 */
	__IO uint8_t SCIFCTRL;   /* 0x14 - FIFO Control Register */
	__IO uint8_t SCIFSR2;    /* 0x15 - FIFO Status Register 2 */
};

/* Control Register 1 bits */
#define SCICR1_M_MASK  0x10 /* Frame length */
#define SCICR1_PE_MASK 0x02 /* Parity enable */
#define SCICR1_PT_MASK 0x01 /* Parity type */

/* Control Register 2 bits */
#define SCICR2_TE_MASK 0x08 /* Transmitter enable */
#define SCICR2_RE_MASK 0x04 /* Receiver enable */

/* FIFO Control Register bits */
#define SCIFCR_RFEN        0x80 /* RX FIFO enable */
#define SCIFCR_TFEN        0x40 /* TX FIFO enable */
#define SCIFCR_RXFLSEL_1_8 0x00 /* RX FIFO trigger level */
#define SCIFCR_TXFLSEL_1_8 0x04 /* TX FIFO trigger level */

/* FIFO Control Register 2 bits */
#define SCIFCR2_RXFTOE  0x04 /* RX FIFO timeout enable */
#define SCIFCR2_TXFCLR  0x02 /* TX FIFO clear */
#define SCIFCR2_RXFCLR  0x01 /* RX FIFO clear */
#define SCIFCR2_TXFIE   0x80 /* TX FIFO interrupt enable */
#define SCIFCR2_RXFIE   0x20 /* RX FIFO interrupt enable */
#define SCIFCR2_RXFTOIE 0x08 /* RX FIFO timeout interrupt enable */
#define SCIFCR2_RXORIE  0x10 /* RX overrun interrupt enable */

/* FIFO Status Register bits */
#define SCIFSR_TFULL_MASK  0x08 /* TX FIFO full */
#define SCIFSR_TEMPTY_MASK 0x04 /* TX FIFO empty */
#define SCIFSR_REMPTY_MASK 0x01 /* RX FIFO empty */
#define SCIFSR_RFTS_MASK   0x20 /* RX FIFO trigger level reached */
#define SCIFSR_FTC_MASK    0x40 /* Frame transmission complete */

/* Status Register 2 bits */
#define SCISR2_FOR_MASK 0x08 /* FIFO overrun */
#define SCISR2_FPF_MASK 0x01 /* Parity error */
#define SCISR2_FNF_MASK 0x04 /* Noise error */
#define SCISR2_FFE_MASK 0x02 /* Frame error */

/* HAL compatibility defines */
#define UART_DATA_FRAME_LEN_10BIT 0
#define UART_DATA_FRAME_LEN_11BIT SCICR1_M_MASK
#define UART_PARITY_NONE          2
#define UART_PARITY_EVE           0
#define UART_PARITY_ODD           SCICR1_PT_MASK
#define UART_INT_MODE             1

/**
 * @brief UART device configuration structure
 */
struct uart_ft9001_config {
	struct ft9001_uart_regs *base; /* UART base address */
	uint32_t clkid;                /* Clock control ID */
	struct reset_dt_spec reset;    /* Reset control */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func; /* IRQ config function */
#endif
};

/**
 * @brief UART device runtime data structure
 */
struct uart_ft9001_data {
	struct uart_config uart_cfg; /* UART configuration */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /* Callback function */
	void *cb_data;                    /* Callback data */
#endif
};

#define DEV_CFG(dev)  ((const struct uart_ft9001_config *)(dev)->config)
#define DEV_DATA(dev) ((struct uart_ft9001_data *)(dev)->data)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_ft9001_isr(const struct device *dev);
#endif

/**
 * @brief Configure UART hardware (based on HAL UART_Init)
 */
static void uart_ft9001_hw_init(struct ft9001_uart_regs *base, const struct uart_config *cfg,
				uint32_t sys_freq)
{
	uint16_t bandrate_i;
	uint8_t bandrate_f;
	uint8_t bandrate_h;
	uint8_t bandrate_l;

	/* Calculate baud rate divisors (from HAL) */
	bandrate_i = ((uint16_t)(sys_freq * 4 / cfg->baudrate)) >> 6;
	bandrate_f = (((uint16_t)(sys_freq * 8 / cfg->baudrate) + 1) / 2) & 0x003f;
	bandrate_h = (uint8_t)((bandrate_i >> 8) & 0x00ff);
	bandrate_l = (uint8_t)(bandrate_i & 0x00ff);

	/* Disable UART */
	base->SCICR2 = 0;
	base->SCIFCR = 0;

	/* Enable FIFO mode */
	base->SCIFCR = SCIFCR_RFEN | SCIFCR_TFEN;

	/* Set baud rate (write fraction before integer) */
	base->SCIBRDF = bandrate_f;
	base->SCIBDH = bandrate_h;
	base->SCIBDL = bandrate_l;

	/* Configure frame format */
	base->SCICR1 = 0x00;
	if (cfg->data_bits == UART_CFG_DATA_BITS_9) {
		base->SCICR1 |= UART_DATA_FRAME_LEN_11BIT;
	} else {
		base->SCICR1 |= UART_DATA_FRAME_LEN_10BIT;
	}

	/* Configure parity */
	if (cfg->parity == UART_CFG_PARITY_NONE) {
		/* Parity disabled - no additional bits to set */
	} else {
		base->SCICR1 |= SCICR1_PE_MASK; /* Enable parity */
		if (cfg->parity == UART_CFG_PARITY_ODD) {
			base->SCICR1 |= SCICR1_PT_MASK; /* Odd parity */
		}
		/* Even parity is default when PE=1, PT=0 */
	}

	/* Set FIFO waterlines */
	base->SCIFCR |= (SCIFCR_RXFLSEL_1_8 | SCIFCR_TXFLSEL_1_8);
	base->SCIDRL = 0; /* Clear data register */

	/* Configure FIFO control 2 */
	base->SCIFCR2 = 0;
	base->SCIFCR2 |= SCIFCR2_RXFTOE;                    /* Enable RX FIFO timeout */
	base->SCIFCR2 |= (SCIFCR2_RXFCLR | SCIFCR2_TXFCLR); /* Clear FIFOs */

	/* Clear status registers */
	base->SCIFSR2 = base->SCIFSR2;

	/* Enable transmitter and receiver */
	base->SCICR2 |= SCICR2_TE_MASK | SCICR2_RE_MASK;
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
	int ret = 0;

	LOG_DBG("Initializing UART at base %p", config->base);

	/* Enable UART clock */
	ret = clock_control_on(clk, (clock_control_subsys_t *)(uintptr_t)config->clkid);
	if (ret < 0) {
		LOG_ERR("Could not enable UART clock");
		return ret;
	}

	/* Get system frequency */
	ret = clock_control_get_rate(clk, (clock_control_subsys_t *)(uintptr_t)config->clkid,
				     &sys_freq);
	if (ret < 0) {
		LOG_WRN("Could not get UART clock rate, using default");
		sys_freq = 160000000 / 2; /* Default assumption */
	}

	/* Toggle reset line */
	ret = reset_line_toggle_dt(&config->reset);
	if (ret < 0) {
		LOG_ERR("UART reset failed");
		return ret;
	}

	/* Initialize hardware */
	uart_ft9001_hw_init(config->base, &data->uart_cfg, sys_freq);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Configure interrupts */
	config->irq_config_func(dev);
	/* Initially disable all interrupts */
	config->base->SCIFCR2 &=
		~(SCIFCR2_TXFIE | SCIFCR2_RXFIE | SCIFCR2_RXFTOIE | SCIFCR2_RXORIE);
#endif

	LOG_INF("UART initialized at %p, baudrate %d", config->base, data->uart_cfg.baudrate);
	return 0;
}

/**
 * @brief Read a character from UART (polling mode)
 */
static int uart_ft9001_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	if (config->base->SCIFSR & SCIFSR_REMPTY_MASK) {
		return -1; /* No data available */
	}

	*c = config->base->SCIDRL;
	return 0;
}

/**
 * @brief Send a character via UART (polling mode)
 */
static void uart_ft9001_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	/* Wait for TX FIFO not full */
	while (config->base->SCIFSR & SCIFSR_TFULL_MASK) {
		/* Wait */
	}

	/* Send character */
	config->base->SCIDRL = c & 0xff;

	/* Wait for transmission complete */
	while (1) {
		if ((config->base->SCIFSR & SCIFSR_TEMPTY_MASK) &&
		    (config->base->SCIFSR & SCIFSR_FTC_MASK)) {
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
	int z_err = 0;

	/* Check error flags in SCIFSR2 */
	z_err = ((config->base->SCIFSR2 & SCISR2_FOR_MASK) ? (1 << 0) : 0) | /* Overrun */
		((config->base->SCIFSR2 & SCISR2_FPF_MASK) ? (1 << 1) : 0) | /* Parity */
		((config->base->SCIFSR2 & SCISR2_FNF_MASK) ? (1 << 5) : 0) | /* Noise */
		((config->base->SCIFSR2 & SCISR2_FFE_MASK) ? (1 << 2) : 0);  /* Frame */

	/* Clear error flags by writing back */
	config->base->SCIFSR2 = config->base->SCIFSR2;

	return z_err;
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
	int num_tx = 0;

	for (num_tx = 0; num_tx < size; num_tx++) {
		if (config->base->SCIFSR & SCIFSR_TFULL_MASK) {
			break; /* TX FIFO full */
		}
		config->base->SCIDRL = tx_data[num_tx];
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

	while (!(config->base->SCIFSR & SCIFSR_REMPTY_MASK) && (i < size)) {
		rx_data[i] = config->base->SCIDRL;
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

	config->base->SCIFCR2 |= SCIFCR2_TXFIE;
}

/**
 * @brief Disable TX interrupt
 */
static void uart_ft9001_irq_tx_disable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	config->base->SCIFCR2 &= ~SCIFCR2_TXFIE;
}

/**
 * @brief Check if TX is ready
 */
static int uart_ft9001_irq_tx_ready(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	return !(config->base->SCIFSR & SCIFSR_TFULL_MASK);
}

/**
 * @brief Enable RX interrupt
 */
static void uart_ft9001_irq_rx_enable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	config->base->SCIFCR2 |= (SCIFCR2_RXFIE | SCIFCR2_RXFTOIE);
}

/**
 * @brief Disable RX interrupt
 */
static void uart_ft9001_irq_rx_disable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	config->base->SCIFCR2 &= ~(SCIFCR2_RXFIE | SCIFCR2_RXFTOIE);
}

/**
 * @brief Check if TX is complete
 */
static int uart_ft9001_irq_tx_complete(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	return ((config->base->SCIFSR & SCIFSR_TEMPTY_MASK) &&
		(config->base->SCIFSR & SCIFSR_FTC_MASK));
}

/**
 * @brief Check if RX data is ready
 */
static int uart_ft9001_irq_rx_ready(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	return ((config->base->SCIFSR & SCIFSR_RFTS_MASK) ||
		!(config->base->SCIFSR & SCIFSR_REMPTY_MASK));
}

/**
 * @brief Enable error interrupt
 */
static void uart_ft9001_irq_err_enable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	config->base->SCIFCR2 |= (SCIFCR2_RXORIE | SCIFCR2_RXFTOIE);
}

/**
 * @brief Disable error interrupt
 */
static void uart_ft9001_irq_err_disable(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	config->base->SCIFCR2 &= ~(SCIFCR2_RXORIE | SCIFCR2_RXFTOIE);
}

/**
 * @brief Check if interrupt is pending
 */
static int uart_ft9001_irq_is_pending(const struct device *dev)
{
	const struct uart_ft9001_config *config = DEV_CFG(dev);

	/* Check if TX or RX interrupts are pending */
	return ((!(config->base->SCIFSR & SCIFSR_TFULL_MASK) &&
		 (config->base->SCIFCR2 & SCIFCR2_TXFIE)) ||
		(((config->base->SCIFSR & SCIFSR_RFTS_MASK) ||
		  !(config->base->SCIFSR & SCIFSR_REMPTY_MASK)) &&
		 (config->base->SCIFCR2 & SCIFCR2_RXFIE)));
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
static const struct uart_driver_api uart_ft9001_driver_api = {
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
		.base = (struct ft9001_uart_regs *)DT_INST_REG_ADDR(idx),                          \
		.clkid = DT_INST_CLOCKS_CELL(idx, id),                                             \
		.reset = RESET_DT_SPEC_INST_GET(idx),                                              \
		FOCALTECH_UART_IRQ_HANDLER_FUNC_INIT(idx)};                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_ft9001_init, NULL, &uart_ft9001_data_##idx,                \
			      &uart_ft9001_cfg_##idx, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,   \
			      (void *)&uart_ft9001_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_FOCALTECH_FT9001_DEVICE);
