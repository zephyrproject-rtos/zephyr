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
#elif defined(CONFIG_SOC_ESP32S2)
#include <esp32s2/rom/ets_sys.h>
#include <esp32s2/rom/gpio.h>
#elif defined(CONFIG_SOC_ESP32C3)
#include <esp32c3/rom/ets_sys.h>
#include <esp32c3/rom/gpio.h>
#endif
#include <soc/uart_struct.h>
#include <soc/dport_access.h>
#include "stubs.h"
#include <hal/uart_ll.h>


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


struct uart_esp32_config {

	struct uart_device_config dev_conf;
	const struct device *clock_dev;

	const struct {
		int tx_out;
		int rx_in;
		int rts_out;
		int cts_in;
	} signals;

	const struct {
		int tx;
		int rx;
		int rts;
		int cts;
	} pins;

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
	((const struct uart_esp32_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct uart_esp32_data *)(dev)->data)
#define DEV_BASE(dev) \
	((volatile uart_dev_t *)(DEV_CFG(dev))->dev_conf.base)

#define UART_FIFO_LIMIT                 (UART_LL_FIFO_DEF_LEN)
#define UART_TX_FIFO_THRESH             0x1
#define UART_RX_FIFO_THRESH             0x16


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_esp32_isr(void *arg);
#endif

static int uart_esp32_poll_in(const struct device *dev, unsigned char *p_char)
{
	if (DEV_BASE(dev)->status.rxfifo_cnt == 0) {
		return -1;
	}

	uart_ll_read_rxfifo(DEV_BASE(dev), p_char, 1);
	return 0;
}

static IRAM_ATTR void uart_esp32_poll_out(const struct device *dev,
				unsigned char c)
{
	/* Wait for space in FIFO */
	while ((UART_FIFO_LIMIT - DEV_BASE(dev)->status.txfifo_cnt) <= 0) {
		; /* Wait */
	}

	/* Send a character */
	uart_ll_write_txfifo(DEV_BASE(dev), &c, 1);
}

static int uart_esp32_err_check(const struct device *dev)
{
	uint32_t err = DEV_BASE(dev)->int_st.parity_err
		    | DEV_BASE(dev)->int_st.frm_err;

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_esp32_config_get(const struct device *dev,
				 struct uart_config *cfg)
{
	struct uart_esp32_data *data = DEV_DATA(dev);

	cfg->baudrate = data->uart_config.baudrate;

	if (DEV_BASE(dev)->conf0.parity_en) {
		cfg->parity = DEV_BASE(dev)->conf0.parity;
	} else {
		cfg->parity = UART_CFG_PARITY_NONE;
	}

	cfg->stop_bits = DEV_BASE(dev)->conf0.stop_bit_num;
	cfg->data_bits = DEV_BASE(dev)->conf0.bit_num;

	if (DEV_BASE(dev)->conf0.tx_flow_en) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	if (DEV_BASE(dev)->conf1.rx_flow_en) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_DTR_DSR;
	}
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_esp32_set_baudrate(const struct device *dev, int baudrate)
{
	uart_ll_set_baudrate(DEV_BASE(dev), baudrate);
	return 1;
}

static int uart_esp32_configure_pins(const struct device *dev)
{
	const struct uart_esp32_config *const cfg = DEV_CFG(dev);

	esp_rom_gpio_matrix_out(cfg->pins.tx,
				  cfg->signals.tx_out,
				  false,
				  false);

	esp_rom_gpio_matrix_in(cfg->pins.rx,
				 cfg->signals.rx_in,
				 false);

	if (cfg->pins.cts) {
		esp_rom_gpio_matrix_out(cfg->pins.cts,
					  cfg->signals.cts_in,
					  false,
					  false);
	}

	if (cfg->pins.rts) {
		esp_rom_gpio_matrix_in(cfg->pins.rts,
					 cfg->signals.rts_out,
					 false);
	}

	return 0;
}

static int uart_esp32_configure(const struct device *dev,
				const struct uart_config *cfg)
{

#ifndef CONFIG_SOC_ESP32C3
	/* this register does not exist for esp32c3 uart controller */
	DEV_BASE(dev)->conf0.tick_ref_always_on = 1;
#endif

	DEV_BASE(dev)->conf1.rxfifo_full_thrhd = UART_RX_FIFO_THRESH;
	DEV_BASE(dev)->conf1.txfifo_empty_thrhd = UART_TX_FIFO_THRESH;

	uart_esp32_configure_pins(dev);
	clock_control_on(DEV_CFG(dev)->clock_dev, DEV_CFG(dev)->clock_subsys);

	/*
	 * Reset RX Buffer by reading all received bytes
	 * Hardware Reset functionality can't be used with UART 1/2
	 */
	while (DEV_BASE(dev)->status.rxfifo_cnt != 0) {
		uint8_t c;

		uart_ll_read_rxfifo(DEV_BASE(dev), &c, 1);
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		DEV_BASE(dev)->conf0.parity = 0;
		DEV_BASE(dev)->conf0.parity_en = 0;
		break;
	case UART_CFG_PARITY_EVEN:
		DEV_BASE(dev)->conf0.parity_en = 1;
		break;
	case UART_CFG_PARITY_ODD:
		DEV_BASE(dev)->conf0.parity = 1;
		DEV_BASE(dev)->conf0.parity_en = 1;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
	case UART_CFG_STOP_BITS_1_5:
	case UART_CFG_STOP_BITS_2:
		DEV_BASE(dev)->conf0.stop_bit_num = cfg->stop_bits;
		break;
	default:
		return -ENOTSUP;
	}

	if (cfg->data_bits <= UART_CFG_DATA_BITS_8) {
		DEV_BASE(dev)->conf0.bit_num = cfg->data_bits;
	} else {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		DEV_BASE(dev)->conf0.tx_flow_en = 0;
		DEV_BASE(dev)->conf1.rx_flow_en = 0;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		DEV_BASE(dev)->conf0.tx_flow_en = 1;
		DEV_BASE(dev)->conf1.rx_flow_en = 1;
		break;
	default:
		return -ENOTSUP;
	}

	if (uart_esp32_set_baudrate(dev, cfg->baudrate)) {
		DEV_DATA(dev)->uart_config.baudrate = cfg->baudrate;
	} else {
		return -ENOTSUP;
	}

	uart_ll_set_rx_tout(DEV_BASE(dev), 0x16);

	return 0;
}

static int uart_esp32_init(const struct device *dev)
{
	uart_esp32_configure(dev, &DEV_DATA(dev)->uart_config);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	DEV_DATA(dev)->irq_line =
		esp_intr_alloc(DEV_CFG(dev)->irq_source,
			0,
			(ISR_HANDLER)uart_esp32_isr,
			(void *)dev,
			NULL);
#endif
	return 0;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_esp32_fifo_fill(const struct device *dev,
				const uint8_t *tx_data, int len)
{
	int space = UART_FIFO_LIMIT - DEV_BASE(dev)->status.txfifo_cnt;

	space = MIN(len, space);
	uart_ll_write_txfifo(DEV_BASE(dev), tx_data, space);
	return space;
}

static int uart_esp32_fifo_read(const struct device *dev,
				uint8_t *rx_data, const int len)
{
	const int num_rx = DEV_BASE(dev)->status.rxfifo_cnt;

	uart_ll_read_rxfifo(DEV_BASE(dev), rx_data, num_rx);
	return num_rx;
}

static void uart_esp32_irq_tx_enable(const struct device *dev)
{
	DEV_BASE(dev)->int_clr.txfifo_empty = 1;
	DEV_BASE(dev)->int_ena.txfifo_empty = 1;
}

static void uart_esp32_irq_tx_disable(const struct device *dev)
{
	DEV_BASE(dev)->int_ena.txfifo_empty = 0;
}

static int uart_esp32_irq_tx_ready(const struct device *dev)
{
	return (DEV_BASE(dev)->status.txfifo_cnt < UART_FIFO_LIMIT);
}

static void uart_esp32_irq_rx_enable(const struct device *dev)
{
	DEV_BASE(dev)->int_clr.rxfifo_full = 1;
	DEV_BASE(dev)->int_clr.rxfifo_tout = 1;
	DEV_BASE(dev)->int_ena.rxfifo_full = 1;
	DEV_BASE(dev)->int_ena.rxfifo_tout = 1;
}

static void uart_esp32_irq_rx_disable(const struct device *dev)
{
	DEV_BASE(dev)->int_ena.rxfifo_full = 0;
	DEV_BASE(dev)->int_ena.rxfifo_tout = 0;
}

static int uart_esp32_irq_tx_complete(const struct device *dev)
{
	/* check if TX FIFO is empty */
	return (DEV_BASE(dev)->status.txfifo_cnt == 0 ? 1 : 0);
}

static int uart_esp32_irq_rx_ready(const struct device *dev)
{
	return (DEV_BASE(dev)->status.rxfifo_cnt > 0);
}

static void uart_esp32_irq_err_enable(const struct device *dev)
{
	/* enable framing, parity */
	DEV_BASE(dev)->int_ena.frm_err = 1;
	DEV_BASE(dev)->int_ena.parity_err = 1;
}

static void uart_esp32_irq_err_disable(const struct device *dev)
{
	DEV_BASE(dev)->int_ena.frm_err = 0;
	DEV_BASE(dev)->int_ena.parity_err = 0;
}

static int uart_esp32_irq_is_pending(const struct device *dev)
{
	return uart_esp32_irq_rx_ready(dev) || uart_esp32_irq_tx_ready(dev);
}

static int uart_esp32_irq_update(const struct device *dev)
{
	DEV_BASE(dev)->int_clr.rxfifo_full = 1;
	DEV_BASE(dev)->int_clr.rxfifo_tout = 1;
	DEV_BASE(dev)->int_clr.txfifo_empty = 1;
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
static const DRAM_ATTR struct uart_esp32_config uart_esp32_cfg_port_##idx = {	       \
	.dev_conf = {							       \
		.base =							       \
		    (uint8_t *)DT_REG_ADDR(DT_NODELABEL(uart##idx)), \
	},								       \
											   \
	.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(uart##idx))),		       \
											   \
	.signals = {							       \
		.tx_out = U##idx##TXD_OUT_IDX,				       \
		.rx_in = U##idx##RXD_IN_IDX,				       \
		.rts_out = U##idx##RTS_OUT_IDX,				       \
		.cts_in = U##idx##CTS_IN_IDX,				       \
	},								       \
									       \
	.pins = {							       \
		.tx = DT_PROP(DT_NODELABEL(uart##idx), tx_pin),	       \
		.rx = DT_PROP(DT_NODELABEL(uart##idx), rx_pin),	       \
		IF_ENABLED(						       \
			DT_PROP(DT_NODELABEL(uart##idx), hw_flow_control),  \
			(.rts = DT_PROP(DT_NODELABEL(uart##idx), rts_pin),  \
			.cts = DT_PROP(DT_NODELABEL(uart##idx), cts_pin),   \
			))						       \
	},								       \
											   \
	.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(DT_NODELABEL(uart##idx), offset), \
	.irq_source = DT_IRQN(DT_NODELABEL(uart##idx))			       \
};									       \
									       \
static struct uart_esp32_data uart_esp32_data_##idx = {			       \
	.uart_config = {						       \
		.baudrate = DT_PROP(DT_NODELABEL(uart##idx), current_speed),\
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
		    PRE_KERNEL_1,					       \
		    CONFIG_SERIAL_INIT_PRIORITY,			       \
		    &uart_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_UART_INIT);
