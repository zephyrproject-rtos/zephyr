/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rh850_rlin3_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include "r_rlin3_uart.h"

LOG_MODULE_REGISTER(rh850_rlin3_uart, CONFIG_UART_LOG_LEVEL);

struct uart_rh850_rlin3_config {
	const struct pinctrl_dev_config *pin_config;
	const uart_api_t *fsp_api;
};

struct uart_rh850_rlin3_int {
	bool txi_flag;
	bool rxi_flag;
	bool eri_flag;
	bool irq_rx_enable;
	bool irq_tx_enable;
	uart_event_t event;
};

struct uart_rh850_rlin3_data {
	const struct device *dev;
	struct uart_config uart_config;
	struct uart_rh850_rlin3_int int_data;
	struct st_rlin3_uart_instance_ctrl *fsp_ctrl;
	struct st_uart_cfg *fsp_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *callback_data;

	volatile uint16_t *const txi_trigger_reg;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static void eri_status_clear(struct uart_rh850_rlin3_data *data);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void rlin3_uart_rxi_isr(void);
void rlin3_uart_txi_isr(void);
void rlin3_uart_eri_isr(void);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static void eri_status_clear(struct uart_rh850_rlin3_data *data)
{
	rlin3_uart_instance_ctrl_t *fsp_ctrl = data->fsp_ctrl;

	/* Clear status */
	fsp_ctrl->p_reg->LEST = 0U;

	/* Dummy read 1 byte to prevent overflow error occurs after the first reception */
	(void)fsp_ctrl->p_reg->LURDR;

	data->int_data.eri_flag = false;
}

static int uart_rh850_rlin3_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	R_RLN30_Type *reg = data->fsp_ctrl->p_reg;

	if (!data->int_data.rxi_flag) {
		/* No data to read */
		return -1;
	}

	*c = reg->LURDR;
	data->int_data.rxi_flag = false;
	return 0;
}

static void uart_rh850_rlin3_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	R_RLN30_Type *reg = data->fsp_ctrl->p_reg;

	while (reg->LST_b.UTS != 0U) {
		;
	}
	reg->LUTDR = c;
	while (reg->LST_b.UTS != 0U) {
		;
	}
}

static int uart_rh850_rlin3_err_check(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	uart_event_t event = data->fsp_ctrl->p_reg->LEST;
	int err = 0;

	if ((event & UART_EVENT_ERR_OVERRUN) != 0) {
		err |= UART_ERROR_OVERRUN;
	}
	if ((event & UART_EVENT_ERR_FRAMING) != 0) {
		err |= UART_ERROR_FRAMING;
	}
	if ((event & UART_EVENT_ERR_PARITY) != 0) {
		err |= UART_ERROR_PARITY;
	}

	/* Clear UART error status register */
	eri_status_clear(data);

	return err;
}

static int uart_rh850_rlin3_apply_config(const struct device *dev)

{
	struct uart_rh850_rlin3_data *data = dev->data;

	struct uart_config *uart_config = &data->uart_config;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;
	const rlin3_uart_extended_cfg_t *fsp_config_extend = fsp_cfg->p_extend;

	fsp_config_extend->p_baud_setting->baudrate = uart_config->baudrate;

	switch (uart_config->data_bits) {
	case UART_CFG_DATA_BITS_7:
		fsp_cfg->data_bits = UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		fsp_cfg->data_bits = UART_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	switch (uart_config->parity) {
	case UART_CFG_PARITY_NONE:
		fsp_cfg->parity = UART_PARITY_OFF;
		break;
	case UART_CFG_PARITY_ODD:
		fsp_cfg->parity = UART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		fsp_cfg->parity = UART_PARITY_EVEN;
		break;
	default:
		return -ENOTSUP;
	}

	switch (uart_config->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		fsp_cfg->stop_bits = UART_STOP_BITS_1;
		break;
	case UART_CFG_STOP_BITS_2:
		fsp_cfg->stop_bits = UART_STOP_BITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_rh850_rlin3_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_rh850_rlin3_config *config = dev->config;
	struct uart_rh850_rlin3_data *data = dev->data;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;
	const rlin3_uart_extended_cfg_t *fsp_config_extend = fsp_cfg->p_extend;
	int err = 0;

	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	err = uart_rh850_rlin3_apply_config(dev);

	if (err != 0) {
		return err;
	}

	if (FSP_SUCCESS != config->fsp_api->close(data->fsp_ctrl)) {
		return -EIO;
	}

	if (FSP_SUCCESS != config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg)) {
		return -EIO;
	}

	if (FSP_SUCCESS != config->fsp_api->baudSet(data->fsp_ctrl,
				fsp_config_extend->p_baud_setting)) {
		return -EIO;
	}

	/* Clear UART error status register */
	eri_status_clear(data);

	data->int_data.rxi_flag = false;
	data->int_data.txi_flag = false;
	data->int_data.eri_flag = false;
	data->int_data.irq_rx_enable = false;
	data->int_data.irq_tx_enable = false;

	return err;
}

static int uart_rh850_rlin3_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(struct uart_config));
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_rh850_rlin3_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	rlin3_uart_instance_ctrl_t *fsp_ctrl = data->fsp_ctrl;

	fsp_ctrl->tx_src_bytes = size;
	fsp_ctrl->p_tx_src = tx_data;

	rlin3_uart_txi_isr();
	data->int_data.txi_flag = false;

	return (size - fsp_ctrl->tx_src_bytes);
}

static int uart_rh850_rlin3_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	rlin3_uart_instance_ctrl_t *fsp_ctrl = data->fsp_ctrl;

	if (data->int_data.rxi_flag) {
		fsp_ctrl->rx_dest_bytes = size;
		fsp_ctrl->p_rx_dest = rx_data;

		rlin3_uart_rxi_isr();
		data->int_data.rxi_flag = false;

		return (size - fsp_ctrl->rx_dest_bytes);
	}

	return 0;
}

static void uart_rh850_rlin3_irq_rx_enable(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;

	data->int_data.irq_rx_enable = true;
	irq_enable(fsp_cfg->rxi_irq);
}

static void uart_rh850_rlin3_irq_rx_disable(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	data->int_data.irq_rx_enable = false;
	data->int_data.rxi_flag = false;
}

static void uart_rh850_rlin3_irq_tx_enable(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;

	data->int_data.irq_tx_enable = true;
	irq_enable(fsp_cfg->txi_irq);

	/* Trigger transfer complete interrupt */
	WRITE_BIT(*(data->txi_trigger_reg), 12, 1);
}

static void uart_rh850_rlin3_irq_tx_disable(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;

	data->int_data.irq_tx_enable = false;
	data->int_data.txi_flag = false;
	irq_disable(fsp_cfg->txi_irq);
}

static int uart_rh850_rlin3_irq_tx_ready(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	return data->int_data.irq_tx_enable && data->int_data.txi_flag;
}

static int uart_rh850_rlin3_irq_rx_ready(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	return data->int_data.rxi_flag && data->int_data.irq_rx_enable;
}

static int uart_rh850_rlin3_irq_err_ready(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	return data->int_data.eri_flag;
}

static int uart_rh850_rlin3_irq_is_pending(const struct device *dev)
{
	return (uart_rh850_rlin3_irq_tx_ready(dev)) || (uart_rh850_rlin3_irq_rx_ready(dev)) ||
	       (uart_rh850_rlin3_irq_err_ready(dev));
}

static void uart_rh850_rlin3_irq_callback_set(const struct device *dev,
					      uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	data->callback = cb;
	data->callback_data = cb_data;
}

static int uart_rh850_rlin3_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 1;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_rh850_rlin3_driver_api) = {
	.poll_in = uart_rh850_rlin3_poll_in,
	.poll_out = uart_rh850_rlin3_poll_out,
	.err_check = uart_rh850_rlin3_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rh850_rlin3_configure,
	.config_get = uart_rh850_rlin3_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rh850_rlin3_fifo_fill,
	.fifo_read = uart_rh850_rlin3_fifo_read,
	.irq_rx_enable = uart_rh850_rlin3_irq_rx_enable,
	.irq_rx_disable = uart_rh850_rlin3_irq_rx_disable,
	.irq_tx_enable = uart_rh850_rlin3_irq_tx_enable,
	.irq_tx_disable = uart_rh850_rlin3_irq_tx_disable,
	.irq_tx_ready = uart_rh850_rlin3_irq_tx_ready,
	.irq_rx_ready = uart_rh850_rlin3_irq_rx_ready,
	.irq_is_pending = uart_rh850_rlin3_irq_is_pending,
	.irq_callback_set = uart_rh850_rlin3_irq_callback_set,
	.irq_update = uart_rh850_rlin3_irq_update,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_rh850_rlin3_init(const struct device *dev)
{
	const struct uart_rh850_rlin3_config *config = dev->config;
	struct uart_rh850_rlin3_data *data = dev->data;
	uart_cfg_t *fsp_cfg = data->fsp_cfg;
	const rlin3_uart_extended_cfg_t *fsp_config_extend = fsp_cfg->p_extend;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pin_config, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* uart_rh850_rlin3_apply_config must be called first before open api */
	ret = uart_rh850_rlin3_apply_config(dev);
	if (ret < 0) {
		return ret;
	}

	if (FSP_SUCCESS != config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg)) {
		return -EIO;
	}

	if (FSP_SUCCESS != config->fsp_api->baudSet(data->fsp_ctrl,
				fsp_config_extend->p_baud_setting)) {
		return -EIO;
	}

	/* Clear UART error status register */
	eri_status_clear(data);

	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static void uart_rh850_rlin3_txi_isr(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	data->int_data.txi_flag = true;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback != NULL) {
		data->callback(dev, data->callback_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

static void uart_rh850_rlin3_eri_isr(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;

	data->int_data.eri_flag = true;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->callback != NULL) {
		data->callback(dev, data->callback_data);
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

#endif

static void uart_rh850_rlin3_rxi_isr(const struct device *dev)
{
	struct uart_rh850_rlin3_data *data = dev->data;
	/* Receive completion interrupt must not be disabled for poll-in API to work */
	data->int_data.rxi_flag = true;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (data->int_data.irq_rx_enable) {
		if (data->callback != NULL) {
			data->callback(dev, data->callback_data);
		}
	}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
}

static void uart_rh850_rlin3_event_handler(uart_callback_args_t *p_args)
{
	const struct device *dev = (const struct device *)p_args->p_context;
	struct uart_rh850_rlin3_data *data = dev->data;

	data->int_data.event = p_args->event;
	switch (p_args->event) {
	case UART_EVENT_RX_CHAR:
		/* Do nothing */
		break;
	case UART_EVENT_RX_COMPLETE:
		/* Do nothing */
		break;
	case UART_EVENT_TX_COMPLETE:
		/* Do nothing */
		break;
	default:
		break;
	}
}

#ifdef CONFIG_RENESAS_RH850_INTIF_INTSEL
/* Macro definition to uart rh850 rlin3 driver get irq number and ipl value */
#define UART_RH850_RLIN3_IRQ_GET(inst, type) \
		COND_CODE_1( \
		DT_NODE_HAS_COMPAT(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(inst), type),   \
							renesas_rh850_intif_intsel), \
		(DT_IRQ(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(inst), type), irq)), \
		(DT_IRQ_BY_NAME(DT_INST_PARENT(inst), type, irq)))
#define UART_RH850_RLIN3_IPL_GET(inst, type) \
			COND_CODE_1( \
			DT_NODE_HAS_COMPAT(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(inst), type),  \
								renesas_rh850_intif_intsel), \
			(DT_IRQ(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(inst), type), priority)), \
			(DT_IRQ_BY_NAME(DT_INST_PARENT(inst), type, priority)))

/* INTSEL support:
 * When RLIN3 interrupts are routed through RH850 INTSEL mux nodes
 * (interrupts-extended), generate the corresponding BSP_INTSEL
 * configuration used by the interrupt interface driver.
 */
#define _UART_RH850_RLIN3_RXI_IRQ_IS_INTSEL(rlin3_unit, mux_channel)  \
						BSP_INTSEL_RLIN3##rlin3_unit##_MUX##mux_channel
#define UART_RH850_RLIN3_RXI_IRQ_IS_INTSEL(rlin3_unit, mux_channel)   \
				_UART_RH850_RLIN3_RXI_IRQ_IS_INTSEL(rlin3_unit, mux_channel)
#define UART_RH850_RLIN3_MUX_CHAN_GET(n)        DT_PROP(DT_IRQ_INTC(DT_INST_PARENT(n)), channel)
#define UART_RH850_RLIN3_CHAN_GET(n)            DT_PROP(DT_DRV_INST(n), channel)
#define UART_RH850_RLIN3_INFO(n) \
		COND_CODE_1( \
		DT_NODE_HAS_COMPAT(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(n), rxi), \
							renesas_rh850_intif_intsel), \
		(UART_RH850_RLIN3_RXI_IRQ_IS_INTSEL(UART_RH850_RLIN3_CHAN_GET(n), \
							UART_RH850_RLIN3_MUX_CHAN_GET(n))),  \
		(BSP_INTSEL_DISABLED))
#define UART_RH850_RLIN3_INTSEL_CONFIG(n)       .intsel_irq_cfg = UART_RH850_RLIN3_INFO(n),
#else /* CONFIG_RENESAS_RH850_INTIF_INTSEL */
/* Macro definition to uart rh850 rlin3 driver get irq number and ipl value */
#define UART_RH850_RLIN3_IRQ_GET(inst, type) DT_IRQ_BY_NAME(DT_INST_PARENT(inst), type, irq)
#define UART_RH850_RLIN3_IPL_GET(inst, type) DT_IRQ_BY_NAME(DT_INST_PARENT(inst), type, priority)
#define UART_RH850_RLIN3_INTSEL_CONFIG(n)
#endif /* CONFIG_RENESAS_RH850_INTIF_INTSEL */

/* Configure txi_trigger_reg*/
#define UART_TXI_IS_INTSEL(n)   DT_NODE_HAS_COMPAT(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(n), txi), \
								renesas_rh850_intif_intsel)
#define UART_TXI_EIC_BASE_INTSEL_MUX(n)     DT_REG_ADDR_BY_NAME(                   \
						DT_PHANDLE(DT_PARENT(DT_IRQ_INTC_BY_NAME(  \
						DT_INST_PARENT(n), txi)), interrupt_parent), EIC) +\
						(2 * UART_RH850_RLIN3_IRQ_GET(n, txi))
#define UART_TXI_EIC_BASE_INTC2(n)  DT_REG_ADDR_BY_NAME(                       \
				DT_IRQ_INTC(DT_INST_PARENT(n)), EIC) + \
				(2 * UART_RH850_RLIN3_IRQ_GET(n, txi))
#define UART_TXI_TRIGGER_ADDR(n)    COND_CODE_1(UART_TXI_IS_INTSEL(n),             \
				(UART_TXI_EIC_BASE_INTSEL_MUX(n)),     \
				(UART_TXI_EIC_BASE_INTC2(n)))
#define UART_RH850_RLIN3_IRQ_CONFIG_INIT(n)                                        \
	.txi_trigger_reg = (uint16_t volatile *) IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (    \
							(UART_TXI_TRIGGER_ADDR(n)),))

/* Connect and enable a UART interrupt defined directly in the RLIN3 node */
#define UART_RH850_IRQ_CONNECT(n, irq_name, isr)                                                   \
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(n), irq_name, irq),                              \
		    DT_IRQ_BY_NAME(DT_INST_PARENT(n), irq_name, priority),     \
			isr,                                                       \
		    DEVICE_DT_INST_GET(n),                                     \
			0);                                                        \
	irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(n), irq_name, irq));
#define UART_RH850_RLIN3_IRQ_CONNECT(n)                                \
	UART_RH850_IRQ_CONNECT(n, eri, uart_rh850_rlin3_eri_isr);                                  \
	UART_RH850_IRQ_CONNECT(n, txi, uart_rh850_rlin3_txi_isr);                                  \
	UART_RH850_IRQ_CONNECT(n, rxi, uart_rh850_rlin3_rxi_isr);

/* Connect and enable a UART interrupt routed through an INTSEL MUX node */
#define UART_RH850_IRQ_CONNECT_MUX(n, irq_name, isr)                            \
	IRQ_CONNECT(DT_IRQ(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(n), irq_name), irq),  \
		   DT_IRQ(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(n), irq_name), priority),  \
		   isr,                                                                 \
		   DEVICE_DT_INST_GET(n),                                               \
		   0);                                                                  \
	irq_enable(DT_IRQ(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(n), irq_name), irq));
#define UART_RH850_RLIN3_IRQ_MUX_CONNECT(n)                                     \
			UART_RH850_IRQ_CONNECT_MUX(n, eri, uart_rh850_rlin3_eri_isr);       \
			UART_RH850_IRQ_CONNECT_MUX(n, txi, uart_rh850_rlin3_txi_isr);       \
			UART_RH850_IRQ_CONNECT_MUX(n, rxi, uart_rh850_rlin3_rxi_isr);

/* Macro for UART_RH850_RLIN3_IRQ_INIT */
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define UART_RH850_RLIN3_IRQ_INIT(n)                                            \
			COND_CODE_1(                                                        \
				DT_NODE_HAS_COMPAT(DT_IRQ_INTC(DT_INST_PARENT(n)),              \
				renesas_rh850_intif_intsel),                                    \
				(UART_RH850_RLIN3_IRQ_MUX_CONNECT(n)),                          \
				(UART_RH850_RLIN3_IRQ_CONNECT(n))                               \
			)
#else   /* CONFIG_UART_INTERRUPT_DRIVEN */
#define UART_RH850_RLIN3_IRQ_INIT(n)                                            \
			COND_CODE_1(                                                        \
				DT_NODE_HAS_COMPAT(DT_IRQ_INTC_BY_NAME(DT_INST_PARENT(n), rxi), \
				renesas_rh850_intif_intsel),                                    \
				(UART_RH850_IRQ_CONNECT_MUX(n, rxi, uart_rh850_rlin3_rxi_isr);),\
				(UART_RH850_IRQ_CONNECT(n, rxi, uart_rh850_rlin3_rxi_isr);)     \
			)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_RH850_INIT(n)                                                                         \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(n));                                                      \
	static rlin3_uart_instance_ctrl_t g_uart##n##_ctrl;                                        \
	static baud_setting_t g_uart##n##_baud_setting;                                            \
                                                                                                   \
	static rlin3_uart_extended_cfg_t g_uart##n##_cfg_extend = {                                \
		.operation_enable = UART_DIR_RX_TX,                                                \
		.invert_input = UART_NORMAL_INPUT,                                                 \
		.invert_output = UART_NORMAL_OUTPUT,                                               \
		.transfer_order = UART_LSB_FIRST,                                                  \
		.enb_overrun_err = 1,                                                              \
		.enb_frame_err = 1,                                                                \
		.noise_cancel = RLIN3_UART_NOISE_CANCELLATION_ENABLE,                              \
		.baud_calculate = RLIN3_UART_BAUD_RATE_CALCULATION_ENABLE,                         \
		.baudrate_max_err_percent = 5,                                                     \
		.p_baud_setting = &g_uart##n##_baud_setting,                                       \
		.timing_select = UART_COMPLETION,                                                  \
		UART_RH850_RLIN3_INTSEL_CONFIG(n)                                                  \
	};                                                                                         \
                                                                                                   \
	static uart_cfg_t g_uart##n##_cfg = {                                                      \
		.channel = DT_INST_PROP(n, channel),                                               \
		.p_extend = &g_uart##n##_cfg_extend,                                               \
		.p_transfer_tx = NULL,                                                             \
		.p_transfer_rx = NULL,                                                             \
		.rxi_ipl = UART_RH850_RLIN3_IPL_GET(n, rxi),                                       \
		.txi_ipl = UART_RH850_RLIN3_IPL_GET(n, txi),                                       \
		.eri_ipl = UART_RH850_RLIN3_IPL_GET(n, eri),                                       \
		.rxi_irq = UART_RH850_RLIN3_IRQ_GET(n, rxi),                                       \
		.txi_irq = UART_RH850_RLIN3_IRQ_GET(n, txi),                                       \
		.eri_irq = UART_RH850_RLIN3_IRQ_GET(n, eri),                                       \
		.p_callback = uart_rh850_rlin3_event_handler,                                      \
		.p_context = (void *)DEVICE_DT_INST_GET(n),                                        \
	};                                                                                         \
                                                                                                   \
	static const struct uart_rh850_rlin3_config uart_rh850_rlin3_config_##n = {                \
		.pin_config = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(n)),                        \
		.fsp_api = &g_uart_on_rlin3,                                                       \
	};                                                                                         \
                                                                                                   \
	static struct uart_rh850_rlin3_data uart_rh850_rlin3_data_##n = {                          \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP_OR(n, current_speed, 115200),             \
				.parity = DT_INST_ENUM_IDX_OR(n, parity, UART_CFG_PARITY_NONE),    \
				.stop_bits =                                                       \
					DT_INST_ENUM_IDX_OR(n, stop_bits, UART_CFG_STOP_BITS_1),   \
				.data_bits =                                                       \
					DT_INST_ENUM_IDX_OR(n, data_bits, UART_CFG_DATA_BITS_8),   \
			},                                                                         \
		.fsp_cfg = &g_uart##n##_cfg,                                                       \
		.fsp_ctrl = &g_uart##n##_ctrl,                                                     \
		UART_RH850_RLIN3_IRQ_CONFIG_INIT(n)};                                              \
                                                                                                   \
	static int uart_rh850_rlin3_init_##n(const struct device *dev)                             \
	{                                                                                          \
		UART_RH850_RLIN3_IRQ_INIT(n)                                                       \
		return uart_rh850_rlin3_init(dev);                                                 \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, &uart_rh850_rlin3_init_##n, NULL, &uart_rh850_rlin3_data_##n,     \
			      &uart_rh850_rlin3_config_##n, PRE_KERNEL_1,                          \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_rh850_rlin3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RH850_INIT)
