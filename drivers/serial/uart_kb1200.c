/*
 * Copyright (c) 2021, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real serial driver. It is used to instantiate struct
 * devices for the "vnd,serial" devicetree compatible used in test code.
 */

#define DT_DRV_COMPAT ene_kb1200_uart

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <soc.h>

#define KB1200_GPIO_MUX 0x40000018

#define KB1200_SERCFG  0x40310000
#define KB1200_SERIE   0x40310004
#define KB1200_SERPF   0x40310008
#define KB1200_SERSTS  0x4031000C
#define KB1200_SERRBUF 0x40310010
#define KB1200_SERTBUF 0x40310014
#define KB1200_SERCTRL 0x40310018

#define KB1200_GPIO_G0_FS 0x50000000
#define KB1200_GPIO_G0_OD 0x50000050
#define KB1200_GPIO_G0_OE 0x50000010
#define KB1200_GPIO_G0_PU 0x50000040
#define KB1200_GPIO_G0_IE 0x50000060
#define KB1200_GPIO_G0_IN 0x50000030
#define KB1200_GPIO_G0_D  0x50000020

typedef void (*kb1200_uart_write_cb_t)(const struct device *dev, void *user_data);

struct kb1200_uart_data {
	kb1200_uart_write_cb_t callback;
	void *callback_data;
};

static int kb1200_uart_poll_in(const struct device *dev, unsigned char *c)
{
	if ((*((volatile uint32_t*)KB1200_SERPF)) & 0x0001) {
		unsigned char rx = (unsigned char)(*((volatile uint32_t*)KB1200_SERRBUF));
		(*((volatile uint32_t*)KB1200_SERPF)) = 0x0001;
		*c = rx;
		return 0;
	}
	return -1;
}

static void kb1200_uart_poll_out(const struct device *dev, unsigned char c)
{
#define RETRY_COUNT 1000
	for (volatile int i = 0; i < RETRY_COUNT; i++) {
		if (((*((volatile uint32_t*)KB1200_SERSTS)) & 0x0001) == 0) {  // Tx FIFO FULL
			(*((volatile uint32_t*)KB1200_SERTBUF)) = c;
			break;
		}
	}
	if ((*((volatile uint32_t*)KB1200_SERSTS)) & 0x0002) {  // Tx FIFO overrun
		(*((volatile uint32_t*)KB1200_SERSTS)) = 0x0002;
	}
}

static int kb1200_uart_err_check(const struct device *dev)
{
	return -ENOTSUP;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int kb1200_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	return -ENOTSUP;
}

static int kb1200_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	return -ENOTSUP;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Enable TX interrupt in event register
 *
 * @param dev UART device struct
 */
static void kb1200_uart_irq_tx_enable(const struct device *dev)
{
//	uint8_t enable = litex_read8(UART_EV_ENABLE_ADDR);
//	litex_write8(enable | UART_EV_TX, UART_EV_ENABLE_ADDR);
}

/**
 * @brief Disable TX interrupt in event register
 *
 * @param dev UART device struct
 */
static void kb1200_uart_irq_tx_disable(const struct device *dev)
{
//	uint8_t enable = litex_read8(UART_EV_ENABLE_ADDR);
//	litex_write8(enable & ~(UART_EV_TX), UART_EV_ENABLE_ADDR);
}

/**
 * @brief Enable RX interrupt in event register
 *
 * @param dev UART device struct
 */
static void kb1200_uart_irq_rx_enable(const struct device *dev)
{
//	uint8_t enable = litex_read8(UART_EV_ENABLE_ADDR);
//	litex_write8(enable | UART_EV_RX, UART_EV_ENABLE_ADDR);
}

/**
 * @brief Disable RX interrupt in event register
 *
 * @param dev UART device struct
 */
static void kb1200_uart_irq_rx_disable(const struct device *dev)
{
//	uint8_t enable = litex_read8(UART_EV_ENABLE_ADDR);
//	litex_write8(enable & ~(UART_EV_RX), UART_EV_ENABLE_ADDR);
}

/**
 * @brief Check if Tx IRQ has been raised and UART is ready to accept new data
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int kb1200_uart_irq_tx_ready(const struct device *dev)
{
//	uint8_t val = litex_read8(UART_TXFULL_ADDR);
//	return !val;
	return 0;
}

/**
 * @brief Check if Rx IRQ has been raised and there's data to be read from UART
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ has been raised, 0 otherwise
 */
static int kb1200_uart_irq_rx_ready(const struct device *dev)
{
//	uint8_t pending;
//	pending = litex_read8(UART_EV_PENDING_ADDR);
//	if (pending & UART_EV_RX) {
//		return 1;
//	} else {
//		return 0;
//	}
	return 0;
}

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int kb1200_uart_fifo_fill(const struct device *dev,
				   const uint8_t *tx_data, int size)
{
//	int i;
//	for (i = 0; i < size && !litex_read8(UART_TXFULL_ADDR); i++) {
//		litex_write8(tx_data[i], UART_RXTX_ADDR);
//	}
//	return i;
	return 0;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int kb1200_uart_fifo_read(const struct device *dev,
				   uint8_t *rx_data, const int size)
{
//	int i;
//	for (i = 0; i < size && !litex_read8(UART_RXEMPTY_ADDR); i++) {
//		rx_data[i] = litex_read8(UART_RXTX_ADDR);
//		/* refresh UART_RXEMPTY by writing UART_EV_RX
//		 * to UART_EV_PENDING
//		 */
//		litex_write8(UART_EV_RX, UART_EV_PENDING_ADDR);
//	}
//	return i;
	return 0;
}

static void kb1200_uart_irq_err(const struct device *dev)
{
	ARG_UNUSED(dev);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int kb1200_uart_irq_is_pending(const struct device *dev)
{
//	uint8_t pending;
//	pending = litex_read8(UART_EV_PENDING_ADDR);
//	if (pending & (UART_EV_TX | UART_EV_RX)) {
//		return 1;
//	} else {
//		return 0;
//	}
	return 0;
}

static int kb1200_uart_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void kb1200_uart_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb,
					   void *cb_data)
{
//	struct kb1200_uart_data *data;
//	data = dev->data;
//	data->callback = cb;
//	data->cb_data = cb_data;
}

static void kb1200_uart_irq_handler(const struct device *dev)
{
//	struct kb1200_uart_data *data = dev->data;
//	unsigned int key = irq_lock();
//	if (data->callback) {
//		data->callback(dev, data->cb_data);
//	}
//	/* clear events */
//	litex_write8(UART_EV_TX | UART_EV_RX, UART_EV_PENDING_ADDR);
//	irq_unlock(key);
}
#endif	/* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api kb1200_uart_api = {
	.poll_in = kb1200_uart_poll_in,
	.poll_out = kb1200_uart_poll_out,
	.err_check = kb1200_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = kb1200_uart_configure,
	.config_get = kb1200_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill		= kb1200_uart_fifo_fill,
	.fifo_read		= kb1200_uart_fifo_read,
	.irq_tx_enable		= kb1200_uart_irq_tx_enable,
	.irq_tx_disable		= kb1200_uart_irq_tx_disable,
	.irq_tx_ready		= kb1200_uart_irq_tx_ready,
	.irq_rx_enable		= kb1200_uart_irq_rx_enable,
	.irq_rx_disable		= kb1200_uart_irq_rx_disable,
	.irq_rx_ready		= kb1200_uart_irq_rx_ready,
	.irq_err_enable		= kb1200_uart_irq_err,
	.irq_err_disable	= kb1200_uart_irq_err,
	.irq_is_pending		= kb1200_uart_irq_is_pending,
	.irq_update		= kb1200_uart_irq_update,
	.irq_callback_set	= kb1200_uart_irq_callback_set
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};


static int kb1200_uart_init(const struct device *dev)
{
	PINMUX_DEV_T ser_tx_pinmux_dev = gpio_pinmux(SER_TX_GPIO_Num);
	PINMUX_DEV_T ser_rx_pinmux_dev = gpio_pinmux(SER_RX_GPIO_Num);
	gpio_pinmux_set(ser_tx_pinmux_dev.port, ser_tx_pinmux_dev.pin, PINMUX_FUNC_B);
	gpio_pinmux_set(ser_rx_pinmux_dev.port, ser_rx_pinmux_dev.pin, PINMUX_FUNC_B);

	(*((volatile uint32_t*)KB1200_GPIO_MUX)) &= ~0x00000600; // bit[10~9] = 0 select serial port
	(*((volatile uint32_t*)KB1200_SERCTRL)) = 0x00000001;   // Mode-1:8-bit serial port
	(*((volatile uint32_t*)KB1200_SERCFG)) = ((24000000UL/115200)<<16) | 0x0002;  // 115200 bps

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_IRQ 19
	IRQ_CONNECT(UART_IRQ, DT_INST_IRQ(0, priority),
			kb1200_uart_irq_handler, DEVICE_DT_INST_GET(0),
			0);
	irq_enable(UART_IRQ);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#define KB1200_UART_DATA_BUFFER(n)                                                                  \
	RING_BUF_DECLARE(written_data_##n, DT_INST_PROP(n, buffer_size));                          \
	RING_BUF_DECLARE(read_queue_##n, DT_INST_PROP(n, buffer_size));                            \
	static struct kb1200_uart_data kb1200_uart_data_##n = {                                      \
		.written = &written_data_##n,                                                      \
		.read_queue = &read_queue_##n,                                                     \
	};
#define KB1200_UART_DATA(n) static struct kb1200_uart_data kb1200_uart_data_##n = {};
#define KB1200_UART_INIT(n)                                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, buffer_size), (KB1200_UART_DATA_BUFFER(n)),            \
		    (KB1200_UART_DATA(n)))                                                          \
	DEVICE_DT_INST_DEFINE(n, &kb1200_uart_init, NULL, &kb1200_uart_data_##n, NULL, PRE_KERNEL_1,  \
			      CONFIG_SERIAL_INIT_PRIORITY, &kb1200_uart_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_UART_INIT)
