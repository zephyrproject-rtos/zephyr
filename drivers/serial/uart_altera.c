/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for Intel FPGA UART Core IP
 * Reference : Embedded Peripherals IP User Guide : 11. UART Core
 * Limitations:
 * 1. User should consider to always use polling mode, as IP core does not have fifo.
 *    So IP can only send/receive 1 character at a time.
 * 2. CTS and RTS is purely software controlled. Assertion might not be on time.
 * 3. Full duplex mode is not supported.
 */

#define DT_DRV_COMPAT   altr_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/serial/uart_altera.h>

#ifdef CONFIG_UART_LINE_CTRL
	#ifndef CONFIG_UART_INTERRUPT_DRIVEN
		/* CTS and RTS is purely software controlled. */
		#error "uart_altera.c: Must enable UART_INTERRUPT_DRIVEN for line control!"
	#endif
#endif

/* register offsets */
#define ALTERA_AVALON_UART_OFFSET  (0x4)

#define ALTERA_AVALON_UART_RXDATA_REG_OFFSET    (0 * ALTERA_AVALON_UART_OFFSET)
#define ALTERA_AVALON_UART_TXDATA_REG_OFFSET    (1 * ALTERA_AVALON_UART_OFFSET)
#define ALTERA_AVALON_UART_STATUS_REG_OFFSET    (2 * ALTERA_AVALON_UART_OFFSET)
#define ALTERA_AVALON_UART_CONTROL_REG_OFFSET   (3 * ALTERA_AVALON_UART_OFFSET)
#define ALTERA_AVALON_UART_DIVISOR_REG_OFFSET   (4 * ALTERA_AVALON_UART_OFFSET)
#define ALTERA_AVALON_UART_EOP_REG_OFFSET       (5 * ALTERA_AVALON_UART_OFFSET)

/*status register mask */
#define ALTERA_AVALON_UART_STATUS_PE_MSK              (0x1)
#define ALTERA_AVALON_UART_STATUS_FE_MSK              (0x2)
#define ALTERA_AVALON_UART_STATUS_BRK_MSK             (0x4)
#define ALTERA_AVALON_UART_STATUS_ROE_MSK             (0x8)
#define ALTERA_AVALON_UART_STATUS_TMT_MSK             (0x20)
#define ALTERA_AVALON_UART_STATUS_TRDY_MSK            (0x40)
#define ALTERA_AVALON_UART_STATUS_RRDY_MSK            (0x80)
#define ALTERA_AVALON_UART_STATUS_DCTS_MSK            (0x400)
#define ALTERA_AVALON_UART_STATUS_CTS_MSK             (0x800)
#define ALTERA_AVALON_UART_STATUS_E_MSK               (0x100)
#define ALTERA_AVALON_UART_STATUS_EOP_MSK             (0x1000)

/* control register mask */
#define ALTERA_AVALON_UART_CONTROL_TMT_MSK             (0x20)
#define ALTERA_AVALON_UART_CONTROL_TRDY_MSK            (0x40)
#define ALTERA_AVALON_UART_CONTROL_RRDY_MSK            (0x80)
#define ALTERA_AVALON_UART_CONTROL_E_MSK               (0x100)
#define ALTERA_AVALON_UART_CONTROL_DCTS_MSK            (0x400)
#define ALTERA_AVALON_UART_CONTROL_RTS_MSK             (0x800)
#define ALTERA_AVALON_UART_CONTROL_EOP_MSK            (0x1000)

/* defined values */
#define UART_ALTERA_NO_ERROR (0u)
#define ALTERA_AVALON_UART_CLEAR_STATUS_VAL (0u)
#define ALTERA_AVALON_UART_PENDING_MASK  (ALTERA_AVALON_UART_STATUS_RRDY_MSK | \
			  ALTERA_AVALON_UART_STATUS_TRDY_MSK | ALTERA_AVALON_UART_STATUS_E_MSK | \
			  ALTERA_AVALON_UART_STATUS_EOP_MSK)

/***********************/
/* configuration flags */
/*
 * The value ALT_AVALON_UART_FB is a value set in the devices flag field to
 * indicate that the device has a fixed baud rate; i.e. if this flag is set
 * software can not control the baud rate of the device.
 */
#define ALT_AVALON_UART_FB 0x1

/*
 * The value ALT_AVALON_UART_FC is a value set in the device flag field to
 * indicate the device is using flow control, i.e. the driver must
 * throttle on transmit if the nCTS pin is low.
 */
#define ALT_AVALON_UART_FC 0x2

/* end of configuration flags */
/******************************/

/* device data */
struct uart_altera_device_data {
	struct uart_config uart_cfg; /* stores uart config from device tree*/
	struct k_spinlock lock;
	uint32_t status_act; /* stores value of status register. */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;  /**< Callback function pointer */
	void *cb_data;  /**< Callback function arg */
#ifdef CONFIG_UART_ALTERA_EOP
	uint8_t set_eop_cb;
	uart_irq_callback_user_data_t cb_eop;  /**< Callback function pointer */
	void *cb_data_eop;  /**< Callback function arg */
#endif /* CONFIG_UART_ALTERA_EOP */
#ifdef CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND
	uint8_t dcts_rising;
#endif /*CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND*/
	uint32_t control_val; /* stores value to set control register. */
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/*
 * device config:
 * stores data that cannot be changed during run time.
 */
struct uart_altera_device_config {
	mm_reg_t base;
	uint32_t flags;              /* refer to configuration flags */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t  irq_config_func;
	unsigned int irq_num;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * function prototypes
 */
static int uart_altera_irq_update(const struct device *dev);
static int uart_altera_irq_tx_ready(const struct device *dev);
static int uart_altera_irq_rx_ready(const struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/**
 * @brief Poll the device for input.
 *
 * This is a non-blocking function.
 *
 * @param dev UART device instance
 * @param p_char Pointer to character
 *
 * @return 0 if a character arrived, -1 if input buffer is empty.
 * -EINVAL if p_char is null pointer
 */
static int uart_altera_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_altera_device_config *config = dev->config;
	struct uart_altera_device_data *data = dev->data;
	int ret_val = -1;
	uint32_t status;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(p_char != NULL, "p_char is null pointer!");

	/* Stop, if p_char is null pointer */
	if (p_char == NULL) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* check if received character is ready.*/
	status = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	if (status & ALTERA_AVALON_UART_STATUS_RRDY_MSK) {
		/* got a character */
		*p_char = sys_read32(config->base + ALTERA_AVALON_UART_RXDATA_REG_OFFSET);
		ret_val = 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief Output a character in polled mode.
 *
 * This function will block until transmitter is ready.
 * Then, a character will be transmitted.
 *
 * @param dev UART device instance
 * @param c Character to send
 */
static void uart_altera_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_altera_device_config *config = dev->config;
	struct uart_altera_device_data *data = dev->data;
	uint32_t status;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	do {
		/* wait until uart is free to transmit.*/
		status = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	} while ((status & ALTERA_AVALON_UART_STATUS_TRDY_MSK) == 0);

	sys_write32(c, config->base + ALTERA_AVALON_UART_TXDATA_REG_OFFSET);

	k_spin_unlock(&data->lock, key);
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
static int uart_altera_init(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;
	/* clear status to ensure, that interrupts are not triggered due to old status. */
	sys_write32(ALTERA_AVALON_UART_CLEAR_STATUS_VAL, config->base
				+ ALTERA_AVALON_UART_STATUS_REG_OFFSET);

	/*
	 * Enable hardware interrupt.
	 * The corresponding csr from IP still needs to be set,
	 * so that the IP generates interrupt signal.
	 */
	config->irq_config_func(dev);

#ifdef CONFIG_UART_LINE_CTRL
	/* Enable DCTS interrupt. */
	data->control_val = ALTERA_AVALON_UART_CONTROL_DCTS_MSK;
#endif /* CONFIG_UART_LINE_CTRL */

	sys_write32(data->control_val, config->base + ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

/**
 * @brief Check if an error was received
 * If error is received, it will be mapped to uart_rx_stop_reason.
 * This function should be called after irq_update.
 * If interrupt driven API is not enabled,
 * this function will read and clear the status register.
 *
 * @param dev UART device struct
 *
 * @return UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_BREAK if an error was detected, 0 otherwise.
 */
static int uart_altera_err_check(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	int err = UART_ALTERA_NO_ERROR;

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	const struct uart_altera_device_config *config = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
#endif

	if (data->status_act & ALTERA_AVALON_UART_STATUS_E_MSK) {
		if (data->status_act & ALTERA_AVALON_UART_STATUS_PE_MSK) {
			err |= UART_ERROR_PARITY;
		}

		if (data->status_act & ALTERA_AVALON_UART_STATUS_FE_MSK) {
			err |= UART_ERROR_FRAMING;
		}

		if (data->status_act & ALTERA_AVALON_UART_STATUS_BRK_MSK) {
			err |= UART_BREAK;
		}

		if (data->status_act & ALTERA_AVALON_UART_STATUS_ROE_MSK) {
			err |= UART_ERROR_OVERRUN;
		}
	}

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
	/* clear status */
	sys_write32(ALTERA_AVALON_UART_CLEAR_STATUS_VAL, config->base
	+ ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	k_spin_unlock(&data->lock, key);
#endif

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/***
 * @brief helper function to check, if the configuration is support.
 * @param  cfg_stored : The original configuration.
 * @param  cfg_in     : The input configuration.
 * @return true if only baudrate is changed. otherwise false.
 */
static bool uart_altera_check_configuration(const struct uart_config *cfg_stored,
				   const struct uart_config *cfg_in)
{
	bool ret_val = false;

	if ((cfg_stored->parity       == cfg_in->parity)
		&& (cfg_stored->stop_bits == cfg_in->stop_bits)
		&& (cfg_stored->data_bits == cfg_in->data_bits)
		&& (cfg_stored->flow_ctrl == cfg_in->flow_ctrl)) {
		ret_val = true;
	}

	return ret_val;
}

/**
 * @brief Set UART configuration using data from *cfg_in.
 *
 * @param dev UART : Device struct
 * @param cfg_in   : The input configuration.
 *
 * @return 0 if success, -ENOTSUP, if input from cfg_in is not configurable.
 * -EINVAL if cfg_in is null pointer
 */
static int uart_altera_configure(const struct device *dev,
				   const struct uart_config *cfg_in)
{
	const struct uart_altera_device_config *config = dev->config;
	struct uart_altera_device_data * const data = dev->data;
	struct uart_config * const cfg_stored = &data->uart_cfg;
	uint32_t divisor_val;
	int ret_val;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(cfg_in != NULL, "cfg_in is null pointer!");

	/* Stop, if cfg_in is null pointer */
	if (cfg_in == NULL) {
		return -EINVAL;
	}

	/* check if configuration is supported. */
	if (uart_altera_check_configuration(cfg_stored, cfg_in)
		&& !(config->flags & ALT_AVALON_UART_FB)) {
		/* calculate and set baudrate. */
		divisor_val = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/cfg_in->baudrate) - 1;
		sys_write32(divisor_val, config->base + ALTERA_AVALON_UART_DIVISOR_REG_OFFSET);

		/* update stored data. */
		cfg_stored->baudrate = cfg_in->baudrate;
		ret_val = 0;
	} else {
		/* return not supported */
		ret_val = -ENOTSUP;
	}

	return ret_val;
}

/**
 * @brief Get UART configuration and stores in *cfg_out.
 *
 * @param dev UART : Device struct
 * @param cfg_out   : The output configuration.
 *
 * @return 0 if success.
 * -EINVAL if cfg_out is null pointer
 */
static int uart_altera_config_get(const struct device *dev,
				struct uart_config *cfg_out)
{
	const struct uart_altera_device_data *data = dev->data;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(cfg_out != NULL, "cfg_out is null pointer!");

	/* Stop, if cfg_out is null pointer */
	if (cfg_out == NULL) {
		return -EINVAL;
	}

	*cfg_out = data->uart_cfg;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/**
 * @brief Fill FIFO with data
 * This function is expected to be called from UART interrupt handler (ISR),
 * if uart_irq_tx_ready() returns true. This function does not block!
 * IP has no fifo. Hence only 1 data can be sent at a time!
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send (unused)
 *
 * @return Number of bytes sent
 */
static int uart_altera_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	ARG_UNUSED(size);

	const struct uart_altera_device_config *config = dev->config;
	struct uart_altera_device_data *data = dev->data;
	int ret_val;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(tx_data != NULL, "tx_data is null pointer!");

	/* Stop, if tx_data is null pointer */
	if (tx_data == NULL) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->status_act & ALTERA_AVALON_UART_STATUS_TRDY_MSK) {
		sys_write32(*tx_data, config->base + ALTERA_AVALON_UART_TXDATA_REG_OFFSET);
		ret_val = 1;
		/* function may be called in a loop. update the actual status! */
		data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	} else {
		ret_val = 0;
	}

#ifdef CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND
	/* clear and CTS rising edge! */
	data->dcts_rising = 0;
#endif /* CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND */

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief Read data from FIFO
 * This function is expected to be called from UART interrupt handler (ISR),
 * if uart_irq_rx_ready() returns true.
 * IP has no fifo. Hence only 1 data can be read at a time!
 *
 * @param dev UART device struct
 * @param rx_data Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_altera_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	ARG_UNUSED(size);

	const struct uart_altera_device_config *config = dev->config;
	struct uart_altera_device_data *data = dev->data;
	int ret_val;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(rx_data != NULL, "rx_data is null pointer!");

	/* Stop, if rx_data is null pointer */
	if (rx_data == NULL) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->status_act & ALTERA_AVALON_UART_STATUS_RRDY_MSK) {
		*rx_data = sys_read32(config->base + ALTERA_AVALON_UART_RXDATA_REG_OFFSET);
		ret_val = 1;

		/* function may be called in a loop. update the actual status! */
		data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	} else {
		ret_val = 0;
	}

#ifdef CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND
	/* assert RTS as soon as rx data is read, as IP has no fifo. */
	data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	if (((data->status_act & ALTERA_AVALON_UART_STATUS_RRDY_MSK) == 0)
	  && (data->status_act & ALTERA_AVALON_UART_STATUS_CTS_MSK)) {
		data->control_val |= ALTERA_AVALON_UART_CONTROL_RTS_MSK;
		sys_write32(data->control_val, config->base
					+ ALTERA_AVALON_UART_CONTROL_REG_OFFSET);
	}
#endif /* CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND */

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev UART device struct
 */
static void uart_altera_irq_tx_enable(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->control_val |= ALTERA_AVALON_UART_CONTROL_TRDY_MSK;

#ifdef CONFIG_UART_LINE_CTRL
	/* also enable RTS, if flow control is enabled. */
	data->control_val |= ALTERA_AVALON_UART_CONTROL_RTS_MSK;
#endif

	sys_write32(data->control_val, config->base + ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable TX interrupt
 *
 * @param dev UART device struct
 */
static void uart_altera_irq_tx_disable(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->control_val &= ~ALTERA_AVALON_UART_CONTROL_TRDY_MSK;

#ifdef CONFIG_UART_LINE_CTRL
	/* also disable RTS, if flow control is enabled. */
	data->control_val &= ~ALTERA_AVALON_UART_CONTROL_RTS_MSK;
#endif

	sys_write32(data->control_val, config->base + ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if UART TX buffer can accept a new char.
 *
 * @param dev UART device struct
 *
 * @return 1 if TX interrupt is enabled and at least one char can be written to UART.
 *         0 if device is not ready to write a new byte.
 */
static int uart_altera_irq_tx_ready(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	int ret_val = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* if TX interrupt is enabled */
	if (data->control_val & ALTERA_AVALON_UART_CONTROL_TRDY_MSK) {
		/* IP core does not have fifo. Wait until tx data is completely shifted. */
		if (data->status_act & ALTERA_AVALON_UART_STATUS_TMT_MSK) {
			ret_val = 1;
		}
	}

#ifdef CONFIG_UART_LINE_CTRL
	/* if flow control is enabled, set tx not ready, if CTS is low. */
	if ((data->status_act & ALTERA_AVALON_UART_STATUS_CTS_MSK) == 0) {
		ret_val = 0;
	}
#ifdef CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND
	if (data->dcts_rising == 0) {
		ret_val = 0;
	}
#endif /* CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND */

#endif /* CONFIG_UART_LINE_CTRL */

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_altera_irq_tx_complete(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	int ret_val = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->status_act & ALTERA_AVALON_UART_STATUS_TMT_MSK) {
		ret_val = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief Enable RX interrupt in
 *
 * @param dev UART device struct
 */
static void uart_altera_irq_rx_enable(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->control_val |= ALTERA_AVALON_UART_CONTROL_RRDY_MSK;
	sys_write32(data->control_val, config->base + ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable RX interrupt
 *
 * @param dev UART device struct
 */
static void uart_altera_irq_rx_disable(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->control_val &= ~ALTERA_AVALON_UART_CONTROL_RRDY_MSK;
	sys_write32(data->control_val, config->base + ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_altera_irq_rx_ready(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	int ret_val = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* if RX interrupt is enabled */
	if (data->control_val & ALTERA_AVALON_UART_CONTROL_RRDY_MSK) {
		/* check for space in rx data register */
		if (data->status_act & ALTERA_AVALON_UART_STATUS_RRDY_MSK) {
			ret_val = 1;
		}
	}

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief This function will cache the status register.
 *
 * @param dev UART device struct
 *
 * @return 1 for success.
 */
static int uart_altera_irq_update(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);

	k_spin_unlock(&data->lock, key);

	return 1;
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_altera_irq_is_pending(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	int ret_val = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->status_act & data->control_val & ALTERA_AVALON_UART_PENDING_MASK) {
		ret_val = 1;
	}

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 * @param cb_data Data to pass to callback function.
 */
static void uart_altera_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct uart_altera_device_data *data = dev->data;

	/* generate fatal error if CONFIG_ASSERT is enabled. */
	__ASSERT(cb != NULL, "uart_irq_callback_user_data_t cb is null pointer!");

	k_spinlock_key_t key = k_spin_lock(&data->lock);

#ifdef CONFIG_UART_ALTERA_EOP
	if (data->set_eop_cb) {
		data->cb_eop = cb;
		data->cb_data_eop = cb_data;
		data->set_eop_cb = 0;
	} else {
		data->cb = cb;
		data->cb_data = cb_data;
	}
#else
	data->cb = cb;
	data->cb_data = cb_data;
#endif /* CONFIG_UART_ALTERA_EOP */

	k_spin_unlock(&data->lock, key);
}

#ifdef CONFIG_UART_LINE_CTRL
/**
 * @brief DCTS Interrupt service routine.
 *
 * Handles assertion and deassettion of CTS/RTS stignal
 *
 * @param dev Pointer to UART device struct
 */
static void uart_altera_dcts_isr(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Assume that user follows zephyr requirement and update status in their call back. */
	if (data->status_act & ALTERA_AVALON_UART_STATUS_CTS_MSK) {
#ifdef CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND
		data->dcts_rising = 1;
#endif /* CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND */

		/* check if device is ready to receive character */
		if ((data->status_act & ALTERA_AVALON_UART_STATUS_RRDY_MSK) == 0) {
			/* Assert RTS to inform other UART. */
			data->control_val |= ALTERA_AVALON_UART_CONTROL_RTS_MSK;
			sys_write32(data->control_val, config->base
						+ ALTERA_AVALON_UART_CONTROL_REG_OFFSET);
		}
	} else {
		/* other UART deasserts RTS */
		if (data->status_act & ALTERA_AVALON_UART_STATUS_TMT_MSK) {
			/* only deasserts if not transmitting. */
			data->control_val &= ~ALTERA_AVALON_UART_CONTROL_RTS_MSK;
			sys_write32(data->control_val, config->base
						+ ALTERA_AVALON_UART_CONTROL_REG_OFFSET);
		}
	}

	k_spin_unlock(&data->lock, key);
}
#endif /* CONFIG_UART_LINE_CTRL */

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param dev Pointer to UART device struct
 *
 */
static void uart_altera_isr(const struct device *dev)
{
	struct uart_altera_device_data *data = dev->data;
	const struct uart_altera_device_config *config = dev->config;

	uart_irq_callback_user_data_t callback = data->cb;

	/* Pre ISR */
#ifdef CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND
	/* deassert RTS as soon as rx data is received, as IP has no fifo. */
	data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);
	if (data->status_act & ALTERA_AVALON_UART_STATUS_RRDY_MSK) {
		data->control_val &= ~ALTERA_AVALON_UART_CONTROL_RTS_MSK;
		sys_write32(data->control_val, config->base
					+ ALTERA_AVALON_UART_CONTROL_REG_OFFSET);
	}
#endif /* CONFIG_UART_ALTERA_LINE_CTRL_WORKAROUND */

	if (callback) {
		callback(dev, data->cb_data);
	}

	/* Post ISR */
#if CONFIG_UART_ALTERA_EOP
	data->status_act = sys_read32(config->base + ALTERA_AVALON_UART_STATUS_REG_OFFSET);

	if (data->status_act & ALTERA_AVALON_UART_STATUS_EOP_MSK) {
		callback = data->cb_eop;
		if (callback) {
			callback(dev, data->cb_data_eop);
		}
	}
#endif /* CONFIG_UART_ALTERA_EOP */

#ifdef CONFIG_UART_LINE_CTRL
	/* handles RTS/CTS signal */
	if (data->status_act & ALTERA_AVALON_UART_STATUS_DCTS_MSK) {
		uart_altera_dcts_isr(dev);
	}
#endif

	/* clear status after all interrupts are handled. */
	sys_write32(ALTERA_AVALON_UART_CLEAR_STATUS_VAL, config->base
				+ ALTERA_AVALON_UART_STATUS_REG_OFFSET);
}

#ifdef CONFIG_UART_DRV_CMD
/**
 * @brief Send extra command to driver
 *
 * @param dev UART device struct
 * @param cmd Command to driver
 * @param p Parameter to the command
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_altera_drv_cmd(const struct device *dev, uint32_t cmd,
				uint32_t p)
{
	struct uart_altera_device_data *data = dev->data;
#if CONFIG_UART_ALTERA_EOP
	const struct uart_altera_device_config *config = dev->config;
#endif
	int ret_val = -ENOTSUP;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	switch (cmd) {
#if CONFIG_UART_ALTERA_EOP
	case CMD_ENABLE_EOP:
		/* enable EOP interrupt */
		data->control_val |= ALTERA_AVALON_UART_CONTROL_EOP_MSK;
		sys_write32(data->control_val, config->base
					+ ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

		/* set EOP character */
		sys_write32((uint8_t) p, config->base + ALTERA_AVALON_UART_EOP_REG_OFFSET);

		/* after this, user needs to call uart_irq_callback_set
		 * to set data->cb_eop and data->cb_data_eop!
		 */
		data->set_eop_cb = 1;
		ret_val = 0;
		break;

	case CMD_DISABLE_EOP:
		/* Disable EOP interrupt */
		data->control_val &= ~ALTERA_AVALON_UART_CONTROL_EOP_MSK;
		sys_write32(data->control_val, config->base
					+ ALTERA_AVALON_UART_CONTROL_REG_OFFSET);

		/* clear call back */
		data->cb_eop = NULL;
		data->cb_data_eop = NULL;
		ret_val = 0;
		break;
#endif /* CONFIG_UART_ALTERA_EOP */
	default:
		ret_val = -ENOTSUP;
		break;
	};

	k_spin_unlock(&data->lock, key);

	return ret_val;
}

#endif /* CONFIG_UART_DRV_CMD */

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_altera_driver_api = {
	.poll_in = uart_altera_poll_in,
	.poll_out = uart_altera_poll_out,
	.err_check = uart_altera_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_altera_configure,
	.config_get = uart_altera_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_altera_fifo_fill,
	.fifo_read = uart_altera_fifo_read,
	.irq_tx_enable = uart_altera_irq_tx_enable,
	.irq_tx_disable = uart_altera_irq_tx_disable,
	.irq_tx_ready = uart_altera_irq_tx_ready,
	.irq_tx_complete = uart_altera_irq_tx_complete,
	.irq_rx_enable = uart_altera_irq_rx_enable,
	.irq_rx_disable = uart_altera_irq_rx_disable,
	.irq_rx_ready = uart_altera_irq_rx_ready,
	.irq_is_pending = uart_altera_irq_is_pending,
	.irq_update = uart_altera_irq_update,
	.irq_callback_set = uart_altera_irq_callback_set,
#endif

#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = uart_altera_drv_cmd,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define UART_ALTERA_IRQ_CONFIG_FUNC(n)                                    \
	static void uart_altera_irq_config_func_##n(const struct device *dev) \
	{                                                                     \
		IRQ_CONNECT(DT_INST_IRQN(n),                                      \
				DT_INST_IRQ(n, priority),                                 \
				uart_altera_isr,                                          \
				DEVICE_DT_INST_GET(n), 0);		                          \
		                                                                  \
		irq_enable(DT_INST_IRQN(n));                                      \
	}

#define UART_ALTERA_IRQ_CONFIG_INIT(n)                                    \
	.irq_config_func = uart_altera_irq_config_func_##n,                   \
	.irq_num = DT_INST_IRQN(n),

#else

#define UART_ALTERA_IRQ_CONFIG_FUNC(n)
#define UART_ALTERA_IRQ_CONFIG_INIT(n)

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_ALTERA_DEVICE_INIT(n)                                        \
UART_ALTERA_IRQ_CONFIG_FUNC(n)                                            \
static struct uart_altera_device_data uart_altera_dev_data_##n = {        \
	.uart_cfg =                                                           \
	{                                                                     \
			.baudrate = DT_INST_PROP(n, current_speed),                   \
			.parity = DT_INST_ENUM_IDX_OR(n, parity,                      \
						 UART_CFG_PARITY_NONE),                           \
			.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits,                \
						 UART_CFG_STOP_BITS_1),                           \
			.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits,                \
						 UART_CFG_DATA_BITS_8),                           \
			.flow_ctrl = DT_INST_PROP(n, hw_flow_control) ?               \
				UART_CFG_FLOW_CTRL_RTS_CTS :                              \
				UART_CFG_FLOW_CTRL_NONE,                                  \
	},                                                                    \
};                                                                        \
	                                                                      \
static const struct uart_altera_device_config uart_altera_dev_cfg_##n = { \
	.base = DT_INST_REG_ADDR(n),                                          \
	.flags = ((DT_INST_PROP(n, fixed_baudrate)?ALT_AVALON_UART_FB:0)      \
			  |(DT_INST_PROP(n, hw_flow_control)?ALT_AVALON_UART_FC:0)),  \
	UART_ALTERA_IRQ_CONFIG_INIT(n)                                        \
};                                                                        \
	                                                                      \
DEVICE_DT_INST_DEFINE(n,                                                  \
			  uart_altera_init,                                           \
			  NULL,                                                       \
			  &uart_altera_dev_data_##n,                                  \
			  &uart_altera_dev_cfg_##n,                                   \
			  PRE_KERNEL_1,                                               \
			  CONFIG_SERIAL_INIT_PRIORITY,                                \
			  &uart_altera_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_ALTERA_DEVICE_INIT)
