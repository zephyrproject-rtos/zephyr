/*
 * Copyright (c) 2019 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_uart

/* Include esp-idf headers first to avoid redefining BIT() macro */
/* TODO: include w/o prefix */
#ifdef CONFIG_SOC_ESP32
#include <esp32/rom/ets_sys.h>
#include <esp32/rom/gpio.h>
#include <soc/dport_reg.h>
#elif defined(CONFIG_SOC_ESP32S2)
#include <esp32s2/rom/ets_sys.h>
#include <esp32s2/rom/gpio.h>
#include <soc/dport_reg.h>
#elif defined(CONFIG_SOC_ESP32C3)
#include <esp32c3/rom/ets_sys.h>
#include <esp32c3/rom/gpio.h>
#endif
#include <soc/uart_struct.h>
#include <hal/uart_ll.h>
#include <hal/uart_hal.h>
#include <hal/uart_types.h>

#include <zephyr/drivers/pinctrl.h>

#include <soc/uart_reg.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>

#ifndef CONFIG_SOC_ESP32C3
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#endif
#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <esp_attr.h>

#ifdef CONFIG_SOC_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

struct uart_esp32_config {
	const struct device *clock_dev;

	const struct pinctrl_dev_config *pcfg;

	const clock_control_subsys_t clock_subsys;

	int irq_source;
};

/* driver data */
struct uart_esp32_data {
	struct uart_config uart_config;
	uart_hal_context_t hal;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
	int irq_line;
};

#define UART_FIFO_LIMIT                 (UART_LL_FIFO_DEF_LEN)
#define UART_TX_FIFO_THRESH             0x1
#define UART_RX_FIFO_THRESH             0x16


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_esp32_isr(void *arg);
#endif

static int uart_esp32_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_esp32_data *data = dev->data;
	int inout_rd_len = 1;

	if (uart_hal_get_rxfifo_len(&data->hal) == 0) {
		return -1;
	}

	uart_hal_read_rxfifo(&data->hal, p_char, &inout_rd_len);

	return 0;
}

static void uart_esp32_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_esp32_data *data = dev->data;
	uint32_t written;

	/* Wait for space in FIFO */
	while (uart_hal_get_txfifo_len(&data->hal) == 0) {
		; /* Wait */
	}

	/* Send a character */
	uart_hal_write_txfifo(&data->hal, &c, 1, &written);
}

static int uart_esp32_err_check(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;
	uint32_t mask = uart_hal_get_intsts_mask(&data->hal);
	uint32_t err = mask & (UART_INTR_PARITY_ERR | UART_INTR_FRAM_ERR);

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_esp32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_esp32_data *data = dev->data;
	uart_parity_t parity;
	uart_stop_bits_t stop_bit;
	uart_word_length_t data_bit;
	uart_hw_flowcontrol_t hw_flow;

	cfg->baudrate = data->uart_config.baudrate;

	uart_hal_get_parity(&data->hal, &parity);
	switch (parity) {
	case UART_PARITY_DISABLE:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	case UART_PARITY_EVEN:
		cfg->parity = UART_CFG_PARITY_EVEN;
		break;
	case UART_PARITY_ODD:
		cfg->parity = UART_CFG_PARITY_ODD;
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_stop_bits(&data->hal, &stop_bit);
	switch (stop_bit) {
	case UART_STOP_BITS_1:
		cfg->stop_bits = UART_CFG_STOP_BITS_1;
		break;
	case UART_STOP_BITS_1_5:
		cfg->stop_bits = UART_CFG_STOP_BITS_1_5;
		break;
	case UART_STOP_BITS_2:
		cfg->stop_bits = UART_CFG_STOP_BITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_data_bit_num(&data->hal, &data_bit);
	switch (data_bit) {
	case UART_DATA_5_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_5;
		break;
	case UART_DATA_6_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_6;
		break;
	case UART_DATA_7_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_7;
		break;
	case UART_DATA_8_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_hw_flow_ctrl(&data->hal, &hw_flow);
	switch (hw_flow) {
	case UART_HW_FLOWCTRL_DISABLE:
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
		break;
	case UART_HW_FLOWCTRL_CTS_RTS:
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_esp32_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_esp32_config *config = dev->config;
	struct uart_esp32_data *data = dev->data;
	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}

	clock_control_on(config->clock_dev, config->clock_subsys);

	uart_hal_set_sclk(&data->hal, UART_SCLK_APB);
	uart_hal_set_rxfifo_full_thr(&data->hal, UART_RX_FIFO_THRESH);
	uart_hal_set_txfifo_empty_thr(&data->hal, UART_TX_FIFO_THRESH);
	uart_hal_rxfifo_rst(&data->hal);

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_hal_set_parity(&data->hal, UART_PARITY_DISABLE);
		break;
	case UART_CFG_PARITY_EVEN:
		uart_hal_set_parity(&data->hal, UART_PARITY_EVEN);
		break;
	case UART_CFG_PARITY_ODD:
		uart_hal_set_parity(&data->hal, UART_PARITY_ODD);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_hal_set_stop_bits(&data->hal, UART_STOP_BITS_1);
		break;
	case UART_CFG_STOP_BITS_1_5:
		uart_hal_set_stop_bits(&data->hal, UART_STOP_BITS_1_5);
		break;
	case UART_CFG_STOP_BITS_2:
		uart_hal_set_stop_bits(&data->hal, UART_STOP_BITS_2);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_5_BITS);
		break;
	case UART_CFG_DATA_BITS_6:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_6_BITS);
		break;
	case UART_CFG_DATA_BITS_7:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_7_BITS);
		break;
	case UART_CFG_DATA_BITS_8:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_8_BITS);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_hal_set_hw_flow_ctrl(&data->hal, UART_HW_FLOWCTRL_DISABLE, 0);
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_hal_set_hw_flow_ctrl(&data->hal, UART_HW_FLOWCTRL_CTS_RTS, 10);
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_set_baudrate(&data->hal, cfg->baudrate);

	uart_hal_set_rx_timeout(&data->hal, 0x16);

	return 0;
}

static int uart_esp32_init(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;
	int ret = uart_esp32_configure(dev, &data->uart_config);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_esp32_config *config = dev->config;

	data->irq_line =
		esp_intr_alloc(config->irq_source,
			0,
			(ISR_HANDLER)uart_esp32_isr,
			(void *)dev,
			NULL);
#endif
	return ret;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_esp32_fifo_fill(const struct device *dev,
				const uint8_t *tx_data, int len)
{
	struct uart_esp32_data *data = dev->data;
	uint32_t written = 0;

	if (len < 0) {
		return 0;
	}

	uart_hal_write_txfifo(&data->hal, tx_data, len, &written);
	return written;
}

static int uart_esp32_fifo_read(const struct device *dev,
				uint8_t *rx_data, const int len)
{
	struct uart_esp32_data *data = dev->data;
	const int num_rx = uart_hal_get_rxfifo_len(&data->hal);
	int read = MIN(len, num_rx);

	if (!read) {
		return 0;
	}

	uart_hal_read_rxfifo(&data->hal, rx_data, &read);
	return read;
}

static void uart_esp32_irq_tx_enable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static void uart_esp32_irq_tx_disable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static int uart_esp32_irq_tx_ready(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	return (uart_hal_get_txfifo_len(&data->hal) > 0 &&
			uart_hal_get_intr_ena_status(&data->hal) & UART_INTR_TXFIFO_EMPTY);
}

static void uart_esp32_irq_rx_enable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
}

static void uart_esp32_irq_rx_disable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_disable_intr_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
}

static int uart_esp32_irq_tx_complete(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	return uart_hal_is_tx_idle(&data->hal);
}

static int uart_esp32_irq_rx_ready(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	return (uart_hal_get_rxfifo_len(&data->hal) > 0);
}

static void uart_esp32_irq_err_enable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	/* enable framing, parity */
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_FRAM_ERR);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_PARITY_ERR);
}

static void uart_esp32_irq_err_disable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_FRAM_ERR);
	uart_hal_disable_intr_mask(&data->hal, UART_INTR_PARITY_ERR);
}

static int uart_esp32_irq_is_pending(const struct device *dev)
{
	return uart_esp32_irq_rx_ready(dev) || uart_esp32_irq_tx_ready(dev);
}

static int uart_esp32_irq_update(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);

	return 1;
}

static void uart_esp32_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_esp32_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void uart_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct uart_esp32_data *data = dev->data;
	uint32_t uart_intr_status = uart_hal_get_intsts_mask(&data->hal);

	if (uart_intr_status == 0) {
		return;
	}
	uart_hal_clr_intsts_mask(&data->hal, uart_intr_status);

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const DRAM_ATTR struct uart_driver_api uart_esp32_api = {
	.poll_in = uart_esp32_poll_in,
	.poll_out = uart_esp32_poll_out,
	.err_check = uart_esp32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure =  uart_esp32_configure,
	.config_get = uart_esp32_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_esp32_fifo_fill,
	.fifo_read = uart_esp32_fifo_read,
	.irq_tx_enable = uart_esp32_irq_tx_enable,
	.irq_tx_disable = uart_esp32_irq_tx_disable,
	.irq_tx_ready = uart_esp32_irq_tx_ready,
	.irq_rx_enable = uart_esp32_irq_rx_enable,
	.irq_rx_disable = uart_esp32_irq_rx_disable,
	.irq_tx_complete = uart_esp32_irq_tx_complete,
	.irq_rx_ready = uart_esp32_irq_rx_ready,
	.irq_err_enable = uart_esp32_irq_err_enable,
	.irq_err_disable = uart_esp32_irq_err_disable,
	.irq_is_pending = uart_esp32_irq_is_pending,
	.irq_update = uart_esp32_irq_update,
	.irq_callback_set = uart_esp32_irq_callback_set,
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define ESP32_UART_INIT(idx)						       \
										\
PINCTRL_DT_INST_DEFINE(idx);							\
										\
static const DRAM_ATTR struct uart_esp32_config uart_esp32_cfg_port_##idx = {	       \
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),		       \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),				\
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset), \
	.irq_source = DT_INST_IRQN(idx)			       \
};									       \
									       \
static struct uart_esp32_data uart_esp32_data_##idx = {			       \
	.uart_config = {						       \
		.baudrate = DT_INST_PROP(idx, current_speed),\
		.parity = UART_CFG_PARITY_NONE,				       \
		.stop_bits = UART_CFG_STOP_BITS_1,			       \
		.data_bits = UART_CFG_DATA_BITS_8,			       \
		.flow_ctrl =	\
			COND_CODE_1(DT_NODE_HAS_PROP(idx, hw_flow_control),	\
		    (UART_CFG_FLOW_CTRL_RTS_CTS), (UART_CFG_FLOW_CTRL_NONE))	\
	},								       \
	.hal = {							       \
		.dev =							       \
		    (uart_dev_t *)DT_INST_REG_ADDR(idx), \
	},								       \
};									       \
									       \
DEVICE_DT_INST_DEFINE(idx,				       \
		    &uart_esp32_init,					       \
		    NULL,				       \
		    &uart_esp32_data_##idx,				       \
		    &uart_esp32_cfg_port_##idx,				       \
		    PRE_KERNEL_1,					       \
		    CONFIG_SERIAL_INIT_PRIORITY,			       \
		    &uart_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_UART_INIT);
