/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for the SiFive Freedom Processor
 */

#define DT_DRV_COMPAT sifive_uart0

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <zephyr/irq.h>

#define RXDATA_EMPTY   (1 << 31)   /* Receive FIFO Empty */
#define RXDATA_MASK    0xFF        /* Receive Data Mask */

#define TXDATA_FULL    (1 << 31)   /* Transmit FIFO Full */

#define TXCTRL_TXEN    (1 << 0)    /* Activate Tx Channel */

#define RXCTRL_RXEN    (1 << 0)    /* Activate Rx Channel */

#define IE_TXWM        (1 << 0)    /* TX Interrupt Enable/Pending */
#define IE_RXWM        (1 << 1)    /* RX Interrupt Enable/Pending */

/*
 * RX/TX Threshold count to generate TX/RX Interrupts.
 * Used by txctrl and rxctrl registers
 */
#define CTRL_CNT(x)    (((x) & 0x07) << 16)

struct uart_sifive_regs_t {
	uint32_t tx;
	uint32_t rx;
	uint32_t txctrl;
	uint32_t rxctrl;
	uint32_t ie;
	uint32_t ip;
	uint32_t div;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_sifive_device_config {
	uintptr_t	port;
	uint32_t	sys_clk_freq;
	uint32_t	baud_rate;
	uint32_t	rxcnt_irq;
	uint32_t	txcnt_irq;
	const struct	pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t	cfg_func;
#endif
};

struct uart_sifive_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define DEV_UART(dev)						\
	((struct uart_sifive_regs_t *)				\
	 ((const struct uart_sifive_device_config * const)(dev)->config)->port)

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register if transmitter is not full.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_sifive_poll_out(const struct device *dev,
					 unsigned char c)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	/* Wait while TX FIFO is full */
	while (uart->tx & TXDATA_FULL) {
	}

	uart->tx = (int)c;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_sifive_poll_in(const struct device *dev, unsigned char *c)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);
	uint32_t val = uart->rx;

	if (val & RXDATA_EMPTY) {
		return -1;
	}

	*c = (unsigned char)(val & RXDATA_MASK);

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_sifive_fifo_fill(const struct device *dev,
				 const uint8_t *tx_data,
				 int size)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);
	int i;

	for (i = 0; i < size && !(uart->tx & TXDATA_FULL); i++)
		uart->tx = (int)tx_data[i];

	return i;
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
static int uart_sifive_fifo_read(const struct device *dev,
				 uint8_t *rx_data,
				 const int size)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);
	int i;
	uint32_t val;

	for (i = 0; i < size; i++) {
		val = uart->rx;

		if (val & RXDATA_EMPTY)
			break;

		rx_data[i] = (uint8_t)(val & RXDATA_MASK);
	}

	return i;
}

/**
 * @brief Enable TX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_tx_enable(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	uart->ie |= IE_TXWM;
}

/**
 * @brief Disable TX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_tx_disable(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	uart->ie &= ~IE_TXWM;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_sifive_irq_tx_ready(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & IE_TXWM);
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_sifive_irq_tx_complete(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	/*
	 * No TX EMPTY flag for this controller,
	 * just check if TX FIFO is not full
	 */
	return !(uart->tx & TXDATA_FULL);
}

/**
 * @brief Enable RX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_rx_enable(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	uart->ie |= IE_RXWM;
}

/**
 * @brief Disable RX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_rx_disable(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	uart->ie &= ~IE_RXWM;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_sifive_irq_rx_ready(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & IE_RXWM);
}

/* No error interrupt for this controller */
static void uart_sifive_irq_err_enable(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void uart_sifive_irq_err_disable(const struct device *dev)
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
static int uart_sifive_irq_is_pending(const struct device *dev)
{
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);

	return !!(uart->ip & (IE_RXWM | IE_TXWM));
}

static int uart_sifive_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_sifive_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_sifive_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_sifive_irq_handler(const struct device *dev)
{
	struct uart_sifive_data *data = dev->data;

	if (data->callback)
		data->callback(dev, data->cb_data);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static int uart_sifive_init(const struct device *dev)
{
	const struct uart_sifive_device_config * const cfg = dev->config;
	volatile struct uart_sifive_regs_t *uart = DEV_UART(dev);
#ifdef CONFIG_PINCTRL
	int ret;
#endif

	/* Enable TX and RX channels */
	uart->txctrl = TXCTRL_TXEN | CTRL_CNT(cfg->txcnt_irq);
	uart->rxctrl = RXCTRL_RXEN | CTRL_CNT(cfg->rxcnt_irq);

	/* Set baud rate */
	uart->div = cfg->sys_clk_freq / cfg->baud_rate - 1;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Ensure that uart IRQ is disabled initially */
	uart->ie = 0U;

	/* Setup IRQ handler */
	cfg->cfg_func();
#endif

#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

static const struct uart_driver_api uart_sifive_driver_api = {
	.poll_in          = uart_sifive_poll_in,
	.poll_out         = uart_sifive_poll_out,
	.err_check        = NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_sifive_fifo_fill,
	.fifo_read        = uart_sifive_fifo_read,
	.irq_tx_enable    = uart_sifive_irq_tx_enable,
	.irq_tx_disable   = uart_sifive_irq_tx_disable,
	.irq_tx_ready     = uart_sifive_irq_tx_ready,
	.irq_tx_complete  = uart_sifive_irq_tx_complete,
	.irq_rx_enable    = uart_sifive_irq_rx_enable,
	.irq_rx_disable   = uart_sifive_irq_rx_disable,
	.irq_rx_ready     = uart_sifive_irq_rx_ready,
	.irq_err_enable   = uart_sifive_irq_err_enable,
	.irq_err_disable  = uart_sifive_irq_err_disable,
	.irq_is_pending   = uart_sifive_irq_is_pending,
	.irq_update       = uart_sifive_irq_update,
	.irq_callback_set = uart_sifive_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_SIFIVE_PORT_0

static struct uart_sifive_data uart_sifive_data_0;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sifive_irq_cfg_func_0(void);
#endif

PINCTRL_DT_INST_DEFINE(0);

static const struct uart_sifive_device_config uart_sifive_dev_cfg_0 = {
	.port         = DT_INST_REG_ADDR(0),
	.sys_clk_freq = SIFIVE_PERIPHERAL_CLOCK_FREQUENCY,
	.baud_rate    = DT_INST_PROP(0, current_speed),
	.rxcnt_irq    = CONFIG_UART_SIFIVE_PORT_0_RXCNT_IRQ,
	.txcnt_irq    = CONFIG_UART_SIFIVE_PORT_0_TXCNT_IRQ,
	.pcfg	      = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cfg_func     = uart_sifive_irq_cfg_func_0,
#endif
};

DEVICE_DT_INST_DEFINE(0,
		    uart_sifive_init,
		    NULL,
		    &uart_sifive_data_0, &uart_sifive_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
		    (void *)&uart_sifive_driver_api);


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sifive_irq_cfg_func_0(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    uart_sifive_irq_handler, DEVICE_DT_INST_GET(0),
		    0);

	irq_enable(DT_INST_IRQN(0));
}
#endif

#endif /* CONFIG_UART_SIFIVE_PORT_0 */

#ifdef CONFIG_UART_SIFIVE_PORT_1

static struct uart_sifive_data uart_sifive_data_1;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sifive_irq_cfg_func_1(void);
#endif

PINCTRL_DT_INST_DEFINE(1);

static const struct uart_sifive_device_config uart_sifive_dev_cfg_1 = {
	.port         = DT_INST_REG_ADDR(1),
	.sys_clk_freq = SIFIVE_PERIPHERAL_CLOCK_FREQUENCY,
	.baud_rate    = DT_INST_PROP(1, current_speed),
	.rxcnt_irq    = CONFIG_UART_SIFIVE_PORT_1_RXCNT_IRQ,
	.txcnt_irq    = CONFIG_UART_SIFIVE_PORT_1_TXCNT_IRQ,
	.pcfg	      = PINCTRL_DT_INST_DEV_CONFIG_GET(1),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cfg_func     = uart_sifive_irq_cfg_func_1,
#endif
};

DEVICE_DT_INST_DEFINE(1,
		    uart_sifive_init,
		    NULL,
		    &uart_sifive_data_1, &uart_sifive_dev_cfg_1,
		    PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,
		    (void *)&uart_sifive_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_sifive_irq_cfg_func_1(void)
{
	IRQ_CONNECT(DT_INST_IRQN(1), DT_INST_IRQ(1, priority),
		    uart_sifive_irq_handler, DEVICE_DT_INST_GET(1),
		    0);

	irq_enable(DT_INST_IRQN(1));
}
#endif

#endif /* CONFIG_UART_SIFIVE_PORT_1 */
