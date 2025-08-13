/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_uart_sci

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include "r_sci_rx_if.h"
#include "iodefine_sci.h"

#if CONFIG_SOC_SERIES_RX130
#include "r_sci_rx130_private.h"
#elif CONFIG_SOC_SERIES_RX261
#include "r_sci_rx261_private.h"
#else
#error Unknown SOC, not (yet) supported.
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rx_uart_sci, CONFIG_UART_LOG_LEVEL);

#define DEV_CFG(dev)  ((const struct uart_rx_sci_config *const)(dev)->config)
#define DEV_BASE(dev) (DEV_CFG(dev)->regs)

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static void uart_rx_sci_txi_isr(const struct device *dev);
#endif

/* SCI SCR register */
#define R_SCI_SCR_TIE_Pos  (SCI_BIT7)
#define R_SCI_SCR_RIE_Pos  (SCI_BIT6)
#define R_SCI_SCR_TEIE_Pos (SCI_BIT2)

/* SCI SSR register */
#define R_SCI_SSR_TDRE_Pos (SCI_BIT7)
#define R_SCI_SSR_RDRF_Pos (SCI_BIT6)
#define R_SCI_SSR_ORER_Pos (SCI_BIT5)
#define R_SCI_SSR_FER_Pos  (SCI_BIT4)
#define R_SCI_SSR_PER_Pos  (SCI_BIT3)
#define R_SCI_SSR_TEND_Pos (SCI_BIT2)

struct uart_rx_sci_config {
	uint32_t regs;
	const struct pinctrl_dev_config *pcfg;
};

struct uart_rx_sci_data {
	const struct device *dev;
	uint8_t channel;
	sci_hdl_t hdl;
	struct uart_config uart_config;
	sci_cfg_t sci_config;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uint8_t rxi_irq;
	uint8_t txi_irq;
	uint8_t tei_irq;
	uint8_t eri_irq;
	uart_irq_callback_user_data_t user_cb;
	void *user_cb_data;
#endif
};

static int uart_rx_sci_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	if (IS_ENABLED(CONFIG_UART_ASYNC_API) && sci->SCR.BIT.RIE) {
		return -EBUSY;
	}

	if (sci->SSR.BIT.RDRF == 0U) {
		/* There are no characters available to read. */
		return -1;
	}

	*c = (unsigned char)sci->RDR;

	return 0;
}

static void uart_rx_sci_poll_out(const struct device *dev, unsigned char c)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	while (sci->SSR.BIT.TEND == 0U) {
	}

	sci->TDR = c;
}

static int uart_rx_err_check(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	const uint32_t status = sci->SSR.BYTE;
	int errors = 0;

	if ((status & BIT(R_SCI_SSR_ORER_Pos)) != 0) {
		errors |= UART_ERROR_OVERRUN;
		sci->SSR.BIT.ORER = 0;
	}
	if ((status & BIT(R_SCI_SSR_PER_Pos)) != 0) {
		errors |= UART_ERROR_PARITY;
		sci->SSR.BIT.PER = 0;
	}
	if ((status & BIT(R_SCI_SSR_FER_Pos)) != 0) {
		errors |= UART_ERROR_FRAMING;
		sci->SSR.BIT.FER = 0;
	}

	return errors;
}

static int uart_rx_sci_apply_config(const struct uart_config *config, sci_cfg_t *uart_config)
{

	switch (config->data_bits) {
	case UART_CFG_DATA_BITS_5:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_6:
		return -ENOTSUP;
	case UART_CFG_DATA_BITS_7:
		uart_config->async.data_size = SCI_DATA_7BIT;
		break;
	case UART_CFG_DATA_BITS_8:
		uart_config->async.data_size = SCI_DATA_8BIT;
		break;
	case UART_CFG_DATA_BITS_9:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		uart_config->async.parity_en = SCI_PARITY_OFF;
		uart_config->async.parity_type = SCI_EVEN_PARITY;
		break;
	case UART_CFG_PARITY_ODD:
		uart_config->async.parity_en = SCI_PARITY_ON;
		uart_config->async.parity_type = SCI_ODD_PARITY;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_config->async.parity_en = SCI_PARITY_ON;
		uart_config->async.parity_type = SCI_EVEN_PARITY;
		break;
	case UART_CFG_PARITY_MARK:
		return -ENOTSUP;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_1:
		uart_config->async.stop_bits = SCI_STOPBITS_1;
		break;
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_2:
		uart_config->async.stop_bits = SCI_STOPBITS_2;
		break;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	uart_config->async.baud_rate = config->baudrate;
	uart_config->async.clk_src = SCI_CLK_INT;
	uart_config->async.int_priority = 4;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static int uart_rx_configure(const struct device *dev, const struct uart_config *cfg)
{
	int err;
	sci_err_t sci_err;
	struct uart_rx_sci_data *data = dev->data;

	err = uart_rx_sci_apply_config(cfg, &data->sci_config);
	if (err) {
		return err;
	}

	sci_err = R_SCI_Close(data->hdl);

	if (sci_err) {
		return -EIO;
	}

	sci_err = R_SCI_Open(data->channel, SCI_MODE_ASYNC, &data->sci_config, NULL, &data->hdl);

	if (sci_err) {
		return -EIO;
	}

	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	return 0;
}

static int uart_rx_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rx_sci_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(*cfg));

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_rx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	uint8_t num_tx = 0U;

	if (size > 0 && sci->SSR.BIT.TDRE) {
		/* Send a character (8bit , parity none) */
		sci->TDR = tx_data[num_tx++];
	}

	return num_tx;
}

static int uart_rx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	uint8_t num_rx = 0U;

	if (size > 0 && sci->SSR.BIT.RDRF) {
		/* Receive a character (8bit , parity none) */
		rx_data[num_rx++] = sci->RDR;
	}

	return num_rx;
}

static void uart_rx_irq_tx_enable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BYTE |= (BIT(R_SCI_SCR_TIE_Pos) | BIT(R_SCI_SCR_TEIE_Pos));
	irq_enable(data->tei_irq);
	if (sci->SSR.BIT.TDRE) {
		/* the callback function is usually called from an interrupt,
		 * preventing other interrupts to be triggered during execution
		 * just to be sure, lock interrupts while the callback is
		 * handled.
		 */
		uint32_t key = irq_lock();

		uart_rx_sci_txi_isr(dev);
		irq_unlock(key);
	}
}

static void uart_rx_irq_tx_disable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BYTE &= ~(BIT(R_SCI_SCR_TIE_Pos) | BIT(R_SCI_SCR_TEIE_Pos));
	irq_disable(data->tei_irq);
}

static int uart_rx_irq_tx_ready(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	return (sci->SCR.BIT.TIE == 1U) &&
	       (sci->SSR.BYTE & (BIT(R_SCI_SSR_TDRE_Pos) | BIT(R_SCI_SSR_TEND_Pos)));
}

static int uart_rx_irq_tx_complete(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	return (sci->SCR.BIT.TEIE == 1U) && (sci->SSR.BYTE & BIT(R_SCI_SSR_TEND_Pos));
}

static void uart_rx_irq_rx_enable(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.RIE = 1U;
}

static void uart_rx_irq_rx_disable(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	sci->SCR.BIT.RIE = 0U;
}

static int uart_rx_irq_rx_ready(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);

	return (sci->SCR.BIT.RIE == 1U) && ((sci->SSR.BYTE & BIT(R_SCI_SSR_RDRF_Pos)));
}

static void uart_rx_irq_err_enable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	irq_enable(data->eri_irq);
}

static void uart_rx_irq_err_disable(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	irq_disable(data->eri_irq);
}

static int uart_rx_irq_is_pending(const struct device *dev)
{
	volatile struct st_sci *sci = (struct st_sci *)DEV_BASE(dev);
	bool tx_pending = false;
	bool rx_pending = false;

	tx_pending = ((sci->SCR.BYTE & BIT(R_SCI_SCR_TIE_Pos)) &&
		      (sci->SSR.BYTE & (BIT(R_SCI_SSR_TEND_Pos) | BIT(R_SCI_SSR_TDRE_Pos))));
	rx_pending = ((sci->SCR.BYTE & BIT(R_SCI_SCR_RIE_Pos)) &&
		      ((sci->SSR.BYTE & (BIT(R_SCI_SSR_RDRF_Pos) | BIT(R_SCI_SSR_PER_Pos) |
					 BIT(R_SCI_SSR_FER_Pos) | BIT(R_SCI_SSR_ORER_Pos)))));

	return tx_pending || rx_pending;
}

static int uart_rx_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_rx_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct uart_rx_sci_data *data = dev->data;

	data->user_cb = cb;
	data->user_cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_rx_init(const struct device *dev)
{
	const struct uart_rx_sci_config *config = dev->config;
	struct uart_rx_sci_data *data = dev->data;
	int ret;
	sci_err_t sci_err;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = uart_rx_sci_apply_config(&data->uart_config, &data->sci_config);
	if (ret) {
		return ret;
	}

	sci_err = R_SCI_Open(data->channel, SCI_MODE_ASYNC, &data->sci_config, NULL, &data->hdl);

	if (sci_err) {
		return -EIO;
	}

	/* Set the Asynchronous Start Bit Edge Detection Select to falling edge on the RXDn pin */
	sci_err = R_SCI_Control(data->hdl, SCI_CMD_START_BIT_EDGE, FIT_NO_PTR);

	if (sci_err) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(uart, uart_rx_driver_api) = {
	.poll_in = uart_rx_sci_poll_in,
	.poll_out = uart_rx_sci_poll_out,
	.err_check = uart_rx_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rx_configure,
	.config_get = uart_rx_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rx_fifo_fill,
	.fifo_read = uart_rx_fifo_read,
	.irq_tx_enable = uart_rx_irq_tx_enable,
	.irq_tx_disable = uart_rx_irq_tx_disable,
	.irq_tx_ready = uart_rx_irq_tx_ready,
	.irq_rx_enable = uart_rx_irq_rx_enable,
	.irq_rx_disable = uart_rx_irq_rx_disable,
	.irq_tx_complete = uart_rx_irq_tx_complete,
	.irq_rx_ready = uart_rx_irq_rx_ready,
	.irq_err_enable = uart_rx_irq_err_enable,
	.irq_err_disable = uart_rx_irq_err_disable,
	.irq_is_pending = uart_rx_irq_is_pending,
	.irq_update = uart_rx_irq_update,
	.irq_callback_set = uart_rx_irq_callback_set,
#endif
};

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
static void uart_rx_sci_rxi_isr(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
}

static void uart_rx_sci_txi_isr(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
}

static void uart_rx_sci_tei_isr(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
}

static void uart_rx_sci_eri_isr(const struct device *dev)
{
	struct uart_rx_sci_data *data = dev->data;

	if (data->user_cb != NULL) {
		data->user_cb(dev, data->user_cb_data);
	}
}
#endif

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
#define UART_RX_SCI_IRQ_INIT(index)                                                                \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, priority),                  \
			    uart_rx_sci_rxi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, priority),                  \
			    uart_rx_sci_txi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, priority),                  \
			    uart_rx_sci_tei_isr, DEVICE_DT_INST_GET(index), 0);                    \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),                       \
			    DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, priority),                  \
			    uart_rx_sci_eri_isr, DEVICE_DT_INST_GET(index), 0);                    \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq));                       \
		irq_enable(DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq));                       \
	} while (0)
#else
#define UART_RX_SCI_IRQ_INIT(index)
#endif

#define UART_RX_INIT(index)                                                                        \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
                                                                                                   \
	static const struct uart_rx_sci_config uart_rx_sci_config_##index = {                      \
		.regs = DT_REG_ADDR(DT_INST_PARENT(index)),                                        \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                          \
	};                                                                                         \
                                                                                                   \
	static struct uart_rx_sci_data uart_rx_sci_data_##index = {                                \
		.dev = DEVICE_DT_GET(DT_DRV_INST(index)),                                          \
		.channel = DT_PROP(DT_INST_PARENT(index), channel),                                \
		.sci_config = {},                                                                  \
		.uart_config =                                                                     \
			{                                                                          \
				.baudrate = DT_INST_PROP(index, current_speed),                    \
				.parity = UART_CFG_PARITY_NONE,                                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                                 \
				.data_bits = UART_CFG_DATA_BITS_8,                                 \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
		.rxi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), rxi, irq),                        \
		.txi_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), txi, irq),                        \
		.tei_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), tei, irq),                        \
		.eri_irq = DT_IRQ_BY_NAME(DT_INST_PARENT(index), eri, irq),                        \
	};                                                                                         \
                                                                                                   \
	static int uart_rx_init_##index(const struct device *dev)                                  \
	{                                                                                          \
		UART_RX_SCI_IRQ_INIT(index);                                                       \
		int err = uart_rx_init(dev);                                                       \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, uart_rx_init_##index, NULL, &uart_rx_sci_data_##index,        \
			      &uart_rx_sci_config_##index, PRE_KERNEL_1,                           \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RX_INIT)
