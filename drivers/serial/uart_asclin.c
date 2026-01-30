/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include "soc.h"
#include "IfxAsclin_regdef.h"
#include "uart_asclin_regs.h"

#define DT_DRV_COMPAT infineon_asclin_uart

#define UART_ASCLIN_FIFO_SIZE 16

enum uart_asclin_clock_type {
	UART_ASCLIN_CLOCK_OFF,
	UART_ASCLIN_CLOCK_FASCLINS,
	UART_ASCLIN_CLOCK_FASCLINF,
};

struct uart_asclin_data {
	struct uart_config *uart_cfg;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	Ifx_ASCLIN_FLAGS flags;
	Ifx_ASCLIN_FLAGSENABLE flags_enable;
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
};

struct uart_asclin_config {
	Ifx_ASCLIN *base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clkctrl;
	uint32_t clk;
	enum uart_asclin_clock_type clk_src;
	uint16_t prescaler;
	uint8_t oversampling;
	uint8_t samplepoint;
	bool median_filter;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

static inline uint32_t uart_asclin_rx_fifo_fill_level(Ifx_ASCLIN *base)
{
	return base->RXFIFOCON.B.FILL;
}

static inline uint32_t uart_asclin_tx_fifo_fill_level(Ifx_ASCLIN *base)
{
	return base->TXFIFOCON.B.FILL;
}

static inline uint8_t uart_asclin_rx_fifo_read(Ifx_ASCLIN *base)
{
	return ASCLIN_RXDATA_READ(base);
}

static inline void uart_asclin_tx_fifo_write(Ifx_ASCLIN *base, uint8_t c)
{
	ASCLIN_TXDATA_WRITE(base, c);
}

static inline void uart_asclin_set_bittime(Ifx_ASCLIN *base, uint8_t oversampling,
					   uint16_t prescaler, uint8_t samplepoint,
					   bool median_filter)
{
	base->BITCON.B = (Ifx_ASCLIN_BITCON_Bits) {.OVERSAMPLING = oversampling - 1,
						  .PRESCALER = prescaler - 1,
						  .SAMPLEPOINT = samplepoint,
						  .SM = median_filter};
}

static int uart_asclin_err_check(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	Ifx_ASCLIN_FLAGS flags = {.U = config->base->FLAGS.U};

	if (flags.B.PE) {
		return UART_ERROR_PARITY;
	}
	if (flags.B.FE) {
		return UART_ERROR_FRAMING;
	}
	if (flags.B.BD) {
		return UART_BREAK;
	}
	if (flags.B.RFO) {
		return UART_ERROR_OVERRUN;
	}
	if (flags.B.CE) {
		return UART_ERROR_COLLISION;
	}

	Ifx_ASCLIN_FLAGSCLEAR clr = {
		.B.PEC = 1, .B.FEC = 1, .B.BDC = 1,
		.B.RFOC = 1, .B.CEC = 1
	};

	config->base->FLAGSCLEAR.U = clr.U;

	return 0;
}

static inline int uart_asclin_set_clk(const struct uart_asclin_config *config, uint8_t clk)
{
	config->base->CSR.U = clk;
	if (clk) {
		return !WAIT_FOR(config->base->CSR.B.CON != 0, 1000, k_busy_wait(1));
	} else {
		return !WAIT_FOR(config->base->CSR.B.CON == 0, 1000, k_busy_wait(1));
	}
}

static inline int uart_asclin_set_mode_init(const struct uart_asclin_config *config)
{
	int ret = uart_asclin_set_clk(config, UART_ASCLIN_CLOCK_OFF);

	if (ret) {
		return ret;
	}
	config->base->FRAMECON.U = 0;

	return 0;
}

static bool uart_asclin_set_baudrate(Ifx_ASCLIN *base, uint32_t baudrate, uint32_t fpd,
				     uint8_t oversampling)
{
	uint32_t fovs = baudrate * oversampling;
	int32_t m[2][2];
	int32_t ai;
	float divider = ((float)fovs / (float)fpd);

	/* initialize matrix */
	m[0][0] = m[1][1] = 1;
	m[0][1] = m[1][0] = 0;

	/* loop finding terms until denom gets too big */
	while (m[1][0] * (ai = (uint32_t)divider) + m[1][1] <= 4095) {
		int32_t t;

		t = m[0][0] * ai + m[0][1];
		m[0][1] = m[0][0];
		m[0][0] = t;
		t = m[1][0] * ai + m[1][1];
		m[1][1] = m[1][0];
		m[1][0] = t;
		divider = 1 / (divider - (float)ai);
	}

	base->BRG.B = (Ifx_ASCLIN_BRG_Bits) {.NUMERATOR = m[0][0], .DENOMINATOR = m[1][0]};

	return true;
}

static inline void uart_asclin_set_framecfg(Ifx_ASCLIN *base, uint8_t data_bits, uint8_t stop_bits,
					    bool parity_enable, bool parity_odd)
{
	/* Frame Configuration*/
	base->FRAMECON.B = (Ifx_ASCLIN_FRAMECON_Bits) {
		.STOP = stop_bits, .MODE = 1, .PEN = parity_enable, .ODD = parity_odd};
	/* Data configuration */
	base->DATCON.B = (Ifx_ASCLIN_DATCON_Bits) {.DATLEN = data_bits};
	/* Fifo configuration */
	base->TXFIFOCON.B = (Ifx_ASCLIN_TXFIFOCON_Bits) {
		.FLUSH = 1, .ENO = 1, .FM = 0, .INW = (data_bits > 8 ? 2 : 1)};
	base->RXFIFOCON.B = (Ifx_ASCLIN_RXFIFOCON_Bits) {
		.FLUSH = 1, .ENI = 1, .FM = 0, .OUTW = (data_bits > 8 ? 2 : 1)};
}

static int uart_asclin_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;
	int ret_val = -1;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(p_char != NULL, "p_char is null pointer!");

	/* Stop, if p_char is null pointer */
	if (p_char == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* check if received character is ready.*/
	if (uart_asclin_rx_fifo_fill_level(config->base)) {
		/* got a character */
		*p_char = uart_asclin_rx_fifo_read(config->base);
		ret_val = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

static void uart_asclin_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;

	k_spinlock_key_t key;

	/* wait until uart is free to transmit.*/
	while (true) {
		key = k_spin_lock(&data->lock);
		if (uart_asclin_tx_fifo_fill_level(config->base) < UART_ASCLIN_FIFO_SIZE) {
			break;
		}
		k_spin_unlock(&data->lock, key);
	}

	uart_asclin_tx_fifo_write(config->base, c);

	k_spin_unlock(&data->lock, key);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_asclin_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *const data = dev->data;
	uint32_t fpd;

	if (!cfg) {
		return -EINVAL;
	}

	if (cfg->stop_bits == UART_CFG_STOP_BITS_0_5 || cfg->parity == UART_CFG_PARITY_MARK ||
	    cfg->parity == UART_CFG_PARITY_SPACE) {
		return -ENOTSUP;
	}

	clock_control_get_rate(config->clkctrl, (clock_control_subsys_t)&config->clk, &fpd);

	uart_asclin_set_mode_init(config);

	uart_asclin_set_baudrate(config->base, cfg->baudrate, fpd, config->oversampling);
	uart_asclin_set_framecfg(config->base, cfg->data_bits + 4, cfg->stop_bits,
				 cfg->parity != UART_CFG_PARITY_NONE,
				 cfg->parity == UART_CFG_PARITY_ODD);

	if (uart_asclin_set_clk(config, config->clk_src)) {
		return -ETIMEDOUT;
	}

	*data->uart_cfg = *cfg;

	return 0;
}

static int uart_asclin_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct uart_asclin_data *data = dev->data;

	__ASSERT(cfg != NULL, "cfg_out is null pointer!");

	if (cfg == NULL) {
		return -EINVAL;
	}

	*cfg = *data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_asclin_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_asclin_config *config = dev->config;
	int count = 0;

	while (count < size &&
	       uart_asclin_tx_fifo_fill_level(config->base) < UART_ASCLIN_FIFO_SIZE) {
		uart_asclin_tx_fifo_write(config->base, tx_data[count++]);
	}

	return count;
}

static int uart_asclin_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_asclin_config *config = dev->config;
	int count = 0;

	while (count < size && uart_asclin_rx_fifo_fill_level(config->base) > 0) {
		rx_data[count++] = uart_asclin_rx_fifo_read(config->base);
	}

	return count;
}

static void uart_asclin_irq_tx_enable(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	uint32_t key = irq_lock();

	config->base->FLAGSENABLE.U |= BIT(31);
	config->base->FLAGSCLEAR.U = BIT(31);
	config->base->FLAGSSET.U = BIT(31);

	irq_unlock(key);
}

static void uart_asclin_irq_tx_disable(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	uint32_t key = irq_lock();
	Ifx_ASCLIN_FLAGSENABLE flags_enable = {.U = config->base->FLAGSENABLE.U};

	flags_enable.B.TFLE = 0;
	config->base->FLAGSENABLE.U = flags_enable.U;

	irq_unlock(key);
}

static int uart_asclin_irq_tx_ready(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;

	return data->flags.B.TFL && (config->base->FLAGSENABLE.U & BIT(31));
}

static int uart_asclin_irq_tx_complete(const struct device *dev)
{
	struct uart_asclin_data *data = dev->data;

	return data->flags.B.TC;
}

static void uart_asclin_irq_rx_enable(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	uint32_t key = irq_lock();
	Ifx_ASCLIN_FLAGSENABLE flags_enable = {.U = config->base->FLAGSENABLE.U};

	flags_enable.B.RFLE = 1;
	config->base->FLAGSENABLE.U = flags_enable.U;

	irq_unlock(key);
}

static void uart_asclin_irq_rx_disable(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	uint32_t key = irq_lock();
	Ifx_ASCLIN_FLAGSENABLE flags_enable = {.U = config->base->FLAGSENABLE.U};

	flags_enable.B.RFLE = 0;
	config->base->FLAGSENABLE.U = flags_enable.U;

	irq_unlock(key);
}

static int uart_asclin_irq_rx_ready(const struct device *dev)
{
	struct uart_asclin_data *data = dev->data;

	return data->flags.B.RFL;
}

static void uart_asclin_irq_err_enable(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	uint32_t key = irq_lock();
	Ifx_ASCLIN_FLAGSENABLE flags_enable = {.U = config->base->FLAGSENABLE.U};

	flags_enable.B.PEE = 1;
	flags_enable.B.FEE = 1;
	flags_enable.B.CEE = 1;
	flags_enable.B.RFOE = 1;
	flags_enable.B.BDE = 1;
	config->base->FLAGSCLEAR.U = BIT(16) | BIT(18) | BIT(21) | BIT(25) | BIT(26);
	config->base->FLAGSENABLE.U = flags_enable.U;

	irq_unlock(key);
}

static void uart_asclin_irq_err_disable(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	uint32_t key = irq_lock();
	Ifx_ASCLIN_FLAGSENABLE flags_enable = {.U = config->base->FLAGSENABLE.U};

	flags_enable.B.PEE = 0;
	flags_enable.B.FEE = 0;
	flags_enable.B.CEE = 0;
	flags_enable.B.RFOE = 0;
	flags_enable.B.BDE = 0;
	config->base->FLAGSENABLE.U = flags_enable.U;

	irq_unlock(key);
}

static int uart_asclin_irq_is_pending(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;
	uint32_t en = config->base->FLAGSENABLE.U;

	return (data->flags.B.RFL && (en & BIT(28))) ||
		(data->flags.B.TFL && (en & BIT(31)));
}

static int uart_asclin_irq_update(const struct device *dev)
{
	const struct uart_asclin_config *config = dev->config;
	struct uart_asclin_data *data = dev->data;

	data->flags.U = config->base->FLAGS.U;
	config->base->FLAGSCLEAR.U = BIT(17);

	return 1;
}

static void uart_asclin_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct uart_asclin_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = user_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_asclin_init(const struct device *dev)
{
	const struct uart_asclin_config *cfg = dev->config;
	const struct uart_asclin_data *data = dev->data;
	uint32_t fpd;
	int ret;

	if (!device_is_ready(cfg->clkctrl)) {
		return -EIO;
	}

	ret = clock_control_on(cfg->clkctrl, (clock_control_subsys_t)&cfg->clk);
	if (ret != 0) {
		return ret;
	}
	ret = clock_control_get_rate(cfg->clkctrl, (clock_control_subsys_t)&cfg->clk, &fpd);
	if (ret != 0) {
		return ret;
	}

	if (!aurix_enable_clock((uintptr_t)&cfg->base->CLC, 1000)) {
		return -EIO;
	}

	/* Pin config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	if (uart_asclin_set_mode_init(cfg)) {
		return -ETIMEDOUT;
	}

	uart_asclin_set_bittime(cfg->base, cfg->oversampling, cfg->prescaler, cfg->samplepoint,
				cfg->median_filter);
	uart_asclin_set_baudrate(cfg->base, data->uart_cfg->baudrate, fpd, cfg->oversampling);
	uart_asclin_set_framecfg(cfg->base, data->uart_cfg->data_bits + 4,
				 data->uart_cfg->stop_bits,
				 data->uart_cfg->parity != UART_CFG_PARITY_NONE,
				 data->uart_cfg->parity == UART_CFG_PARITY_ODD);

	cfg->base->FLAGSENABLE.U = 0;
	cfg->base->FLAGSCLEAR.U = 0xFFFFFFFFu;

	if (uart_asclin_set_clk(cfg, cfg->clk_src)) {
		return -ETIMEDOUT;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif

	return 0;
}

static DEVICE_API(uart, uart_asclin_driver_api) = {
	.poll_in = uart_asclin_poll_in,
	.poll_out = uart_asclin_poll_out,
	.err_check = uart_asclin_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_asclin_configure,
	.config_get = uart_asclin_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_asclin_fifo_fill,
	.fifo_read = uart_asclin_fifo_read,
	.irq_tx_enable = uart_asclin_irq_tx_enable,
	.irq_tx_disable = uart_asclin_irq_tx_disable,
	.irq_tx_ready = uart_asclin_irq_tx_ready,
	.irq_tx_complete = uart_asclin_irq_tx_complete,
	.irq_rx_enable = uart_asclin_irq_rx_enable,
	.irq_rx_disable = uart_asclin_irq_rx_disable,
	.irq_rx_ready = uart_asclin_irq_rx_ready,
	.irq_err_enable = uart_asclin_irq_err_enable,
	.irq_err_disable = uart_asclin_irq_err_disable,
	.irq_is_pending = uart_asclin_irq_is_pending,
	.irq_update = uart_asclin_irq_update,
	.irq_callback_set = uart_asclin_irq_callback_set,
#endif

#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = uart_asclin_drv_cmd,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static void uart_asclin_isr(const struct device *dev)
{
	struct uart_asclin_data *data = dev->data;

	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#define UART_ASCLIN_IRQ_CONFIG_FUNC(n)                                                             \
	static void uart_asclin_irq_config_func_##n(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, tx, irq), DT_INST_IRQ_BY_NAME(n, tx, priority), \
			    uart_asclin_isr, DEVICE_DT_INST_GET(n), 0);                            \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, rx, irq), DT_INST_IRQ_BY_NAME(n, rx, priority), \
			    uart_asclin_isr, DEVICE_DT_INST_GET(n), 0);                            \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, err, irq),                                      \
			    DT_INST_IRQ_BY_NAME(n, err, priority), uart_asclin_isr,                \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQ_BY_NAME(n, tx, irq));                                       \
		irq_enable(DT_INST_IRQ_BY_NAME(n, rx, irq));                                       \
		irq_enable(DT_INST_IRQ_BY_NAME(n, err, irq));                                      \
	}

#define UART_ASCLIN_IRQ_CONFIG_INIT(n) .irq_config_func = uart_asclin_irq_config_func_##n,

#else

#define UART_ASCLIN_IRQ_CONFIG_FUNC(n)
#define UART_ASCLIN_IRQ_CONFIG_INIT(n)

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_ASCLIN_CLOCK_ID(n)                                                                    \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, fasclinf), \
		(DT_INST_CLOCKS_CELL_BY_NAME(n, fasclinf, id)), \
		(DT_INST_CLOCKS_CELL_BY_NAME(n, fasclins, id)))
#define UART_ASCLIN_CLOCK_SRC(n)                                                                   \
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, fasclinf), \
		(UART_ASCLIN_CLOCK_FASCLINF), \
		(UART_ASCLIN_CLOCK_FASCLINS))

#define UART_ASCLIN_DEVICE_INIT(n)                                                                 \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	UART_ASCLIN_IRQ_CONFIG_FUNC(n)                                                             \
	static struct uart_config uart_cfg_##n = {                                                 \
		.baudrate = DT_INST_PROP_OR(n, current_speed, 115200),                             \
		.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),                    \
		.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),              \
		.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),              \
		.flow_ctrl = DT_INST_PROP(n, hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS         \
							      : UART_CFG_FLOW_CTRL_NONE,           \
	};                                                                                         \
	static struct uart_asclin_data uart_asclin_dev_data_##n = {.uart_cfg = &uart_cfg_##n};     \
	static const struct uart_asclin_config uart_asclin_dev_cfg_##n = {                         \
		.base = (Ifx_ASCLIN *)DT_INST_REG_ADDR(n),                                         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clkctrl = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(n)),                          \
		.clk = UART_ASCLIN_CLOCK_ID(n),                                                    \
		.clk_src = UART_ASCLIN_CLOCK_SRC(n),                                               \
		.oversampling = DT_INST_PROP_OR(n, oversampling, 16),                              \
		.samplepoint = DT_INST_PROP_OR(n, samplepoint, 8),                                 \
		.median_filter = DT_INST_PROP(n, median_filter),                                   \
		.prescaler = DT_INST_PROP_OR(n, prescaler, 1),                                     \
		UART_ASCLIN_IRQ_CONFIG_INIT(n)};                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_asclin_init, NULL, &uart_asclin_dev_data_##n,                \
			      &uart_asclin_dev_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_asclin_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_ASCLIN_DEVICE_INIT)
