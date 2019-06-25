/*
 * Copyright (c) 2018 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <drivers/uart.h>


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

#define BAUDVALUE_LSB ((u16_t)(0x00FF))
#define BAUDVALUE_MSB ((u16_t)(0xFF00))
#define BAUDVALUE_SHIFT ((u8_t)(5))

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static struct k_thread rx_thread;
static K_THREAD_STACK_DEFINE(rx_stack, 512);
#endif

struct uart_miv_regs_t {
	u8_t tx;
	u8_t reserved0[3];
	u8_t rx;
	u8_t reserved1[3];
	u8_t ctrlreg1;
	u8_t reserved2[3];
	u8_t ctrlreg2;
	u8_t reserved3[3];
	u8_t status;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(struct device *dev);
#endif

struct uart_miv_device_config {
	u32_t       uart_addr;
	u32_t       sys_clk_freq;
	u32_t       line_config;
	u32_t       baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t cfg_func;
#endif
};

struct uart_miv_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_CFG(dev)						\
	((const struct uart_miv_device_config * const)		\
	 (dev)->config->config_info)
#define DEV_UART(dev)						\
	((struct uart_miv_regs_t *)(DEV_CFG(dev))->uart_addr)
#define DEV_DATA(dev)						\
	((struct uart_miv_data * const)(dev)->driver_data)

static void uart_miv_poll_out(struct device *dev,
				       unsigned char c)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	while (!(uart->status & STATUS_TXRDY_MASK)) {
	}

	uart->tx = c;
}

static int uart_miv_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	if (uart->status & STATUS_RXFULL_MASK) {
		*c = (unsigned char)(uart->rx & RXDATA_MASK);
		return 0;
	}

	return -1;
}

static int uart_miv_err_check(struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	u32_t flags = uart->status;
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

static int uart_miv_fifo_fill(struct device *dev,
			      const u8_t *tx_data,
			      int size)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	int i;

	for (i = 0; i < size && (uart->status & STATUS_TXRDY_MASK); i++) {
		uart->tx = tx_data[i];
	}

	return i;
}

static int uart_miv_fifo_read(struct device *dev,
			      u8_t *rx_data,
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

static void uart_miv_irq_tx_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_miv_irq_tx_disable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_miv_irq_tx_ready(struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	return !(uart->status & STATUS_TXRDY_MASK);
}

static int uart_miv_irq_tx_complete(struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_miv_irq_rx_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_miv_irq_rx_disable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_miv_irq_rx_ready(struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & STATUS_RXFULL_MASK);
}

static void uart_miv_irq_err_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_miv_irq_err_disable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static int uart_miv_irq_is_pending(struct device *dev)
{
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);

	return !!(uart->status & STATUS_RXFULL_MASK);
}

static int uart_miv_irq_update(struct device *dev)
{
	return 1;
}

static void uart_miv_irq_handler(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct uart_miv_data *data = DEV_DATA(dev);

	if (data->callback) {
		data->callback(data->cb_data);
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
	struct device *dev = (struct device *)arg1;
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	const struct uart_miv_device_config *const cfg = DEV_CFG(dev);
	/* Make it go to sleep for a period no longer than
	 * time to receive next character.
	 */
	u32_t delay = 1000000 / cfg->baud_rate;

	while (1) {
		if (uart->status & STATUS_RXFULL_MASK) {
			uart_miv_irq_handler(dev);
		}
		k_sleep(delay);
	}
}

static void uart_miv_irq_callback_set(struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_miv_data *data = DEV_DATA(dev);

	data->callback = cb;
	data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_miv_init(struct device *dev)
{
	const struct uart_miv_device_config *const cfg = DEV_CFG(dev);
	volatile struct uart_miv_regs_t *uart = DEV_UART(dev);
	/* Calculate divider value to set baudrate */
	u16_t baud_value = (cfg->sys_clk_freq / (cfg->baud_rate * 16U)) - 1;

	/* Set baud rate */
	uart->ctrlreg1 = (u8_t)(baud_value & BAUDVALUE_LSB);
	uart->ctrlreg2 = (u8_t)(cfg->line_config) |
			 (u8_t)((baud_value & BAUDVALUE_MSB) >> BAUDVALUE_SHIFT);

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

#ifdef CONFIG_UART_MIV_PORT_0

static struct uart_miv_data uart_miv_data_0;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_miv_irq_cfg_func_0(struct device *dev);
#endif

static const struct uart_miv_device_config uart_miv_dev_cfg_0 = {
	.uart_addr    = DT_MIV_UART_0_BASE_ADDR,
	.sys_clk_freq = DT_MIV_UART_0_CLOCK_FREQUENCY,
	.line_config  = MIV_UART_0_LINECFG,
	.baud_rate    = DT_MIV_UART_0_BAUD_RATE,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cfg_func     = uart_miv_irq_cfg_func_0,
#endif
};

DEVICE_AND_API_INIT(uart_miv_0, DT_MIV_UART_0_NAME,
		    uart_miv_init, &uart_miv_data_0, &uart_miv_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_miv_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_miv_irq_cfg_func_0(struct device *dev)
{
	/* Create a thread which will poll for data - replacement for IRQ */
	k_thread_create(&rx_thread, rx_stack, 500,
			uart_miv_rx_thread, dev, NULL, NULL, K_PRIO_COOP(2),
			0, K_NO_WAIT);
}
#endif

#endif /* CONFIG_UART_MIV_PORT_0 */
