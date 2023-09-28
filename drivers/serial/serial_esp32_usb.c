/*
 * Copyright (c) 2022 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_usb_serial

#include <hal/usb_serial_jtag_ll.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#if defined(CONFIG_SOC_SERIES_ESP32C3)
#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#else
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <esp_attr.h>

#ifdef CONFIG_SOC_SERIES_ESP32C3
#define ISR_HANDLER isr_handler_t
#else
#define ISR_HANDLER intr_handler_t
#endif

/*
 * Timeout after which the poll_out function stops waiting for space in the tx fifo.
 *
 * Without this timeout, the function would get stuck forever and block the processor if no host is
 * connected to the USB port.
 *
 * USB full-speed uses a frame rate of 1 ms. Thus, a timeout of 50 ms provides plenty of safety
 * margin even for a loaded bus. This is the same value as used in the ESP-IDF.
 */
#define USBSERIAL_POLL_OUT_TIMEOUT_MS (50U)

struct serial_esp32_usb_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
};

struct serial_esp32_usb_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
	int irq_line;
	int64_t last_tx_time;
};

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
	struct serial_esp32_usb_data *data = dev->data;

	/*
	 * If there is no USB host connected, this function will busy-wait once for the timeout
	 * period, but return immediately for subsequent calls.
	 */
	do {
		if (usb_serial_jtag_ll_txfifo_writable()) {
			usb_serial_jtag_ll_write_txfifo(&c, 1);
			usb_serial_jtag_ll_txfifo_flush();
			data->last_tx_time = k_uptime_get();
			return;
		}
	} while ((k_uptime_get() - data->last_tx_time) < USBSERIAL_POLL_OUT_TIMEOUT_MS);
}

static int serial_esp32_usb_err_check(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int serial_esp32_usb_init(const struct device *dev)
{
	const struct serial_esp32_usb_config *config = dev->config;
	struct serial_esp32_usb_data *data = dev->data;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	int ret = clock_control_on(config->clock_dev, config->clock_subsys);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
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
	struct serial_esp32_usb_data *data = dev->data;

	usb_serial_jtag_ll_clr_intsts_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);
	usb_serial_jtag_ll_ena_intr_mask(USB_SERIAL_JTAG_INTR_SERIAL_IN_EMPTY);

	if (data->irq_cb != NULL) {
		data->irq_cb(dev, data->irq_cb_data);
	}
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

static const DRAM_ATTR struct serial_esp32_usb_config serial_esp32_usb_cfg = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.irq_source = DT_INST_IRQN(0)
};

static struct serial_esp32_usb_data serial_esp32_usb_data_0;

DEVICE_DT_INST_DEFINE(0, &serial_esp32_usb_init, NULL, &serial_esp32_usb_data_0,
		      &serial_esp32_usb_cfg, PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY, &serial_esp32_usb_api);
