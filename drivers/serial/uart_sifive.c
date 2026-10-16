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
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

#define RXDATA_EMPTY   (1 << 31)   /* Receive FIFO Empty */
#define RXDATA_MASK    0xFF        /* Receive Data Mask */

#define TXDATA_FULL    (1 << 31)   /* Transmit FIFO Full */

#define TXCTRL_TXEN    (1 << 0)    /* Activate Tx Channel */

#define RXCTRL_RXEN    (1 << 0)    /* Activate Rx Channel */

#define IE_TXWM        (1 << 0)    /* TX Interrupt Enable/Pending */
#define IE_RXWM        (1 << 1)    /* RX Interrupt Enable/Pending */

/* txwm pending while the TX FIFO is empty */
#define TXCTRL_TXCNT   (1U << 16)

/* rxwm pending while the RX FIFO is not empty */
#define RXCTRL_RXCNT   (0U << 16)

/* Register offsets */
#define UART_SIFIVE_TXDATA 0x00
#define UART_SIFIVE_RXDATA 0x04
#define UART_SIFIVE_TXCTRL 0x08
#define UART_SIFIVE_RXCTRL 0x0c
#define UART_SIFIVE_IE     0x10
#define UART_SIFIVE_IP     0x14
#define UART_SIFIVE_DIV    0x18

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
typedef void (*irq_cfg_func_t)(void);
#endif

struct uart_sifive_device_config {
	DEVICE_MMIO_ROM;
	uint32_t	baud_rate;
	const struct	pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_cfg_func_t	cfg_func;
#endif
};

struct uart_sifive_data {
	DEVICE_MMIO_RAM;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

static inline uint32_t uart_sifive_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void uart_sifive_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

/*
 * Writes to txdata are ignored while the TX FIFO is full, so a single AMO
 * reads the full flag and enqueues the character in one bus access.
 */
static inline bool uart_sifive_tx_try(const struct device *dev, uint8_t c)
{
#ifdef CONFIG_RISCV_ISA_EXT_ZAAMO
	mm_reg_t reg = DEVICE_MMIO_GET(dev) + UART_SIFIVE_TXDATA;
	uint32_t val;

	__asm__ volatile("amoor.w %0, %1, %2"
			 : "=r"(val)
			 : "r"((uint32_t)c), "A"(*(volatile uint32_t *)reg)
			 : "memory");

	return (val & TXDATA_FULL) == 0U;
#else
	if ((uart_sifive_read(dev, UART_SIFIVE_TXDATA) & TXDATA_FULL) != 0U) {
		return false;
	}

	uart_sifive_write(dev, UART_SIFIVE_TXDATA, c);

	return true;
#endif
}

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
	/* Retry while TX FIFO is full */
	while (!uart_sifive_tx_try(dev, c)) {
	}
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
	uint32_t val = uart_sifive_read(dev, UART_SIFIVE_RXDATA);

	if ((val & RXDATA_EMPTY) != 0U) {
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
	int i;

	for (i = 0; i < size; i++) {
		if (!uart_sifive_tx_try(dev, tx_data[i])) {
			break;
		}
	}

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
	int i;
	uint32_t val;

	for (i = 0; i < size; i++) {
		val = uart_sifive_read(dev, UART_SIFIVE_RXDATA);

		if ((val & RXDATA_EMPTY) != 0U) {
			break;
		}

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
	sys_set_bits(DEVICE_MMIO_GET(dev) + UART_SIFIVE_IE, IE_TXWM);
}

/**
 * @brief Disable TX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_tx_disable(const struct device *dev)
{
	sys_clear_bits(DEVICE_MMIO_GET(dev) + UART_SIFIVE_IE, IE_TXWM);
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
	return (uart_sifive_read(dev, UART_SIFIVE_IP) & IE_TXWM) != 0U;
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
	/*
	 * No TX EMPTY flag for this controller, but with TXCTRL_TXCNT
	 * the TX watermark is pending exactly while the TX FIFO is empty.
	 */
	return (uart_sifive_read(dev, UART_SIFIVE_IP) & IE_TXWM) != 0U;
}

/**
 * @brief Enable RX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_rx_enable(const struct device *dev)
{
	sys_set_bits(DEVICE_MMIO_GET(dev) + UART_SIFIVE_IE, IE_RXWM);
}

/**
 * @brief Disable RX interrupt in ie register
 *
 * @param dev UART device struct
 */
static void uart_sifive_irq_rx_disable(const struct device *dev)
{
	sys_clear_bits(DEVICE_MMIO_GET(dev) + UART_SIFIVE_IE, IE_RXWM);
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
	return (uart_sifive_read(dev, UART_SIFIVE_IP) & IE_RXWM) != 0U;
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
	/* ip reports the watermark conditions regardless of ie */
	return (uart_sifive_read(dev, UART_SIFIVE_IP) & uart_sifive_read(dev, UART_SIFIVE_IE) &
		(IE_RXWM | IE_TXWM)) != 0U;
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

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static int uart_sifive_init(const struct device *dev)
{
	const struct uart_sifive_device_config * const cfg = dev->config;
#ifdef CONFIG_PINCTRL
	int ret;
#endif

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Enable TX and RX channels */
	uart_sifive_write(dev, UART_SIFIVE_TXCTRL, TXCTRL_TXEN | TXCTRL_TXCNT);
	uart_sifive_write(dev, UART_SIFIVE_RXCTRL, RXCTRL_RXEN | RXCTRL_RXCNT);

	/* Set baud rate */
	uart_sifive_write(dev, UART_SIFIVE_DIV,
			  SIFIVE_PERIPHERAL_CLOCK_FREQUENCY / cfg->baud_rate - 1U);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Ensure that uart IRQ is disabled initially */
	uart_sifive_write(dev, UART_SIFIVE_IE, 0U);

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

static DEVICE_API(uart, uart_sifive_driver_api) = {
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
	.irq_callback_set = uart_sifive_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_SIFIVE_IRQ_CFG_FUNC_DEFINE(n)                                                         \
	static void uart_sifive_irq_cfg_func_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_sifive_irq_handler,    \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#else
#define UART_SIFIVE_IRQ_CFG_FUNC_DEFINE(n)
#endif

#define UART_SIFIVE_INIT(n)                                                                        \
	static struct uart_sifive_data uart_sifive_data_##n;                                       \
                                                                                                   \
	UART_SIFIVE_IRQ_CFG_FUNC_DEFINE(n)                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct uart_sifive_device_config uart_sifive_dev_cfg_##n = {                  \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.baud_rate = DT_INST_PROP(n, current_speed),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			   (.cfg_func = uart_sifive_irq_cfg_func_##n,))                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_sifive_init, NULL, &uart_sifive_data_##n,                    \
			      &uart_sifive_dev_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      (void *)&uart_sifive_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_SIFIVE_INIT)
