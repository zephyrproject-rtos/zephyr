/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_uart_sau

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <r_sau_uart.h>

LOG_MODULE_REGISTER(renesas_ra_uart_sau, CONFIG_UART_LOG_LEVEL);

#define SAU_UART_STCLK_MAX (127)
#define SAU_UART_STCLK_MIN (2)

typedef R_SAU0_Type uart_renesas_ra_sau_regs_t;

struct uart_renesas_ra_sau_config {
	const struct device *clock_dev;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	uart_renesas_ra_sau_regs_t *const regs;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	uint8_t tx_channel;
	uint8_t rx_channel;
};

struct uart_renesas_ra_sau_data {
	struct uart_config uart_cfg;
	sau_uart_instance_ctrl_t fsp_ctrl;
	uart_cfg_t fsp_cfg;
	sau_uart_extended_cfg_t fsp_extend_cfg;
	sau_uart_baudrate_setting_t fsp_baud_setting;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_callback_user_data_t user_cb;
	void *user_cb_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_renesas_ra_sau_baudrate_validate(const struct device *dev, uint32_t baudrate,
						 sau_uart_baudrate_setting_t *const p_baud_setting)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	uint32_t peripheral_clock;
	uint8_t stclk;
	int ret;

	ret = clock_control_get_rate(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys,
				     &peripheral_clock);
	if (ret < 0) {
		LOG_ERR("Failed to get peripheral clock rate: %d", ret);
		return ret;
	}

	stclk = DIV_ROUND_CLOSEST(peripheral_clock, (2 * baudrate)) - 1;

	if (stclk < SAU_UART_STCLK_MIN || stclk > SAU_UART_STCLK_MAX) {
		LOG_ERR("SAU UART baudrate of %u is not achievable with selected operation clock "
			"settings. Suggest stclk >= %hu and stclk <= %hu. Calculated stclk: %u",
			baudrate, SAU_UART_STCLK_MIN, SAU_UART_STCLK_MAX, stclk);
		return -EINVAL;
	}

	p_baud_setting->stclk = stclk;

	return 0;
}

static int uart_renesas_ra_sau_apply_config(const struct device *dev,
					    const struct uart_config *uart_cfg)
{
	struct uart_renesas_ra_sau_data *data = dev->data;
	uart_data_bits_t data_bits;
	uart_parity_t parity;
	fsp_err_t fsp_err;
	int ret;

	if (uart_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOSYS;
	}

	if (uart_cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOSYS;
	}

	/* If already open, check if the configuration is the same to prevent reconfiguration */
	if (data->fsp_ctrl.open != 0) {
		if (memcmp(&data->uart_cfg, uart_cfg, sizeof(struct uart_config)) == 0) {
			return 0;
		}
	}

	switch (uart_cfg->data_bits) {
	case UART_CFG_DATA_BITS_7:
		data_bits = UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		data_bits = UART_DATA_BITS_8;
		break;
	default:
		LOG_ERR("Unsupported data bits setting");
		return -ENOSYS;
	}

	switch (uart_cfg->parity) {
	case UART_CFG_PARITY_NONE:
		parity = UART_PARITY_OFF;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		parity = UART_PARITY_ODD;
		break;
	default:
		LOG_ERR("Unsupported parity setting");
		return -ENOSYS;
	}

	ret = uart_renesas_ra_sau_baudrate_validate(dev, uart_cfg->baudrate,
						    &data->fsp_baud_setting);
	if (ret < 0) {
		LOG_ERR("Failed to calculate baudrate settings");
		return ret;
	}

	/* If the UART is already open, close it before applying new configuration */
	if (data->fsp_ctrl.open != 0) {
		fsp_err = R_SAU_UART_Close(&data->fsp_ctrl);
		if (fsp_err != FSP_SUCCESS) {
			return -EIO;
		}
	}

	/* Update the new configuration to the data structure */
	data->fsp_cfg.data_bits = data_bits;
	data->fsp_cfg.parity = parity;

	/* Open the UART with the new configuration */
	fsp_err = R_SAU_UART_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	memcpy(&data->uart_cfg, uart_cfg, sizeof(struct uart_config));
	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
void uart_renesas_ra_sau_isr(struct device *dev)
{
	struct uart_renesas_ra_sau_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_renesas_ra_sau_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	int ret = -1;

	/* Check if the receive data register is full */
	if (cfg->regs->SSR_b[cfg->rx_channel].BFF == 1) {
		*c = (unsigned char)cfg->regs->SDR_b[cfg->rx_channel].DAT;
		ret = 0;
	}

	return ret;
}

static void uart_renesas_ra_sau_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	uart_renesas_ra_sau_regs_t *const regs = cfg->regs;

	/* Wait until the transmit buffer is empty */
	while (regs->SSR_b[cfg->tx_channel].BFF != 0) {
	}
	regs->SDR_b[cfg->tx_channel].DAT = c;
}

static int uart_renesas_ra_sau_err_check(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	const uint16_t ssr_reg = cfg->regs->SSR[cfg->rx_channel];
	int err = 0;
	uint8_t flag_clear = 0U;

	if (ssr_reg == 0) {
		return 0;
	}

	if (R_SAU0_SSR_OVF_Msk & ssr_reg) {
		flag_clear |= R_SAU0_SSR_OVF_Msk;
		err |= UART_ERROR_OVERRUN;
	}

	if (R_SAU0_SSR_PEF_Msk & ssr_reg) {
		flag_clear |= R_SAU0_SSR_PEF_Msk;
		err |= UART_ERROR_PARITY;
	}

	if (R_SAU0_SSR_FEF_Msk & ssr_reg) {
		flag_clear |= R_SAU0_SSR_FEF_Msk;
		err |= UART_ERROR_FRAMING;
	}

	/* The data buffer must be read as part of clearing
	 * the error to avoid an overrun error after recovery.
	 */
	cfg->regs->SDR_b[cfg->rx_channel].DAT;

	/* Clear the error flags */
	cfg->regs->SIR[cfg->rx_channel] = flag_clear;

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_renesas_ra_sau_configure(const struct device *dev, const struct uart_config *cfg)
{
	return uart_renesas_ra_sau_apply_config(dev, cfg);
}

static int uart_renesas_ra_sau_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_renesas_ra_sau_data *data = dev->data;

	*cfg = data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_renesas_ra_sau_irq_callback_set(const struct device *dev,
						 uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_renesas_ra_sau_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->user_cb = cb;
	data->user_cb_data = cb_data;
	irq_unlock(key);
}

static void uart_renesas_ra_sau_irq_tx_enable(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	struct uart_renesas_ra_sau_data *data = dev->data;

	cfg->regs->SMR[cfg->tx_channel] |= R_SAU0_SMR_MD0_Msk;
	/* Enable TX interrupt */
	irq_enable((uint32_t)data->fsp_cfg.txi_irq);
}

static void uart_renesas_ra_sau_irq_tx_disable(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	struct uart_renesas_ra_sau_data *data = dev->data;

	cfg->regs->SMR[cfg->tx_channel] &= ~R_SAU0_SMR_MD0_Msk;
	/* Disable TX interrupt */
	irq_disable((uint32_t)data->fsp_cfg.txi_irq);
}

static void uart_renesas_ra_sau_irq_rx_enable(const struct device *dev)
{
	struct uart_renesas_ra_sau_data *data = dev->data;

	/* Enable RX interrupt */
	irq_enable((uint32_t)data->fsp_cfg.rxi_irq);
}

static void uart_renesas_ra_sau_irq_rx_disable(const struct device *dev)
{
	struct uart_renesas_ra_sau_data *data = dev->data;

	/* Disable RX interrupt */
	irq_disable((uint32_t)data->fsp_cfg.rxi_irq);
}

static void uart_renesas_ra_sau_irq_err_enable(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;

	/* Enable error interrupt */
	cfg->regs->SCR[cfg->rx_channel] |= R_SAU0_SCR_EOC_Msk;
}

static void uart_renesas_ra_sau_irq_err_disable(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;

	/* Disable error interrupt */
	cfg->regs->SCR[cfg->rx_channel] &= ~R_SAU0_SCR_EOC_Msk;
}

static int uart_renesas_ra_sau_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static int uart_renesas_ra_sau_irq_is_pending(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	struct uart_renesas_ra_sau_data *data = dev->data;
	const uint16_t tx_ssr = cfg->regs->SSR[cfg->tx_channel];
	const uint16_t rx_ssr = cfg->regs->SSR[cfg->rx_channel];
	const uint16_t rx_scr = cfg->regs->SCR[cfg->rx_channel];

	/* Check TX buffer full interrupt flag - pending when BFF = 0 (empty) */
	bool tx_pending =
		!(tx_ssr & R_SAU0_SSR_BFF_Msk) && irq_is_enabled((uint32_t)data->fsp_cfg.txi_irq);

	/* Check RX buffer full interrupt flag - pending when BFF = 1 (full) */
	bool rx_pending = !!(rx_ssr & (R_SAU0_SSR_BFF_Msk)) &&
			  irq_is_enabled((uint32_t)data->fsp_cfg.rxi_irq);

	/* Check error interrupt flags - pending when one or more error flags are set */
	bool err_pending =
		!!(rx_ssr & (R_SAU0_SSR_OVF_Msk | R_SAU0_SSR_PEF_Msk | R_SAU0_SSR_FEF_Msk)) &&
		!!(rx_scr & R_SAU0_SCR_EOC_Msk);

	return (tx_pending || rx_pending || err_pending);
}

static int uart_renesas_ra_sau_irq_tx_ready(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	uart_renesas_ra_sau_regs_t *const regs = cfg->regs;

	/* Check whether the transmit buffer is empty */
	if (regs->SSR_b[cfg->tx_channel].BFF == 0) {
		return 1;
	}

	return 0;
}

static int uart_renesas_ra_sau_irq_rx_ready(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;

	/* Check whether the receive buffer is full */
	return (cfg->regs->SSR_b[cfg->rx_channel].BFF == 1);
}

static int uart_renesas_ra_sau_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	int ret = 0;

	/* Check whether the transmit buffer is empty and fill it */
	if ((size > 0) && (cfg->regs->SSR_b[cfg->tx_channel].BFF == 0)) {
		cfg->regs->SDR_b[cfg->tx_channel].DAT = *tx_data;
		ret = 1;
	}

	return ret;
}

static int uart_renesas_ra_sau_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	int ret = 0;

	/* Check whether the receive buffer is full and read from it */
	if ((size > 0) && (cfg->regs->SSR_b[cfg->rx_channel].BFF == 1)) {
		*rx_data = cfg->regs->SDR_b[cfg->rx_channel].DAT;
		ret = 1;
	}

	return ret;
}

static int uart_renesas_ra_sau_irq_tx_complete(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;

	/* Check whether transmit is complete */
	return (cfg->regs->SSR_b[cfg->tx_channel].TSF == 0);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_renesas_ra_sau_driver_api) = {
	.poll_in = uart_renesas_ra_sau_poll_in,
	.poll_out = uart_renesas_ra_sau_poll_out,
	.err_check = uart_renesas_ra_sau_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_renesas_ra_sau_configure,
	.config_get = uart_renesas_ra_sau_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_renesas_ra_sau_fifo_fill,
	.fifo_read = uart_renesas_ra_sau_fifo_read,
	.irq_tx_enable = uart_renesas_ra_sau_irq_tx_enable,
	.irq_tx_disable = uart_renesas_ra_sau_irq_tx_disable,
	.irq_tx_ready = uart_renesas_ra_sau_irq_tx_ready,
	.irq_rx_enable = uart_renesas_ra_sau_irq_rx_enable,
	.irq_rx_disable = uart_renesas_ra_sau_irq_rx_disable,
	.irq_tx_complete = uart_renesas_ra_sau_irq_tx_complete,
	.irq_rx_ready = uart_renesas_ra_sau_irq_rx_ready,
	.irq_err_enable = uart_renesas_ra_sau_irq_err_enable,
	.irq_err_disable = uart_renesas_ra_sau_irq_err_disable,
	.irq_is_pending = uart_renesas_ra_sau_irq_is_pending,
	.irq_update = uart_renesas_ra_sau_irq_update,
	.irq_callback_set = uart_renesas_ra_sau_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_renesas_ra_sau_init(const struct device *dev)
{
	const struct uart_renesas_ra_sau_config *cfg = dev->config;
	struct uart_renesas_ra_sau_data *data = dev->data;
	int ret;

	ret = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (data->fsp_baud_setting.operation_clock != SAU_UART_OPERATION_CLOCK_CK0 &&
	    data->fsp_baud_setting.operation_clock != SAU_UART_OPERATION_CLOCK_CK1) {
		LOG_ERR("Invalid operation clock setting. Expected SAU_CKm0 or SAU_CKm1.");
		return -EINVAL;
	}

	ret = uart_renesas_ra_sau_apply_config(dev, &data->uart_cfg);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#define UART_RENESAS_RA_SAU_TX_IRQ_GET(idx, cell)                                                  \
	DT_IRQ(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), cell)

#define UART_RENESAS_RA_SAU_RX_IRQ_GET(idx, cell)                                                  \
	DT_IRQ(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 1), cell)

#define UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx)                                                     \
	DT_CLOCKS_CTLR(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0))

#define UART_RENESAS_RA_SAU_RX_CLOCK_CTRL(idx)                                                     \
	DT_CLOCKS_CTLR(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 1))

#define RENESAS_RA_SAU_UART_BIR_ORDER_GET(idx)                                                     \
	COND_CODE_1(DT_INST_PROP(idx, msb_first), (SAU_UART_DATA_SEQUENCE_MSB),			   \
		    (SAU_UART_DATA_SEQUENCE_LSB))

#define RENESAS_RA_SAU_UART_TX_SIGNAL_LEVEL_GET(idx)                                               \
	COND_CODE_1(DT_INST_PROP(idx, tx_signal_inversion), (SAU_UART_SIGNAL_LEVEL_INVERTED),	   \
		    (SAU_UART_SIGNAL_LEVEL_STANDARD))

#define RENESAS_RA_SAU_UART_CHECK_OPERATION_CLOCK(idx)                                             \
	BUILD_ASSERT(                                                                              \
		(IS_EQ(DT_PROP(DT_PARENT(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)), unit), 0)  \
			 ? (DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx),                   \
					 DT_NODELABEL(sau_ck00)) ||                                \
			    DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx),                   \
					 DT_NODELABEL(sau_ck01)))                                  \
			 : (IS_EQ(DT_PROP(DT_PARENT(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)), \
					  unit),                                                   \
				  1)                                                               \
				    ? (DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx),        \
						    DT_NODELABEL(sau_ck10)) ||                     \
				       DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx),        \
						    DT_NODELABEL(sau_ck11)))                       \
				    : 0)),                                                         \
		"operation_clock not supported")

#define RENESAS_RA_SAU_UART_OPERATION_CLOCK(idx)                                                   \
	(DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx), DT_NODELABEL(sau_ck00)) ||           \
	 DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx), DT_NODELABEL(sau_ck10)))             \
		? SAU_UART_OPERATION_CLOCK_CK0                                                     \
		: SAU_UART_OPERATION_CLOCK_CK1

#if !defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define UART_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_DEFINE(idx)
#define UART_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_GET(idx)
#else
#define UART_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_DEFINE(idx)                                            \
	static void uart_renesas_ra_sau_irq_configure_##idx(const struct device *dev)              \
	{                                                                                          \
		IRQ_CONNECT(UART_RENESAS_RA_SAU_RX_IRQ_GET(idx, irq),                              \
			    UART_RENESAS_RA_SAU_RX_IRQ_GET(idx, priority),                         \
			    uart_renesas_ra_sau_isr, DEVICE_DT_INST_GET(idx), 0);                  \
                                                                                                   \
		IRQ_CONNECT(UART_RENESAS_RA_SAU_TX_IRQ_GET(idx, irq),                              \
			    UART_RENESAS_RA_SAU_TX_IRQ_GET(idx, priority),                         \
			    uart_renesas_ra_sau_isr, DEVICE_DT_INST_GET(idx), 0);                  \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQ(idx, irq), DT_INST_IRQ(idx, priority),                     \
			    uart_renesas_ra_sau_isr, DEVICE_DT_INST_GET(idx), 0);                  \
		irq_enable(DT_INST_IRQ(idx, irq));                                                 \
	}

#define UART_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_GET(idx)                                               \
	.irq_config_func = uart_renesas_ra_sau_irq_configure_##idx

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_RENESAS_RA_SAU_INIT(idx)                                                              \
	BUILD_ASSERT(                                                                              \
		DT_SAME_NODE(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx),                               \
			     UART_RENESAS_RA_SAU_RX_CLOCK_CTRL(idx)),                              \
		"SAU UART TX and SAU UART RX must have the same clock sources on device tree");    \
                                                                                                   \
	RENESAS_RA_SAU_UART_CHECK_OPERATION_CLOCK(idx);                                            \
                                                                                                   \
	UART_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_DEFINE(idx)                                            \
	PINCTRL_DT_DEFINE(DT_DRV_INST(idx));                                                       \
	static const struct uart_renesas_ra_sau_config uart_renesas_ra_sau_config_##idx = {        \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(idx)),                             \
		.clock_dev = DEVICE_DT_GET(UART_RENESAS_RA_SAU_TX_CLOCK_CTRL(idx)),                \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = (uint32_t)DT_CLOCKS_CELL_BY_IDX(                           \
					DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), 0, mstp),    \
				.stop_bit = DT_CLOCKS_CELL_BY_IDX(                                 \
					DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0), 0,           \
					stop_bit),                                                 \
			},                                                                         \
		.regs = (uart_renesas_ra_sau_regs_t *)DT_REG_ADDR(                                 \
			DT_PARENT(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0))),                  \
		.tx_channel = DT_REG_ADDR(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 0)),           \
		.rx_channel = DT_REG_ADDR(DT_INST_PHANDLE_BY_IDX(idx, sau_channels, 1)),           \
		UART_RENESAS_RA_SAU_IRQ_CONFIG_FUNC_GET(idx)};                                     \
                                                                                                   \
	static struct uart_renesas_ra_sau_data uart_renesas_ra_sau_data_##idx = {                  \
		.uart_cfg =                                                                        \
			{                                                                          \
				.baudrate = DT_INST_PROP(idx, current_speed),                      \
				.parity = DT_INST_ENUM_IDX(idx, parity),                           \
				.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                     \
				.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                     \
				.flow_ctrl = DT_INST_PROP(idx, hw_flow_control)                    \
						     ? UART_CFG_FLOW_CTRL_RTS_CTS                  \
						     : UART_CFG_FLOW_CTRL_NONE,                    \
			},                                                                         \
		.fsp_baud_setting =                                                                \
			{                                                                          \
				.operation_clock = RENESAS_RA_SAU_UART_OPERATION_CLOCK(idx),       \
			},                                                                         \
		.fsp_extend_cfg =                                                                  \
			{                                                                          \
				.sequence = RENESAS_RA_SAU_UART_BIR_ORDER_GET(idx),                \
				.signal_level = RENESAS_RA_SAU_UART_TX_SIGNAL_LEVEL_GET(idx),      \
				.p_baudrate = &uart_renesas_ra_sau_data_##idx.fsp_baud_setting,    \
			},                                                                         \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.channel = DT_INST_REG_ADDR(idx),                                  \
				.rxi_ipl = UART_RENESAS_RA_SAU_RX_IRQ_GET(idx, priority),          \
				.rxi_irq = UART_RENESAS_RA_SAU_RX_IRQ_GET(idx, irq),               \
				.txi_ipl = UART_RENESAS_RA_SAU_TX_IRQ_GET(idx, priority),          \
				.txi_irq = UART_RENESAS_RA_SAU_TX_IRQ_GET(idx, irq),               \
				.eri_ipl = BSP_IRQ_DISABLED,                                       \
				.eri_irq = FSP_INVALID_VECTOR,                                     \
				.p_extend = &uart_renesas_ra_sau_data_##idx.fsp_extend_cfg,        \
				.p_context = NULL,                                                 \
				.p_transfer_tx = NULL,                                             \
				.p_transfer_rx = NULL,                                             \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_renesas_ra_sau_init, NULL,                                 \
			      &uart_renesas_ra_sau_data_##idx, &uart_renesas_ra_sau_config_##idx,  \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,                           \
			      &uart_renesas_ra_sau_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RENESAS_RA_SAU_INIT)
