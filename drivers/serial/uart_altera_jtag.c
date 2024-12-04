/*
 * Copyright (c) 2017-2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for Intel FPGA UART Core IP
 * Reference : Embedded Peripherals IP User Guide : 12. JTAG UART Core
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/sys/__assert.h>

#define DT_DRV_COMPAT   altr_jtag_uart

#define UART_ALTERA_JTAG_DATA_OFFSET    0x00        /* DATA : Register offset */
#define UART_ALTERA_JTAG_CTRL_OFFSET    0x04        /* CTRL : Register offset */
#define UART_IE_TX                      (1 << 1)    /* CTRL : TX Interrupt Enable */
#define UART_IE_RX                      (1 << 0)    /* CTRL : RX Interrupt Enable */
#define UART_DATA_MASK                  0xFF        /* DATA : Data Mask */
#define UART_WFIFO_MASK                 0xFFFF0000  /* CTRL : Transmit FIFO Mask */
#define UART_WFIFO_OFST                 (16)

#define ALTERA_AVALON_JTAG_UART_DATA_DATA_OFST            (0)
#define ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK           (0x00008000)

#define ALTERA_AVALON_JTAG_UART_CONTROL_RI_MSK            (0x00000100)
#define ALTERA_AVALON_JTAG_UART_CONTROL_WI_MSK            (0x00000200)

#ifdef CONFIG_UART_ALTERA_JTAG_HAL
#include "altera_avalon_jtag_uart.h"
#include "altera_avalon_jtag_uart_regs.h"

extern int altera_avalon_jtag_uart_read(altera_avalon_jtag_uart_state *sp,
		char *buffer, int space, int flags);
extern int altera_avalon_jtag_uart_write(altera_avalon_jtag_uart_state *sp,
		const char *ptr, int count, int flags);
#else

/* device data */
struct uart_altera_jtag_device_data {
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;  /* Callback function pointer */
	void *cb_data;                     /* Callback function arg */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* device config */
struct uart_altera_jtag_device_config {
	mm_reg_t base;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t  irq_config_func;
	unsigned int irq_num;
	uint16_t write_fifo_depth;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */

#ifndef CONFIG_UART_ALTERA_JTAG_HAL
/**
 * @brief Poll the device for input.
 *
 * @param dev UART device instance
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 otherwise.
 * -EINVAL if c is null pointer.
 */
static int uart_altera_jtag_poll_in(const struct device *dev,
					       unsigned char *c)
{
	int ret = -1;
	const struct uart_altera_jtag_device_config *config = dev->config;
	struct uart_altera_jtag_device_data *data = dev->data;
	unsigned int input_data;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(c != NULL, "c is null pointer!");

	/* Stop, if c is null pointer */
	if (c == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	input_data = sys_read32(config->base + UART_ALTERA_JTAG_DATA_OFFSET);

	/* check if data is valid. */
	if (input_data & ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK) {
		*c = (input_data & UART_DATA_MASK) >> ALTERA_AVALON_JTAG_UART_DATA_DATA_OFST;
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}
#endif  /* CONFIG_UART_ALTERA_JTAG_HAL */

/**
 * @brief Output a character in polled mode.
 *
 * This routine checks if the transmitter is full.
 * When the transmitter is not full, it writes a character to the data register.
 * It waits and blocks the calling thread, otherwise. This function is a blocking call.
 *
 * @param dev UART device instance
 * @param c Character to send
 */
static void uart_altera_jtag_poll_out(const struct device *dev,
					       unsigned char c)
{
#ifdef CONFIG_UART_ALTERA_JTAG_HAL
	altera_avalon_jtag_uart_state ustate;

	ustate.base = JTAG_UART_0_BASE;
	altera_avalon_jtag_uart_write(&ustate, &c, 1, 0);
#else
	const struct uart_altera_jtag_device_config *config = dev->config;
	struct uart_altera_jtag_device_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* While TX FIFO full */
	while (!(sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET) & UART_WFIFO_MASK)) {
	}

	sys_write8(c, config->base + UART_ALTERA_JTAG_DATA_OFFSET);

	k_spin_unlock(&data->lock, key);
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */
}

/**
 * @brief Initialise an instance of the driver
 *
 * This function initialise the interrupt configuration for the driver.
 *
 * @param dev UART device instance
 *
 * @return 0 to indicate success.
 */
static int uart_altera_jtag_init(const struct device *dev)
{
	/*
	 * Work around to clear interrupt enable bits
	 * as it is not being done by HAL driver explicitly.
	 */
#ifdef CONFIG_UART_ALTERA_JTAG_HAL
	IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_0_BASE, 0);
#else
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/*
	 * Enable hardware interrupt.
	 * The corresponding csr from IP still needs to be set,
	 * so that the IP generates interrupt signal.
	 */
	config->irq_config_func(dev);
#endif
	/*  Disable the tx and rx interrupt signals from JTAG core IP. */
	ctrl_val &= ~(UART_IE_TX | UART_IE_RX);
	sys_write32(ctrl_val, config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */
	return 0;
}

/*
 * Functions for Interrupt driven API
 */
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && !defined(CONFIG_UART_ALTERA_JTAG_HAL)

/**
 * @brief Fill FIFO with data
 * This function is expected to be called from UART interrupt handler (ISR),
 * if uart_irq_tx_ready() returns true.
 *
 * @param dev UART device instance
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_altera_jtag_fifo_fill(const struct device *dev,
									  const uint8_t *tx_data,
									  int size)
{
	const struct uart_altera_jtag_device_config *config = dev->config;
	struct uart_altera_jtag_device_data *data = dev->data;
	uint32_t ctrl_val;
	uint32_t space = 0;

	int i;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(tx_data != NULL, "tx buffer is null pointer!");

	/* Stop, if buffer is null pointer */
	if (tx_data == NULL) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
	space = (ctrl_val & UART_WFIFO_MASK) >> UART_WFIFO_OFST;

	/* guard for tx data overflow:
	 * make sure that driver is not sending more than current available space.
	 */
	for (i = 0; (i < size) && (i < space); i++) {
		sys_write8(tx_data[i], config->base + UART_ALTERA_JTAG_DATA_OFFSET);
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Read data from FIFO
 * This function is expected to be called from UART interrupt handler (ISR),
 * if uart_irq_rx_ready() returns true.
 *
 * @param dev UART device instance
 * @param rx_data Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_altera_jtag_fifo_read(const struct device *dev, uint8_t *rx_data,
									  const int size)
{
	const struct uart_altera_jtag_device_config *config = dev->config;
	struct uart_altera_jtag_device_data *data = dev->data;
	int i;
	unsigned int input_data;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(rx_data != NULL, "Rx buffer is null pointer!");

	/* Stop, if buffer is null pointer */
	if (rx_data == NULL) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; i < size; i++) {
		input_data = sys_read32(config->base + UART_ALTERA_JTAG_DATA_OFFSET);

		if (input_data & ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK) {
			rx_data[i] = (input_data & UART_DATA_MASK) >>
						  ALTERA_AVALON_JTAG_UART_DATA_DATA_OFST;
		} else {
			/* break upon invalid data or no more data */
			break;
		}
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device instance
 */
static void uart_altera_jtag_irq_tx_enable(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ctrl_val |= UART_IE_TX;
	sys_write32(ctrl_val, config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device instance
 */
static void uart_altera_jtag_irq_tx_disable(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ctrl_val &= ~UART_IE_TX;
	sys_write32(ctrl_val, config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if UART TX buffer can accept a new char.
 *
 * @param dev UART device instance
 *
 * @return 1 if TX interrupt is enabled and at least one char can be written to UART.
 *         0 If device is not ready to write a new byte.
 */
static int uart_altera_jtag_irq_tx_ready(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
	int ret = 0;
	uint32_t space = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* if TX interrupt is enabled */
	if (ctrl_val & ALTERA_AVALON_JTAG_UART_CONTROL_WI_MSK) {
		/* check for space in tx fifo */
		space = (ctrl_val & UART_WFIFO_MASK) >> UART_WFIFO_OFST;
		if (space) {
			ret = 1;
		}
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device instance
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_altera_jtag_irq_tx_complete(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
	int ret = 0;
	uint32_t space = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* note: This is checked indirectly via the space in tx fifo. */
	space = (ctrl_val & UART_WFIFO_MASK) >> UART_WFIFO_OFST;
	if (space == config->write_fifo_depth) {
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device instance
 */
static void uart_altera_jtag_irq_rx_enable(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ctrl_val |= UART_IE_RX;
	sys_write32(ctrl_val, config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device instance
 */
static void uart_altera_jtag_irq_rx_disable(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ctrl_val &= ~UART_IE_RX;
	sys_write32(ctrl_val, config->base + UART_ALTERA_JTAG_CTRL_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device instance
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_altera_jtag_irq_rx_ready(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (ctrl_val & ALTERA_AVALON_JTAG_UART_CONTROL_RI_MSK) {
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device instance
 *
 * @return Always 1
 */
static int uart_altera_jtag_irq_update(const struct device *dev)
{
	return 1;
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device instance
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_altera_jtag_irq_is_pending(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	const struct uart_altera_jtag_device_config *config = dev->config;
	uint32_t ctrl_val = sys_read32(config->base + UART_ALTERA_JTAG_CTRL_OFFSET);
	int ret = 0;

	if (ctrl_val &
		(ALTERA_AVALON_JTAG_UART_CONTROL_RI_MSK|ALTERA_AVALON_JTAG_UART_CONTROL_WI_MSK)) {
		ret = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device instance
 * @param cb Callback function pointer.
 * @param cb_data Data to pass to callback function.
 */
static void uart_altera_jtag_irq_callback_set(const struct device *dev,
								  uart_irq_callback_user_data_t cb,
								  void *cb_data)
{
	struct uart_altera_jtag_device_data *data = dev->data;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(cb != NULL, "uart_irq_callback_user_data_t cb is null pointer!");

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->cb = cb;
	data->cb_data = cb_data;

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param dev Pointer to UART device struct
 */
static void uart_altera_jtag_isr(const struct device *dev)
{
	struct uart_altera_jtag_device_data *data = dev->data;
	uart_irq_callback_user_data_t callback = data->cb;

	if (callback) {
		callback(dev, data->cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN && !CONFIG_UART_ALTERA_JTAG_HAL */

static DEVICE_API(uart, uart_altera_jtag_driver_api) = {
#ifndef CONFIG_UART_ALTERA_JTAG_HAL
	.poll_in = uart_altera_jtag_poll_in,
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */
	.poll_out = uart_altera_jtag_poll_out,
	.err_check = NULL,
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && !defined(CONFIG_UART_ALTERA_JTAG_HAL)
	.fifo_fill = uart_altera_jtag_fifo_fill,
	.fifo_read = uart_altera_jtag_fifo_read,
	.irq_tx_enable = uart_altera_jtag_irq_tx_enable,
	.irq_tx_disable = uart_altera_jtag_irq_tx_disable,
	.irq_tx_ready = uart_altera_jtag_irq_tx_ready,
	.irq_tx_complete = uart_altera_jtag_irq_tx_complete,
	.irq_rx_enable = uart_altera_jtag_irq_rx_enable,
	.irq_rx_disable = uart_altera_jtag_irq_rx_disable,
	.irq_rx_ready = uart_altera_jtag_irq_rx_ready,
	.irq_is_pending = uart_altera_jtag_irq_is_pending,
	.irq_update = uart_altera_jtag_irq_update,
	.irq_callback_set = uart_altera_jtag_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN && !CONFIG_UART_ALTERA_JTAG_HAL */
};

#ifdef CONFIG_UART_ALTERA_JTAG_HAL
#define UART_ALTERA_JTAG_DEVICE_INIT(n)						\
DEVICE_DT_INST_DEFINE(n, uart_altera_jtag_init, NULL, NULL, NULL, PRE_KERNEL_1,	\
		      CONFIG_SERIAL_INIT_PRIORITY,					\
		      &uart_altera_jtag_driver_api);
#else

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_ALTERA_JTAG_CONFIG_FUNC(n)					\
	static void uart_altera_jtag_irq_config_func_##n(const struct device *dev)  \
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),		\
				DT_INST_IRQ(n, priority),	\
				uart_altera_jtag_isr,		\
				DEVICE_DT_INST_GET(n), 0);	\
											\
		irq_enable(DT_INST_IRQN(n));		\
	}

#define UART_ALTERA_JTAG_CONFIG_INIT(n)		\
	.irq_config_func = uart_altera_jtag_irq_config_func_##n,	\
	.irq_num = DT_INST_IRQN(n),				\
	.write_fifo_depth = DT_INST_PROP_OR(n, write_fifo_depth, 0),\

#else
#define UART_ALTERA_JTAG_CONFIG_FUNC(n)
#define UART_ALTERA_JTAG_CONFIG_INIT(n)
#define UART_ALTERA_JTAG_DATA_INIT(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_ALTERA_JTAG_DEVICE_INIT(n)				\
UART_ALTERA_JTAG_CONFIG_FUNC(n)						\
static struct uart_altera_jtag_device_data uart_altera_jtag_device_data_##n = {		\
};											\
											\
static const struct uart_altera_jtag_device_config uart_altera_jtag_dev_cfg_##n = { \
	.base = DT_INST_REG_ADDR(n),					\
	UART_ALTERA_JTAG_CONFIG_INIT(n)					\
};											\
DEVICE_DT_INST_DEFINE(n,							\
		      uart_altera_jtag_init,				\
		      NULL,									\
		      &uart_altera_jtag_device_data_##n,	\
		      &uart_altera_jtag_dev_cfg_##n,		\
		      PRE_KERNEL_1,							\
		      CONFIG_SERIAL_INIT_PRIORITY,			\
		      &uart_altera_jtag_driver_api);
#endif /* CONFIG_UART_ALTERA_JTAG_HAL */

DT_INST_FOREACH_STATUS_OKAY(UART_ALTERA_JTAG_DEVICE_INIT)
