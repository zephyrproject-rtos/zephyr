/*
 * Copyright (c) 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_coreuart

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>


/* UART REGISTERS DEFINITIONS */

/* TX register */
#define TXDATA_REG_OFFSET   0x0

#define TXDATA_OFFSET   0x0
#define TXDATA_MASK     0xFF
#define TXDATA_SHIFT    0

/* RX register */
#define RXDATA_REG_OFFSET   0x4

#define RXDATA_OFFSET   0x4
#define RXDATA_MASK     0xFF
#define RXDATA_SHIFT    0

/* Control1 register */
#define CTRL1_REG_OFFSET        0x8

/* Baud value lower 8 bits */
#define CTRL1_BAUDVALUE_OFFSET   0x8
#define CTRL1_BAUDVALUE_MASK     0xFF
#define CTRL1_BAUDVALUE_SHIFT    0

/* Control2 register */
#define CTRL2_REG_OFFSET          0xC

/* Bit length */
#define CTRL2_BIT_LENGTH_OFFSET   0xC
#define CTRL2_BIT_LENGTH_MASK     0x01
#define CTRL2_BIT_LENGTH_SHIFT    0

/* Parity enable */
#define CTRL2_PARITY_EN_OFFSET    0xC
#define CTRL2_PARITY_EN_MASK      0x02
#define CTRL2_PARITY_EN_SHIFT     1

/* Odd/even parity configuration */
#define CTRL2_ODD_EVEN_OFFSET     0xC
#define CTRL2_ODD_EVEN_MASK       0x04
#define CTRL2_ODD_EVEN_SHIFT      2

/* Baud value higher 5 bits */
#define CTRL2_BAUDVALUE_OFFSET    0xC
#define CTRL2_BAUDVALUE_MASK      0xF8
#define CTRL2_BAUDVALUE_SHIFT     3

/* Status register */
#define StatusReg_REG_OFFSET    0x10

#define STATUS_REG_OFFSET       0x10

/* TX ready */
#define STATUS_TXRDY_OFFSET   0x10
#define STATUS_TXRDY_MASK     0x01
#define STATUS_TXRDY_SHIFT    0

/* Receive full - raised even when 1 char arrived */
#define STATUS_RXFULL_OFFSET   0x10
#define STATUS_RXFULL_MASK     0x02
#define STATUS_RXFULL_SHIFT    1

/* Parity error */
#define STATUS_PARITYERR_OFFSET   0x10
#define STATUS_PARITYERR_MASK     0x04
#define STATUS_PARITYERR_SHIFT    2

/* Overflow */
#define STATUS_OVERFLOW_OFFSET   0x10
#define STATUS_OVERFLOW_MASK     0x08
#define STATUS_OVERFLOW_SHIFT    3

/* Frame error */
#define STATUS_FRAMERR_OFFSET   0x10
#define STATUS_FRAMERR_MASK     0x10
#define STATUS_FRAMERR_SHIFT    4

/* Data bits length defines */
#define DATA_7_BITS     0x00
#define DATA_8_BITS     0x01

/* Parity defines */
#define NO_PARITY       0x00
#define EVEN_PARITY     0x02
#define ODD_PARITY      0x06

/* Error Status definitions */
#define UART_PARITY_ERROR    0x01
#define UART_OVERFLOW_ERROR  0x02
#define UART_FRAMING_ERROR   0x04

#define BAUDVALUE_LSB ((uint16_t)(0x00FF))
#define BAUDVALUE_MSB ((uint16_t)(0xFF00))
#define BAUDVALUE_SHIFT ((uint8_t)(5))

#define MIV_UART_0_LINECFG 0x1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static struct k_thread rx_thread;
static K_KERNEL_STACK_DEFINE(rx_stack, 512);
#endif

struct uart_miv_regs_t {
	uint8_t tx;
	uint8_t reserved0[3];
	uint8_t rx;
	uint8_t reserved1[3];
	uint8_t ctrlreg1;
	uint8_t reserved2[3];
	uint8_t ctrlreg2;
	uint8_t reserved3[3];
	uint8_t status;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(const struct device *dev);
#endif

struct uart_miv_device_config {
	uint32_t       uart_addr;
	uint32_t       sys_clk_freq;
	uint32_t       line_config;
	uint32_t       baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t cfg_func;
#endif
};

struct uart_miv_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct device *dev;
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_UART(dev)						\
	((struct uart_miv_regs_t *)				\
	 ((const struct uart_miv_device_config * const)(dev)->config)->uart_addr)

static void uart_miv_poll_out(const struct device *dev,
				       unsigned char c)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	while (!(uart->status & STATUS_TXRDY_MASK)) {
	}

	uart->tx = c;
}

static int uart_miv_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	if (uart->status & STATUS_RXFULL_MASK) {
		*c = (unsigned char)(uart->rx & RXDATA_MASK);
		return 0;
	}

	return -1;
}

static int uart_miv_err_check(const struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	uint32_t flags = uart->status;
	int err = 0;

	if (flags & STATUS_PARITYERR_MASK) {
		err |= UART_PARITY_ERROR;
	}

	if (flags & STATUS_OVERFLOW_MASK) {
		err |= UART_OVERFLOW_ERROR;
	}

	if (flags & STATUS_FRAMERR_MASK) {
		err |= UART_FRAMING_ERROR;
	}

	return err;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_miv_fifo_fill(const struct device *dev,
			      const uint8_t *tx_data,
			      int size)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	int i;

	for (i = 0; i < size && (uart->status & STATUS_TXRDY_MASK); i++) {
		uart->tx = tx_data[i];
	}

	return i;
}

static int uart_miv_fifo_read(const struct device *dev,
			      uint8_t *rx_data,
			      const int size)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	int i;

	for (i = 0; i < size; i++) {
		if (uart->status & STATUS_RXFULL_MASK) {
			rx_data[i] = (unsigned char)(uart->rx & RXDATA_MASK);
		} else {
			break;
		}
	}

	return i;
}

static void uart_miv_irq_tx_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_miv_irq_tx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_miv_irq_tx_ready(const struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	return !(uart->status & STATUS_TXRDY_MASK);
}

static int uart_miv_irq_tx_complete(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_miv_irq_rx_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_miv_irq_rx_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_miv_irq_rx_ready(const struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & STATUS_RXFULL_MASK);
}

static void uart_miv_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_miv_irq_err_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_miv_irq_is_pending(const struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & STATUS_RXFULL_MASK);
}

static int uart_miv_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_miv_irq_handler(const struct device *dev)
{
	struct uart_miv_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}

/*
 * This thread is a workaround for IRQs that are not connected in Mi-V.
 * Since we cannot rely on IRQs, the rx_thread is working instead and
 * polling for data. The thread calls the registered callback when data
 * arrives.
 */
void uart_miv_rx_thread(void *arg1, void *arg2, void *arg3)
{
	struct uart_miv_data *data = (struct uart_miv_data *)arg1;
	const struct device *dev = data->dev;
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	const struct uart_miv_device_config *const cfg = dev->config;
	/* Make it go to sleep for a period no longer than
	 * time to receive next character.
	 */
	uint32_t delay = 1000000 / cfg->baud_rate;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		if (uart->status & STATUS_RXFULL_MASK) {
			uart_miv_irq_handler(dev);
		}
		k_sleep(K_USEC(delay));
	}
}

static void uart_miv_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_miv_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_miv_init(const struct device *dev)
{
	const struct uart_miv_device_config *const cfg = dev->config;
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	/* Calculate divider value to set baudrate */
	uint16_t baud_value = (cfg->sys_clk_freq / (cfg->baud_rate * 16U)) - 1;

	/* Set baud rate */
	uart->ctrlreg1 = (uint8_t)(baud_value & BAUDVALUE_LSB);
	uart->ctrlreg2 = (uint8_t)(cfg->line_config) |
			 (uint8_t)((baud_value & BAUDVALUE_MSB) >> BAUDVALUE_SHIFT);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Setup thread polling for data */
	cfg->cfg_func(dev);
#endif
	return 0;
}

static const struct uart_driver_api uart_miv_driver_api = {
	.poll_in          = uart_miv_poll_in,
	.poll_out         = uart_miv_poll_out,
	.err_check        = uart_miv_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_miv_fifo_fill,
	.fifo_read        = uart_miv_fifo_read,
	.irq_tx_enable    = uart_miv_irq_tx_enable,
	.irq_tx_disable   = uart_miv_irq_tx_disable,
	.irq_tx_ready     = uart_miv_irq_tx_ready,
	.irq_tx_complete  = uart_miv_irq_tx_complete,
	.irq_rx_enable    = uart_miv_irq_rx_enable,
	.irq_rx_disable   = uart_miv_irq_rx_disable,
	.irq_rx_ready     = uart_miv_irq_rx_ready,
	.irq_err_enable   = uart_miv_irq_err_enable,
	.irq_err_disable  = uart_miv_irq_err_disable,
	.irq_is_pending   = uart_miv_irq_is_pending,
	.irq_update       = uart_miv_irq_update,
	.irq_callback_set = uart_miv_irq_callback_set,
#endif
};

/* This driver is single-instance. */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported uart_miv instance");

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static struct uart_miv_data uart_miv_data_0;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_miv_irq_cfg_func_0(const struct device *dev);
#endif

static const struct uart_miv_device_config uart_miv_dev_cfg_0 = {
	.uart_addr    = DT_INST_REG_ADDR(0),
	.sys_clk_freq = DT_INST_PROP(0, clock_frequency),
	.line_config  = MIV_UART_0_LINECFG,
	.baud_rate    = DT_INST_PROP(0, current_speed),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cfg_func     = uart_miv_irq_cfg_func_0,
#endif
};

DEVICE_DT_INST_DEFINE(0, uart_miv_init, NULL,
		    &uart_miv_data_0, &uart_miv_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
		    (void *)&uart_miv_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_miv_irq_cfg_func_0(const struct device *dev)
{
	struct uart_miv_data *data = dev->data;

	data->dev = dev;

	/* Create a thread which will poll for data - replacement for IRQ */
	k_thread_create(&rx_thread, rx_stack, 500,
			uart_miv_rx_thread, data, NULL, NULL, K_PRIO_COOP(2),
			0, K_NO_WAIT);
}
#endif

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay) */
