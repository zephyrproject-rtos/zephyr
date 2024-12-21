/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si32_usart

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#include <SI32_CLKCTRL_A_Type.h>
#include <SI32_USART_A_Type.h>
#include <si32_device.h>

struct usart_si32_config {
	SI32_USART_A_Type *usart;
	bool hw_flow_control;
	uint8_t parity;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_config_func_t irq_config_func;
#endif
	const struct device *clock_dev;
};

struct usart_si32_data {
	uint32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static int usart_si32_poll_in(const struct device *dev, unsigned char *c)
{
	const struct usart_si32_config *config = dev->config;
	int ret = -1;

	if (SI32_USART_A_read_rx_fifo_count(config->usart) != 0) {
		*c = SI32_USART_A_read_data_u8(config->usart);
		ret = 0;
	}

	return ret;
}

static void usart_si32_poll_out(const struct device *dev, unsigned char c)
{
	const struct usart_si32_config *config = dev->config;

	while (SI32_USART_A_read_tx_fifo_count(config->usart) ||
	       SI32_USART_A_is_tx_busy(config->usart)) {
		/* busy wait */
	}

	SI32_USART_A_write_data_u8(config->usart, c);
}

static int usart_si32_err_check(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;
	int ret = 0;

	if (SI32_USART_A_is_tx_fifo_error_interrupt_pending(config->usart)) {
		SI32_USART_A_clear_tx_fifo_error_interrupt(config->usart);
	}

	if (SI32_USART_A_is_rx_overrun_interrupt_pending(config->usart)) {
		SI32_USART_A_clear_rx_overrun_error_interrupt(config->usart);
		ret |= UART_ERROR_OVERRUN;
	}

	if (SI32_USART_A_is_rx_parity_error_interrupt_pending(config->usart)) {
		SI32_USART_A_clear_rx_parity_error_interrupt(config->usart);
		ret |= UART_ERROR_PARITY;
	}

	if (SI32_USART_A_is_rx_frame_error_interrupt_pending(config->usart)) {
		SI32_USART_A_clear_rx_frame_error_interrupt(config->usart);
		ret |= UART_ERROR_FRAMING;
	}

	return ret;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int usart_si32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct usart_si32_config *config = dev->config;
	int i;

	/* NOTE: Checking `SI32_USART_A_is_tx_busy` is a workaround.
	 *       For some reason data gets corrupted when writing to the FIFO
	 *       while a write is happening.
	 */
	for (i = 0; i < size && SI32_USART_A_read_tx_fifo_count(config->usart) == 0 &&
		    !SI32_USART_A_is_tx_busy(config->usart);
	     i++) {
		SI32_USART_A_write_data_u8(config->usart, tx_data[i]);
	}

	return i;
}

static int usart_si32_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct usart_si32_config *config = dev->config;
	int i;

	for (i = 0; i < size; i++) {
		if (!SI32_USART_A_read_rx_fifo_count(config->usart)) {
			break;
		}

		rx_data[i] = SI32_USART_A_read_data_u8(config->usart);
	}

	return i;
}

static void usart_si32_irq_tx_enable(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	SI32_USART_A_enable_tx_data_request_interrupt(config->usart);
}

static void usart_si32_irq_tx_disable(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	SI32_USART_A_disable_tx_data_request_interrupt(config->usart);
}

static int usart_si32_irq_tx_ready(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	return SI32_USART_A_is_tx_data_request_interrupt_pending(config->usart);
}

static int usart_si32_irq_tx_complete(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	return SI32_USART_A_is_tx_complete(config->usart);
}

static void usart_si32_irq_rx_enable(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	SI32_USART_A_enable_rx_data_request_interrupt(config->usart);
}

static void usart_si32_irq_rx_disable(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	SI32_USART_A_disable_rx_data_request_interrupt(config->usart);
}

static int usart_si32_irq_rx_ready(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	return SI32_USART_A_is_rx_data_request_interrupt_pending(config->usart);
}

static void usart_si32_irq_err_enable(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	SI32_USART_A_enable_rx_error_interrupts(config->usart);
	SI32_USART_A_enable_tx_error_interrupts(config->usart);
}

static void usart_si32_irq_err_disable(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;

	SI32_USART_A_disable_rx_error_interrupts(config->usart);
	SI32_USART_A_disable_tx_error_interrupts(config->usart);
}

static int usart_si32_irq_is_pending(const struct device *dev)
{
	return usart_si32_irq_rx_ready(dev) || usart_si32_irq_tx_ready(dev);
}

static int usart_si32_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void usart_si32_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct usart_si32_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void usart_si32_irq_handler(const struct device *dev)
{
	struct usart_si32_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}

	usart_si32_err_check(dev);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, usart_si32_driver_api) = {
	.poll_in = usart_si32_poll_in,
	.poll_out = usart_si32_poll_out,
	.err_check = usart_si32_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = usart_si32_fifo_fill,
	.fifo_read = usart_si32_fifo_read,
	.irq_tx_enable = usart_si32_irq_tx_enable,
	.irq_tx_disable = usart_si32_irq_tx_disable,
	.irq_tx_ready = usart_si32_irq_tx_ready,
	.irq_tx_complete = usart_si32_irq_tx_complete,
	.irq_rx_enable = usart_si32_irq_rx_enable,
	.irq_rx_disable = usart_si32_irq_rx_disable,
	.irq_rx_ready = usart_si32_irq_rx_ready,
	.irq_err_enable = usart_si32_irq_err_enable,
	.irq_err_disable = usart_si32_irq_err_disable,
	.irq_is_pending = usart_si32_irq_is_pending,
	.irq_update = usart_si32_irq_update,
	.irq_callback_set = usart_si32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int usart_si32_init(const struct device *dev)
{
	const struct usart_si32_config *config = dev->config;
	struct usart_si32_data *data = dev->data;
	uint32_t apb_freq;
	uint32_t baud_register_value;
	int ret;
	enum SI32_USART_A_PARITY_Enum parity = SI32_USART_A_PARITY_ODD;
	bool parity_enabled;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_get_rate(config->clock_dev, NULL, &apb_freq);
	if (ret) {
		return ret;
	}

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		parity_enabled = false;
		break;
	case UART_CFG_PARITY_ODD:
		parity = SI32_USART_A_PARITY_ODD;
		parity_enabled = true;
		break;
	case UART_CFG_PARITY_EVEN:
		parity = SI32_USART_A_PARITY_EVEN;
		parity_enabled = true;
		break;
	case UART_CFG_PARITY_MARK:
		parity = SI32_USART_A_PARITY_SET;
		parity_enabled = true;
		break;
	case UART_CFG_PARITY_SPACE:
		parity = SI32_USART_A_PARITY_CLEAR;
		parity_enabled = true;
		break;
	default:
		return -ENOTSUP;
	}

	if (config->usart == SI32_USART_0) {
		SI32_CLKCTRL_A_enable_apb_to_modules_0(SI32_CLKCTRL_0,
						       SI32_CLKCTRL_A_APBCLKG0_USART0);
	} else if (config->usart == SI32_USART_1) {
		SI32_CLKCTRL_A_enable_apb_to_modules_0(SI32_CLKCTRL_0,
						       SI32_CLKCTRL_A_APBCLKG0_USART1);
	} else {
		return -ENOTSUP;
	}

	baud_register_value = (apb_freq / (2 * data->baud_rate)) - 1;

	SI32_USART_A_exit_loopback_mode(config->usart);

	if (config->hw_flow_control) {
		SI32_USART_A_enable_rts(config->usart);
		SI32_USART_A_select_rts_deassert_on_byte_free(config->usart);
		SI32_USART_A_disable_rts_inversion(config->usart);

		SI32_USART_A_enable_cts(config->usart);
		SI32_USART_A_disable_cts_inversion(config->usart);
	}

	/* Transmitter */
	if (parity_enabled) {
		SI32_USART_A_select_tx_parity(config->usart, parity);
		SI32_USART_A_enable_tx_parity_bit(config->usart);
	} else {
		SI32_USART_A_disable_tx_parity_bit(config->usart);
	}
	SI32_USART_A_select_tx_data_length(config->usart, SI32_USART_A_DATA_LENGTH_8_BITS);
	SI32_USART_A_enable_tx_start_bit(config->usart);
	SI32_USART_A_enable_tx_stop_bit(config->usart);
	SI32_USART_A_select_tx_stop_bits(config->usart, SI32_USART_A_STOP_BITS_1_BIT);
	SI32_USART_A_set_tx_baudrate(config->usart, (uint16_t)baud_register_value);
	SI32_USART_A_select_tx_asynchronous_mode(config->usart);
	SI32_USART_A_disable_tx_signal_inversion(config->usart);
	SI32_USART_A_select_tx_fifo_threshold_for_request_to_1(config->usart);
	SI32_USART_A_enable_tx(config->usart);

	/* Receiver */
	if (parity_enabled) {
		SI32_USART_A_select_rx_parity(config->usart, parity);
		SI32_USART_A_enable_rx_parity_bit(config->usart);
	} else {
		SI32_USART_A_disable_rx_parity_bit(config->usart);
	}
	SI32_USART_A_select_rx_data_length(config->usart, SI32_USART_A_DATA_LENGTH_8_BITS);
	SI32_USART_A_enable_rx_start_bit(config->usart);
	SI32_USART_A_enable_rx_stop_bit(config->usart);
	SI32_USART_A_select_rx_stop_bits(config->usart, SI32_USART_A_STOP_BITS_1_BIT);
	SI32_USART_A_set_rx_baudrate(config->usart, (uint16_t)baud_register_value);
	SI32_USART_A_select_rx_asynchronous_mode(config->usart);
	SI32_USART_A_disable_rx_signal_inversion(config->usart);
	SI32_USART_A_select_rx_fifo_threshold_1(config->usart);
	SI32_USART_A_enable_rx(config->usart);

	SI32_USART_A_flush_tx_fifo(config->usart);
	SI32_USART_A_flush_rx_fifo(config->usart);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	config->irq_config_func(dev);
#endif

	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define SI32_USART_IRQ_HANDLER_DECL(index)                                                         \
	static void usart_si32_irq_config_func_##index(const struct device *dev);
#define SI32_USART_IRQ_HANDLER(index)                                                              \
	static void usart_si32_irq_config_func_##index(const struct device *dev)                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                     \
			    usart_si32_irq_handler, DEVICE_DT_INST_GET(index), 0);                 \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#else
#define SI32_USART_IRQ_HANDLER_DECL(index) /* Not used */
#define SI32_USART_IRQ_HANDLER(index)      /* Not used */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define SI32_USART_IRQ_HANDLER_FUNC(index) .irq_config_func = usart_si32_irq_config_func_##index,
#else
#define SI32_USART_IRQ_HANDLER_FUNC(index) /* Not used */
#endif

#define SI32_USART_INIT(index)                                                                     \
	SI32_USART_IRQ_HANDLER_DECL(index)                                                         \
                                                                                                   \
	static const struct usart_si32_config usart_si32_cfg_##index = {                           \
		.usart = (SI32_USART_A_Type *)DT_INST_REG_ADDR(index),                             \
		.hw_flow_control = DT_INST_PROP(index, hw_flow_control),                           \
		.parity = DT_INST_ENUM_IDX_OR(index, parity, UART_CFG_PARITY_NONE),                \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		SI32_USART_IRQ_HANDLER_FUNC(index)};                                               \
                                                                                                   \
	static struct usart_si32_data usart_si32_data_##index = {                                  \
		.baud_rate = DT_INST_PROP(index, current_speed),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &usart_si32_init, NULL, &usart_si32_data_##index,             \
			      &usart_si32_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &usart_si32_driver_api);                                             \
                                                                                                   \
	SI32_USART_IRQ_HANDLER(index)

DT_INST_FOREACH_STATUS_OKAY(SI32_USART_INIT)
