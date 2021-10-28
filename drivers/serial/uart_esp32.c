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
#include "stubs.h"
#include <hal/uart_ll.h>
#include <hal/uart_hal.h>
#include <hal/uart_types.h>

#include <drivers/gpio.h>

#include <soc/gpio_sig_map.h>
#include <soc/uart_reg.h>
#include <device.h>
#include <soc.h>
#include <drivers/uart.h>

#ifndef CONFIG_SOC_ESP32C3
#include <drivers/interrupt_controller/intc_esp32.h>
#else
#include <drivers/interrupt_controller/intc_esp32c3.h>
#endif
#include <drivers/clock_control.h>
#include <errno.h>
#include <sys/util.h>
#include <esp_attr.h>

#ifdef CONFIG_SOC_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

struct uart_esp32_pin {
	const char *gpio_name;
	int signal;
	int pin;
};

struct uart_esp32_config {
	uart_hal_context_t hal;
	const struct device *clock_dev;

	const struct uart_esp32_pin tx;
	const struct uart_esp32_pin rx;
	const struct uart_esp32_pin rts;
	const struct uart_esp32_pin cts;

	const clock_control_subsys_t clock_subsys;

	int irq_source;
};

/* driver data */
struct uart_esp32_data {
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
	int irq_line;
};

#define DEV_CFG(dev) \
	((struct uart_esp32_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct uart_esp32_data *)(dev)->data)

#define UART_FIFO_LIMIT                 (UART_LL_FIFO_DEF_LEN)
#define UART_TX_FIFO_THRESH             0x1
#define UART_RX_FIFO_THRESH             0x16


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_esp32_isr(void *arg);
#endif

static int uart_esp32_poll_in(const struct device *dev, unsigned char *p_char)
{
	int inout_rd_len = 1;

	if (uart_hal_get_rxfifo_len(&DEV_CFG(dev)->hal) == 0) {
		return -1;
	}

	uart_hal_read_rxfifo(&DEV_CFG(dev)->hal, p_char, &inout_rd_len);
	return inout_rd_len;
}

static void uart_esp32_poll_out(const struct device *dev, unsigned char c)
{
	uint32_t written;

	/* Wait for space in FIFO */
	while (uart_hal_get_txfifo_len(&DEV_CFG(dev)->hal) == 0) {
		; /* Wait */
	}

	/* Send a character */
	uart_hal_write_txfifo(&DEV_CFG(dev)->hal, &c, 1, &written);
}

static int uart_esp32_err_check(const struct device *dev)
{
	uint32_t mask = uart_hal_get_intsts_mask(&DEV_CFG(dev)->hal);
	uint32_t err = mask & (UART_INTR_PARITY_ERR | UART_INTR_FRAM_ERR);

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_esp32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	uart_parity_t parity;
	uart_stop_bits_t stop_bit;
	uart_word_length_t data_bit;
	uart_hw_flowcontrol_t hw_flow;

	uart_hal_get_baudrate(&DEV_CFG(dev)->hal, &cfg->baudrate);

	uart_hal_get_parity(&DEV_CFG(dev)->hal, &parity);
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

	uart_hal_get_stop_bits(&DEV_CFG(dev)->hal, &stop_bit);
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

	uart_hal_get_data_bit_num(&DEV_CFG(dev)->hal, &data_bit);
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

	uart_hal_get_hw_flow_ctrl(&DEV_CFG(dev)->hal, &hw_flow);
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

static int uart_esp32_configure_pins(const struct device *dev, const struct uart_config *uart)
{
	const struct uart_esp32_config *const cfg = DEV_CFG(dev);

	do {
		if (cfg->tx.gpio_name == NULL || cfg->rx.gpio_name == NULL)
			break;

		/* TX pin config */
		const struct device *tx_dev = device_get_binding(cfg->tx.gpio_name);

		if (!tx_dev)
			break;
		gpio_pin_set(tx_dev, cfg->tx.pin, 1);
		gpio_pin_configure(tx_dev, cfg->tx.pin, GPIO_OUTPUT);
		esp_rom_gpio_matrix_out(cfg->tx.pin, cfg->tx.signal, 0, 0);

		/* RX pin config */
		const struct device *rx_dev = device_get_binding(cfg->rx.gpio_name);

		if (!rx_dev)
			break;
		gpio_pin_configure(rx_dev, cfg->rx.pin, GPIO_PULL_UP | GPIO_INPUT);
		esp_rom_gpio_matrix_in(cfg->rx.pin, cfg->rx.signal, 0);

		if (uart->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
			if (cfg->rts.gpio_name == NULL || cfg->cts.gpio_name == NULL)
				break;

			/* CTS pin config */
			const struct device *cts_dev = device_get_binding(cfg->cts.gpio_name);

			if (!cts_dev)
				break;
			gpio_pin_configure(cts_dev, cfg->cts.pin, GPIO_PULL_UP | GPIO_INPUT);
			esp_rom_gpio_matrix_in(cfg->cts.pin, cfg->cts.signal, 0);

			/* RTS pin config */
			const struct device *rts_dev = device_get_binding(cfg->rts.gpio_name);

			if (!rts_dev)
				break;
			gpio_pin_configure(rts_dev, cfg->rts.pin, GPIO_OUTPUT);
			esp_rom_gpio_matrix_out(cfg->rts.pin, cfg->rts.signal, 0, 0);
		}
		return 0;

	} while (0);

	return -EINVAL;
}

static int uart_esp32_configure(const struct device *dev, const struct uart_config *cfg)
{
	int ret = uart_esp32_configure_pins(dev, cfg);

	if (ret < 0) {
		return ret;
	}

	clock_control_on(DEV_CFG(dev)->clock_dev, DEV_CFG(dev)->clock_subsys);

	uart_hal_set_sclk(&DEV_CFG(dev)->hal, UART_SCLK_APB);
	uart_hal_set_rxfifo_full_thr(&DEV_CFG(dev)->hal, UART_RX_FIFO_THRESH);
	uart_hal_set_txfifo_empty_thr(&DEV_CFG(dev)->hal, UART_TX_FIFO_THRESH);
	uart_hal_rxfifo_rst(&DEV_CFG(dev)->hal);

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_hal_set_parity(&DEV_CFG(dev)->hal, UART_PARITY_DISABLE);
		break;
	case UART_CFG_PARITY_EVEN:
		uart_hal_set_parity(&DEV_CFG(dev)->hal, UART_PARITY_EVEN);
		break;
	case UART_CFG_PARITY_ODD:
		uart_hal_set_parity(&DEV_CFG(dev)->hal, UART_PARITY_ODD);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_hal_set_stop_bits(&DEV_CFG(dev)->hal, UART_STOP_BITS_1);
		break;
	case UART_CFG_STOP_BITS_1_5:
		uart_hal_set_stop_bits(&DEV_CFG(dev)->hal, UART_STOP_BITS_1_5);
		break;
	case UART_CFG_STOP_BITS_2:
		uart_hal_set_stop_bits(&DEV_CFG(dev)->hal, UART_STOP_BITS_2);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		uart_hal_set_data_bit_num(&DEV_CFG(dev)->hal, UART_DATA_5_BITS);
		break;
	case UART_CFG_DATA_BITS_6:
		uart_hal_set_data_bit_num(&DEV_CFG(dev)->hal, UART_DATA_6_BITS);
		break;
	case UART_CFG_DATA_BITS_7:
		uart_hal_set_data_bit_num(&DEV_CFG(dev)->hal, UART_DATA_7_BITS);
		break;
	case UART_CFG_DATA_BITS_8:
		uart_hal_set_data_bit_num(&DEV_CFG(dev)->hal, UART_DATA_8_BITS);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_hal_set_hw_flow_ctrl(&DEV_CFG(dev)->hal, UART_HW_FLOWCTRL_DISABLE, 0);
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_hal_set_hw_flow_ctrl(&DEV_CFG(dev)->hal, UART_HW_FLOWCTRL_CTS_RTS, 10);
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_set_baudrate(&DEV_CFG(dev)->hal, cfg->baudrate);

	uart_hal_set_rx_timeout(&DEV_CFG(dev)->hal, 0x16);

	return 0;
}

static int uart_esp32_init(const struct device *dev)
{
	int ret = uart_esp32_configure(dev, &DEV_DATA(dev)->uart_config);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	DEV_DATA(dev)->irq_line =
		esp_intr_alloc(DEV_CFG(dev)->irq_source,
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
	uint32_t written = 0;

	uart_hal_write_txfifo(&DEV_CFG(dev)->hal, tx_data, len, &written);
	return written;
}

static int uart_esp32_fifo_read(const struct device *dev,
				uint8_t *rx_data, const int len)
{
	const int num_rx = uart_hal_get_rxfifo_len(&DEV_CFG(dev)->hal);
	int read = MIN(len, num_rx);

	if (!read) {
		return 0;
	}

	uart_hal_read_rxfifo(&DEV_CFG(dev)->hal, rx_data, &read);
	return read;
}

static void uart_esp32_irq_tx_enable(const struct device *dev)
{
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, UART_INTR_TXFIFO_EMPTY);
	uart_hal_ena_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_TXFIFO_EMPTY);
}

static void uart_esp32_irq_tx_disable(const struct device *dev)
{
	uart_hal_disable_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_TXFIFO_EMPTY);
}

static int uart_esp32_irq_tx_ready(const struct device *dev)
{
	return (uart_hal_get_txfifo_len(&DEV_CFG(dev)->hal) > 0);
}

static void uart_esp32_irq_rx_enable(const struct device *dev)
{
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_ena_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_ena_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_TOUT);
}

static void uart_esp32_irq_rx_disable(const struct device *dev)
{
	uart_hal_disable_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_disable_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_TOUT);
}

static int uart_esp32_irq_tx_complete(const struct device *dev)
{
	/* check if TX FIFO is empty */
	return (uart_hal_get_txfifo_len(&DEV_CFG(dev)->hal) == UART_LL_FIFO_DEF_LEN ? 1 : 0);
}

static int uart_esp32_irq_rx_ready(const struct device *dev)
{
	return (uart_hal_get_rxfifo_len(&DEV_CFG(dev)->hal) > 0);
}

static void uart_esp32_irq_err_enable(const struct device *dev)
{
	/* enable framing, parity */
	uart_hal_ena_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_FRAM_ERR);
	uart_hal_ena_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_PARITY_ERR);
}

static void uart_esp32_irq_err_disable(const struct device *dev)
{
	uart_hal_disable_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_FRAM_ERR);
	uart_hal_disable_intr_mask(&DEV_CFG(dev)->hal, UART_INTR_PARITY_ERR);
}

static int uart_esp32_irq_is_pending(const struct device *dev)
{
	return uart_esp32_irq_rx_ready(dev) || uart_esp32_irq_tx_ready(dev);
}

static int uart_esp32_irq_update(const struct device *dev)
{
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, UART_INTR_TXFIFO_EMPTY);

	return 1;
}

static void uart_esp32_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	DEV_DATA(dev)->irq_cb = cb;
	DEV_DATA(dev)->irq_cb_data = cb_data;
}

static void uart_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct uart_esp32_data *data = DEV_DATA(dev);
	uint32_t uart_intr_status = uart_hal_get_intsts_mask(&DEV_CFG(dev)->hal);

	if (uart_intr_status == 0) {
		return;
	}
	uart_hal_clr_intsts_mask(&DEV_CFG(dev)->hal, uart_intr_status);

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

#define GPIO0_NAME COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay), \
			     (DT_LABEL(DT_INST(0, espressif_esp32_gpio))), (NULL))
#define GPIO1_NAME COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay), \
			     (DT_LABEL(DT_INST(1, espressif_esp32_gpio))), (NULL))

#define DT_UART_ESP32_GPIO_NAME(idx, pin) ( \
	DT_INST_PROP(idx, pin) < 32 ? GPIO0_NAME : GPIO1_NAME)

#define ESP32_UART_INIT(idx)						       \
static const DRAM_ATTR struct uart_esp32_config uart_esp32_cfg_port_##idx = {	       \
	.hal = {							       \
		.dev =							       \
		    (uart_dev_t *)DT_REG_ADDR(DT_NODELABEL(uart##idx)), \
	},								       \
	.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(uart##idx))),		       \
	.tx = {	\
		.signal = U##idx##TXD_OUT_IDX,	\
		.pin = DT_INST_PROP(idx, tx_pin),	\
		.gpio_name = DT_UART_ESP32_GPIO_NAME(idx, tx_pin),	\
	},	\
	.rx = {	\
		.signal = U##idx##RXD_IN_IDX,	\
		.pin = DT_INST_PROP(idx, rx_pin),	\
		.gpio_name = DT_UART_ESP32_GPIO_NAME(idx, rx_pin),	\
	},	\
	IF_ENABLED(DT_PROP(DT_NODELABEL(uart##idx), hw_flow_control), (	\
	.rts = {	\
		.signal = U##idx##RTS_OUT_IDX,	\
		.pin = DT_INST_PROP(idx, rts_pin),	\
		.gpio_name = DT_UART_ESP32_GPIO_NAME(idx, rts_pin),	\
	},	\
	.cts = {	\
		.signal = U##idx##CTS_IN_IDX,	\
		.pin = DT_INST_PROP(idx, cts_pin),	\
		.gpio_name = DT_UART_ESP32_GPIO_NAME(idx, cts_pin),	\
	},))	\
	.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(DT_NODELABEL(uart##idx), offset), \
	.irq_source = DT_IRQN(DT_NODELABEL(uart##idx))			       \
};									       \
									       \
static struct uart_esp32_data uart_esp32_data_##idx = {			       \
	.uart_config = {						       \
		.baudrate = DT_INST_PROP(idx, current_speed),\
		.parity = UART_CFG_PARITY_NONE,				       \
		.stop_bits = UART_CFG_STOP_BITS_1,			       \
		.data_bits = UART_CFG_DATA_BITS_8,			       \
		.flow_ctrl = IS_ENABLED(				       \
			DT_PROP(DT_NODELABEL(uart##idx), hw_flow_control)) ?\
			UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE   \
	}								       \
};									       \
									       \
DEVICE_DT_DEFINE(DT_NODELABEL(uart##idx),				       \
		    &uart_esp32_init,					       \
		    NULL,				       \
		    &uart_esp32_data_##idx,				       \
		    &uart_esp32_cfg_port_##idx,				       \
		    POST_KERNEL,					       \
		    CONFIG_SERIAL_INIT_PRIORITY,			       \
		    &uart_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_UART_INIT);
