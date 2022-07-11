/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_usb_serial

#include "stubs.h"
#include <hal/usb_serial_jtag_ll.h>

#include <soc/uart_reg.h>
#include <device.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <esp_attr.h>

#define ISR_HANDLER isr_handler_t

struct serial_esp32_usb_pin {
	const char *gpio_name;
	int pin;
};

struct serial_esp32_usb_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	uint8_t uart_num;
	int irq_source;
};

struct serial_esp32_usb_data {
	struct uart_config serial_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
	int irq_line;
};

#define SERIAL_FIFO_LIMIT (USB_SERIAL_JTAG_PACKET_SZ_BYTES)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void serial_esp32_usb_isr(void *arg);
#endif

static int serial_esp32_usb_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct serial_esp32_usb_data *data = dev->data;

	if (!usb_serial_jtag_ll_rxfifo_data_available()) {
		return -1;
	}

	usb_serial_jtag_ll_read_rxfifo(p_char, 1);

	return 0;
}

static void serial_esp32_usb_poll_out(const struct device *dev, unsigned char c)
{
	ARG_UNUSED(dev);

	/* Wait for space in FIFO */
	while (usb_serial_jtag_ll_txfifo_writable() == 0) {
		;
	}

	/* Send a character */
	usb_serial_jtag_ll_write_txfifo(&c, 1);

	usb_serial_jtag_ll_txfifo_flush();
}

static int serial_esp32_usb_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int serial_esp32_usb_config_get(const struct device *dev, struct uart_config *cfg)
{
	return -ENOTSUP;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int serial_esp32_usb_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct serial_esp32_usb_config *config = dev->config;
	struct serial_esp32_usb_data *data = dev->data;

	clock_control_on(config->clock_dev, config->clock_subsys);

	return 0;
}

static int serial_esp32_usb_init(const struct device *dev)
{
	struct serial_esp32_usb_data *data = dev->data;
	int ret = serial_esp32_usb_configure(dev, &data->serial_config);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct serial_esp32_usb_config *config = dev->config;

	data->irq_line = esp_intr_alloc(config->irq_source, 0, (ISR_HANDLER)serial_esp32_usb_isr,
					(void *)dev, NULL);
#endif
	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int serial_esp32_usb_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	ARG_UNUSED(dev);

	int ret = usb_serial_jtag_ll_write_txfifo(tx_data, len);

	usb_serial_jtag_ll_txfifo_flush();

	return ret;
}

static int serial_esp32_usb_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	ARG_UNUSED(dev);

	return usb_serial_jtag_ll_read_rxfifo(rx_data, len);
}

static void serial_esp32_usb_irq_tx_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
	usb_serial_jtag_ll_ena_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
}

static void serial_esp32_usb_irq_tx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	usb_serial_jtag_ll_disable_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
}

static int serial_esp32_usb_irq_tx_ready(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (usb_serial_jtag_ll_txfifo_writable() &&
		usb_serial_jtag_ll_get_intr_ena_status() & USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
}

static void serial_esp32_usb_irq_rx_enable(const struct device *dev)
{
	ARG_UNUSED(dev);

	usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
	usb_serial_jtag_ll_ena_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
}

static void serial_esp32_usb_irq_rx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	usb_serial_jtag_ll_disable_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
}

static int serial_esp32_usb_irq_tx_complete(const struct device *dev)
{
	ARG_UNUSED(dev);

	return usb_serial_jtag_ll_txfifo_writable();
}

static int serial_esp32_usb_irq_rx_ready(const struct device *dev)
{
	ARG_UNUSED(dev);

	return usb_serial_jtag_ll_rxfifo_data_available();
}

static void serial_esp32_usb_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void serial_esp32_usb_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int serial_esp32_usb_irq_is_pending(const struct device *dev)
{
	return serial_esp32_usb_irq_rx_ready(dev) || serial_esp32_usb_irq_tx_ready(dev);
}

static int serial_esp32_usb_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_OUT_RECV_PKT);
	usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);

	return 1;
}

static void serial_esp32_usb_irq_callback_set(const struct device *dev,
					      uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct serial_esp32_usb_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void serial_esp32_usb_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct serial_esp32_usb_data *data = dev->data;
	uint32_t uart_intr_status = usb_serial_jtag_ll_get_intsts_mask();

	if (uart_intr_status == 0) {
		return;
	}
	usb_serial_jtag_ll_clr_intsts_mask(uart_intr_status);

	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const DRAM_ATTR struct uart_driver_api serial_esp32_usb_api = {
	.poll_in = serial_esp32_usb_poll_in,
	.poll_out = serial_esp32_usb_poll_out,
	.err_check = serial_esp32_usb_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = serial_esp32_usb_configure,
	.config_get = serial_esp32_usb_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = serial_esp32_usb_fifo_fill,
	.fifo_read = serial_esp32_usb_fifo_read,
	.irq_tx_enable = serial_esp32_usb_irq_tx_enable,
	.irq_tx_disable = serial_esp32_usb_irq_tx_disable,
	.irq_tx_ready = serial_esp32_usb_irq_tx_ready,
	.irq_rx_enable = serial_esp32_usb_irq_rx_enable,
	.irq_rx_disable = serial_esp32_usb_irq_rx_disable,
	.irq_tx_complete = serial_esp32_usb_irq_tx_complete,
	.irq_rx_ready = serial_esp32_usb_irq_rx_ready,
	.irq_err_enable = serial_esp32_usb_irq_err_enable,
	.irq_err_disable = serial_esp32_usb_irq_err_disable,
	.irq_is_pending = serial_esp32_usb_irq_is_pending,
	.irq_update = serial_esp32_usb_irq_update,
	.irq_callback_set = serial_esp32_usb_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define ESP32_UART_INIT(idx)                                                                       \
	static const DRAM_ATTR struct serial_esp32_usb_config serial_esp32_usb_cfg_port_##idx = {  \
		.uart_num = DT_INST_PROP(idx, peripheral),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),          \
		.irq_source = DT_INST_IRQN(idx)                                                    \
	};                                                                                         \
												   \
	static struct serial_esp32_usb_data serial_esp32_usb_data_##idx = {                        \
		.serial_config = { .baudrate = DT_INST_PROP(idx, current_speed),                   \
				   .parity = UART_CFG_PARITY_NONE,                                 \
				   .stop_bits = UART_CFG_STOP_BITS_1,                              \
				   .data_bits = UART_CFG_DATA_BITS_8,                              \
				   .flow_ctrl =                                                    \
					   COND_CODE_1(DT_NODE_HAS_PROP(idx, hw_flow_control),     \
						       (UART_CFG_FLOW_CTRL_RTS_CTS),               \
						       (UART_CFG_FLOW_CTRL_NONE)) },               \
	};                                                                                         \
												   \
	DEVICE_DT_INST_DEFINE(idx, &serial_esp32_usb_init, NULL, &serial_esp32_usb_data_##idx,     \
			      &serial_esp32_usb_cfg_port_##idx, PRE_KERNEL_1,                      \
			      CONFIG_SERIAL_INIT_PRIORITY, &serial_esp32_usb_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_UART_INIT);
