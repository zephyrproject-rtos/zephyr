/*
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	infineon_xmc4xxx_uart

#include <xmc_uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

struct uart_xmc4xxx_config {
	XMC_USIC_CH_t *uart;
	const struct pinctrl_dev_config *pcfg;
	uint8_t input_src;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_config_func_t irq_config_func;
	uint8_t irq_num;
#endif
	uint8_t fifo_start_offset;
	uint8_t fifo_tx_size;
	uint8_t fifo_rx_size;
};

struct uart_xmc4xxx_data {
	XMC_UART_CH_CONFIG_t config;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
	uint8_t service_request;
#endif
};

static int uart_xmc4xxx_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	bool fifo_empty;

	if (config->fifo_rx_size > 0) {
		fifo_empty = XMC_USIC_CH_RXFIFO_IsEmpty(config->uart);
	} else {
		fifo_empty = !XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
	}
	if (fifo_empty) {
		return -1;
	}

	*c = (unsigned char)XMC_UART_CH_GetReceivedData(config->uart);

	return 0;
}

static void uart_xmc4xxx_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	/* XMC_UART_CH_Transmit() only blocks for UART to finish transmitting */
	/* when fifo is not used */
	while (config->fifo_tx_size > 0 && XMC_USIC_CH_TXFIFO_IsFull(config->uart)) {
	}
	XMC_UART_CH_Transmit(config->uart, c);
}

#define MAX_FIFO_SIZE 64
static int uart_xmc4xxx_init(const struct device *dev)
{
	int ret;
	const struct uart_xmc4xxx_config *config = dev->config;
	struct uart_xmc4xxx_data *data = dev->data;
	uint8_t fifo_offset = config->fifo_start_offset;

	data->config.data_bits = 8U;
	data->config.stop_bits = 1U;

	XMC_UART_CH_Init(config->uart, &(data->config));

	if (config->fifo_tx_size > 0) {
		/* fifos need to be aligned on fifo size */
		fifo_offset = ROUND_UP(fifo_offset, BIT(config->fifo_tx_size));
		XMC_USIC_CH_TXFIFO_Configure(config->uart, fifo_offset, config->fifo_tx_size, 1);
		fifo_offset += BIT(config->fifo_tx_size);
	}

	if (config->fifo_rx_size > 0) {
		/* fifos need to be aligned on fifo size */
		fifo_offset = ROUND_UP(fifo_offset, BIT(config->fifo_rx_size));
		XMC_USIC_CH_RXFIFO_Configure(config->uart, fifo_offset, config->fifo_rx_size, 0);
		fifo_offset += BIT(config->fifo_rx_size);
	}

	if (fifo_offset > MAX_FIFO_SIZE) {
		return -EINVAL;
	}

	/* Connect UART RX to logical 1. It is connected to proper pin after pinctrl is applied */
	XMC_UART_CH_SetInputSource(config->uart, XMC_UART_CH_INPUT_RXD, 0x7);

	/* Start the UART before pinctrl, because the USIC is driving the TX line */
	/* low in off state */
	XMC_UART_CH_Start(config->uart);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	/* Connect UART RX to the target pin */
	XMC_UART_CH_SetInputSource(config->uart, XMC_UART_CH_INPUT_RXD,
				   config->input_src);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	config->irq_config_func(dev);
#endif

	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)

static void uart_xmc4xxx_isr(void *arg)
{
	const struct device *dev = arg;
	struct uart_xmc4xxx_data *data = dev->data;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}

static int uart_xmc4xxx_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	int i = 0;

	for (i = 0; i < len; i++) {
		bool fifo_full;

		XMC_UART_CH_Transmit(config->uart, tx_data[i]);
		if (config->fifo_tx_size == 0) {
			return 1;
		}

		fifo_full = XMC_USIC_CH_TXFIFO_IsFull(config->uart);
		if (fifo_full) {
			return i + 1;
		}
	}
	return i;
}

static int uart_xmc4xxx_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	int i;

	for (i = 0; i < size; i++) {
		bool fifo_empty;

		if (config->fifo_rx_size > 0) {
			fifo_empty = XMC_USIC_CH_RXFIFO_IsEmpty(config->uart);
		} else {
			fifo_empty = !XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
		}
		if (fifo_empty) {
			break;
		}
		rx_data[i] = XMC_UART_CH_GetReceivedData(config->uart);
	}
	return i;
}

static void uart_xmc4xxx_irq_tx_enable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	const struct uart_xmc4xxx_data *data = dev->data;

	if (config->fifo_tx_size > 0) {
		/* wait till the fifo has at least 1 byte free */
		while (XMC_USIC_CH_TXFIFO_IsFull(config->uart)) {
		}
		XMC_USIC_CH_TXFIFO_EnableEvent(config->uart,
					       XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	} else {
		XMC_USIC_CH_EnableEvent(config->uart, XMC_USIC_CH_EVENT_TRANSMIT_BUFFER);
	}
	XMC_USIC_CH_TriggerServiceRequest(config->uart, data->service_request);
}

static void uart_xmc4xxx_irq_tx_disable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_tx_size > 0) {
		XMC_USIC_CH_TXFIFO_DisableEvent(config->uart,
						XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	} else {
		XMC_USIC_CH_DisableEvent(config->uart, XMC_USIC_CH_EVENT_TRANSMIT_BUFFER);
	}
}

static int uart_xmc4xxx_irq_tx_ready(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_tx_size > 0) {
		return !XMC_USIC_CH_TXFIFO_IsFull(config->uart);
	} else {
		return XMC_USIC_CH_GetTransmitBufferStatus(config->uart) ==
			XMC_USIC_CH_TBUF_STATUS_IDLE;
	}
}

static void uart_xmc4xxx_irq_rx_enable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	uint32_t recv_status;

	if (config->fifo_rx_size > 0) {
		XMC_USIC_CH_RXFIFO_Flush(config->uart);
		XMC_USIC_CH_RXFIFO_SetSizeTriggerLimit(config->uart, config->fifo_rx_size, 0);
#if CONFIG_UART_XMC4XXX_RX_FIFO_INT_TRIGGER
		config->uart->RBCTR |= BIT(USIC_CH_RBCTR_SRBTEN_Pos);
#endif
		XMC_USIC_CH_RXFIFO_EnableEvent(config->uart,
					       XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD |
					       XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE);
	} else {
		/* flush out any received bytes while the uart rx irq was disabled */
		recv_status = XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
		if (recv_status & USIC_CH_RBUFSR_RDV0_Msk) {
			XMC_UART_CH_GetReceivedData(config->uart);
		}
		if (recv_status & USIC_CH_RBUFSR_RDV1_Msk) {
			XMC_UART_CH_GetReceivedData(config->uart);
		}

		XMC_USIC_CH_EnableEvent(config->uart, XMC_USIC_CH_EVENT_STANDARD_RECEIVE |
						      XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);
	}
}

static void uart_xmc4xxx_irq_rx_disable(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_rx_size > 0) {
		XMC_USIC_CH_RXFIFO_DisableEvent(config->uart,
						XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD |
						XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE);
	} else {
		XMC_USIC_CH_DisableEvent(config->uart, XMC_USIC_CH_EVENT_STANDARD_RECEIVE |
						       XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);
	}
}

static int uart_xmc4xxx_irq_rx_ready(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;

	if (config->fifo_rx_size > 0) {
		return !XMC_USIC_CH_RXFIFO_IsEmpty(config->uart);
	} else {
		return XMC_USIC_CH_GetReceiveBufferStatus(config->uart);
	}
}

static void uart_xmc4xxx_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb, void *user_data)
{
	struct uart_xmc4xxx_data *data = dev->data;

	data->user_cb = cb;
	data->user_data = user_data;
}

#define NVIC_ISPR_BASE 0xe000e200u
static int uart_xmc4xxx_irq_is_pending(const struct device *dev)
{
	const struct uart_xmc4xxx_config *config = dev->config;
	uint32_t irq_num = config->irq_num;
	uint32_t setpend;

	/* the NVIC_ISPR_BASE address stores info which interrupts are pending */
	/* bit 0 -> irq 0, bit 1 -> irq 1,...  */
	setpend = *((uint32_t *)(NVIC_ISPR_BASE) + irq_num / 32);
	irq_num = irq_num & 0x1f; /* take modulo 32 */
	return (setpend & BIT(irq_num)) > 0;
}
#endif

static const struct uart_driver_api uart_xmc4xxx_driver_api = {
	.poll_in = uart_xmc4xxx_poll_in,
	.poll_out = uart_xmc4xxx_poll_out,
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	.fifo_fill = uart_xmc4xxx_fifo_fill,
	.fifo_read = uart_xmc4xxx_fifo_read,
	.irq_tx_enable = uart_xmc4xxx_irq_tx_enable,
	.irq_tx_disable = uart_xmc4xxx_irq_tx_disable,
	.irq_tx_ready = uart_xmc4xxx_irq_tx_ready,
	.irq_rx_enable = uart_xmc4xxx_irq_rx_enable,
	.irq_rx_disable = uart_xmc4xxx_irq_rx_disable,
	.irq_rx_ready = uart_xmc4xxx_irq_rx_ready,
	.irq_callback_set = uart_xmc4xxx_irq_callback_set,
	.irq_is_pending = uart_xmc4xxx_irq_is_pending,
#endif
};

#define USIC_IRQ_MIN  84
#define USIC_IRQ_MAX  101
#define IRQS_PER_USIC 6

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define XMC4XXX_IRQ_HANDLER(index)                                                         \
static void uart_xmc4xxx_irq_setup_##index(const struct device *dev)                       \
{                                                                                          \
	const struct uart_xmc4xxx_config *config = dev->config;                            \
	struct uart_xmc4xxx_data *data = dev->data;                                        \
											   \
	__ASSERT(config->irq_num >= USIC_IRQ_MIN && config->irq_num <= USIC_IRQ_MAX,       \
		 "Invalid irq number\n");                                                  \
											   \
	data->service_request = (config->irq_num - USIC_IRQ_MIN) % IRQS_PER_USIC;          \
											   \
	if (config->fifo_tx_size > 0) {                                                    \
		XMC_USIC_CH_TXFIFO_SetInterruptNodePointer(                                \
			config->uart, XMC_USIC_CH_TXFIFO_INTERRUPT_NODE_POINTER_STANDARD,  \
			data->service_request);                                            \
	} else {                                                                           \
		XMC_USIC_CH_SetInterruptNodePointer(                                       \
			config->uart, XMC_USIC_CH_INTERRUPT_NODE_POINTER_TRANSMIT_BUFFER,  \
			data->service_request);                                            \
	}                                                                                  \
                                                                                           \
	if (config->fifo_rx_size > 0) {                                                    \
		XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(                                \
			config->uart, XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_STANDARD,  \
			data->service_request);                                            \
		XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(                                \
			config->uart, XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_ALTERNATE, \
			data->service_request);                                            \
	} else {                                                                           \
		XMC_USIC_CH_SetInterruptNodePointer(                                       \
			config->uart, XMC_USIC_CH_INTERRUPT_NODE_POINTER_RECEIVE,          \
			data->service_request);                                            \
											   \
		XMC_USIC_CH_SetInterruptNodePointer(                                       \
			config->uart, XMC_USIC_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE,\
			data->service_request);                                            \
	}                                                                                  \
											   \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, tx_rx, irq),                                \
		    DT_INST_IRQ_BY_NAME(index, tx_rx, priority), uart_xmc4xxx_isr,         \
		    DEVICE_DT_INST_GET(index), 0);                                         \
	irq_enable(DT_INST_IRQ_BY_NAME(index, tx_rx, irq));                                \
}

#define XMC4XXX_IRQ_STRUCT_INIT(index)                                                             \
	.irq_config_func = uart_xmc4xxx_irq_setup_##index,                                         \
	.irq_num = DT_INST_IRQ_BY_NAME(index, tx_rx, irq),

#else
#define XMC4XXX_IRQ_HANDLER(index)
#define XMC4XXX_IRQ_STRUCT_INIT(index)
#endif

#define XMC4XXX_INIT(index)						\
PINCTRL_DT_INST_DEFINE(index);						\
XMC4XXX_IRQ_HANDLER(index)						\
static struct uart_xmc4xxx_data xmc4xxx_data_##index = {		\
	.config.baudrate = DT_INST_PROP(index, current_speed)		\
};									\
									\
static const struct uart_xmc4xxx_config xmc4xxx_config_##index = {	\
	.uart = (XMC_USIC_CH_t *)DT_INST_REG_ADDR(index),		\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),			\
	.input_src = DT_INST_ENUM_IDX(index, input_src),		\
XMC4XXX_IRQ_STRUCT_INIT(index)						\
	.fifo_start_offset = DT_INST_PROP(index, fifo_start_offset),    \
	.fifo_tx_size = DT_INST_ENUM_IDX(index, fifo_tx_size),          \
	.fifo_rx_size = DT_INST_ENUM_IDX(index, fifo_rx_size),          \
};									\
									\
	DEVICE_DT_INST_DEFINE(index, &uart_xmc4xxx_init,		\
			    NULL,					\
			    &xmc4xxx_data_##index,			\
			    &xmc4xxx_config_##index, PRE_KERNEL_1,	\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &uart_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_INIT)
