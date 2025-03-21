/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(i2c_cadence, CONFIG_I2C_LOG_LEVEL);

/* Register offsets for the I2C device. */
#define CDNS_I2C_CR_OFFSET		0x00 /* Control Register, RW */
#define CDNS_I2C_SR_OFFSET		0x04 /* Status Register, RO */
#define CDNS_I2C_ADDR_OFFSET		0x08 /* I2C Address Register, RW */
#define CDNS_I2C_DATA_OFFSET		0x0C /* I2C Data Register, RW */
#define CDNS_I2C_ISR_OFFSET		0x10 /* IRQ Status Register, RW */
#define CDNS_I2C_XFER_SIZE_OFFSET	0x14 /* Transfer Size Register, RW */
#define CDNS_I2C_TIME_OUT_OFFSET	0x1C /* Time Out Register, RW */
#define CDNS_I2C_IMR_OFFSET		0x20 /* IRQ Mask Register, RO */
#define CDNS_I2C_IER_OFFSET		0x24 /* IRQ Enable Register, WO */
#define CDNS_I2C_IDR_OFFSET		0x28 /* IRQ Disable Register, WO */
#define CDNS_I2C_GFR_OFFSET		0x2C /* Glitch Filter Register, RW */

/* Control Register Bit mask definitions */
#define CDNS_I2C_CR_HOLD		BIT(4) /* Hold the I2C Bus */
#define CDNS_I2C_CR_ACK_EN		BIT(3) /* Enables or disables acknowledgment */
#define CDNS_I2C_CR_NEA			BIT(2) /* No Extended addressing */
#define CDNS_I2C_CR_MS			BIT(1) /* 0 = Slave Mode, 1 = Master Mode */
#define CDNS_I2C_CR_RW			BIT(0) /* Transfer Dir: 0 = Transmitter, 1 = Receiver */
#define CDNS_I2C_CR_CLR_FIFO		BIT(6) /* Clears the FIFO on initialization */

/* Master Enable Mask */
#define CDNS_I2C_CR_MASTER_EN_MASK	(CDNS_I2C_CR_ACK_EN | \
					 CDNS_I2C_CR_NEA | \
					 CDNS_I2C_CR_MS)

/* Dividers for clock generation */
#define CDNS_I2C_CR_DIVA_SHIFT		14U
#define CDNS_I2C_CR_DIVA_MASK		((uint32_t)3U << CDNS_I2C_CR_DIVA_SHIFT)
#define CDNS_I2C_CR_DIVB_SHIFT		8U
#define CDNS_I2C_CR_DIVB_MASK		((uint32_t)0x3f << CDNS_I2C_CR_DIVB_SHIFT)

/* Status Register Bit mask definitions */
#define CDNS_I2C_SR_BA			BIT(8) /* Bus is available */
#define CDNS_I2C_SR_TXDV		BIT(6) /* Transmit data is valid */
#define CDNS_I2C_SR_RXDV		BIT(5) /* Received data is valid */
#define CDNS_I2C_SR_RXRW		BIT(3) /* Read or Write operation */

/*
 * I2C Address Register Bit mask definitions
 * Normal addressing mode uses [6:0] bits.
 * Extended addressing mode uses [9:0] bits.
 * A write access to this register always initiates a transfer if the I2C is in master mode.
 */
#define CDNS_I2C_ADDR_MASK		0x000003FFU /* I2C Address Mask */

/*
 * I2C Interrupt Registers Bit mask definitions
 * All the four interrupt registers (Status/Mask/Enable/Disable) have the same bit definitions.
 */
#define CDNS_I2C_IXR_ARB_LOST		BIT(9) /* Arbitration Lost Interrupt */
#define CDNS_I2C_IXR_RX_UNF		BIT(7) /* RX FIFO Underflow Interrupt */
#define CDNS_I2C_IXR_TX_OVF		BIT(6) /* TX FIFO Overflow Interrupt */
#define CDNS_I2C_IXR_RX_OVF		BIT(5) /* RX FIFO Overflow Interrupt */
#define CDNS_I2C_IXR_SLV_RDY		BIT(4) /* Slave Ready Interrupt */
#define CDNS_I2C_IXR_TO			BIT(3) /* Timeout Interrupt */
#define CDNS_I2C_IXR_NACK		BIT(2) /* NACK Interrupt */
#define CDNS_I2C_IXR_DATA		BIT(1) /* Data Interrupt */
#define CDNS_I2C_IXR_COMP		BIT(0) /* Transfer Complete Interrupt */

/* All Interrupt Mask */
#define CDNS_I2C_IXR_ALL_INTR_MASK	(CDNS_I2C_IXR_ARB_LOST | \
					 CDNS_I2C_IXR_RX_UNF | \
					 CDNS_I2C_IXR_TX_OVF | \
					 CDNS_I2C_IXR_RX_OVF | \
					 CDNS_I2C_IXR_SLV_RDY | \
					 CDNS_I2C_IXR_TO | \
					 CDNS_I2C_IXR_NACK | \
					 CDNS_I2C_IXR_DATA | \
					 CDNS_I2C_IXR_COMP)

/* Error Interrupt Mask */
#define CDNS_I2C_IXR_ERR_INTR_MASK	(CDNS_I2C_IXR_ARB_LOST | \
					 CDNS_I2C_IXR_RX_UNF | \
					 CDNS_I2C_IXR_TX_OVF | \
					 CDNS_I2C_IXR_RX_OVF | \
					 CDNS_I2C_IXR_NACK)

/* Enabled Interrupt Mask */
#define CDNS_I2C_ENABLED_INTR_MASK	(CDNS_I2C_IXR_ARB_LOST | \
					 CDNS_I2C_IXR_RX_UNF | \
					 CDNS_I2C_IXR_TX_OVF | \
					 CDNS_I2C_IXR_RX_OVF | \
					 CDNS_I2C_IXR_NACK | \
					 CDNS_I2C_IXR_DATA | \
					 CDNS_I2C_IXR_COMP)

/* System clock frequency for I2C ticks */
#define CDNS_I2C_TICKS_PER_SEC		CONFIG_SYS_CLOCK_TICKS_PER_SEC

/* Default timeout ticks for I2C operations */
#define CDNS_I2C_TIMEOUT_TICKS		CDNS_I2C_TICKS_PER_SEC

#define CDNS_I2C_MAX_TRANSFER_SIZE	255U /* Maximum transfer size for I2C data */

/* Default transfer size */
#define CDNS_I2C_TRANSFER_SIZE_DEFAULT	(CDNS_I2C_MAX_TRANSFER_SIZE - 3U)

/* Maximum dividers for I2C clock */
#define CDNS_I2C_DIVA_MAX		4U
#define CDNS_I2C_DIVB_MAX		64U
#define CDNS_I2C_CLK_DIV_FACTOR		22U

#define CDNS_I2C_TIMEOUT_MAX		  0xFFU /* Maximum value for Timeout Register */
#define CDNS_I2C_POLL_US		100000U /* Polling interval in microseconds */
#define CDNS_I2C_TIMEOUT_US		500000U /* Timeout value for I2C operations */

/* Event flag for I2C transfer completion */
#define I2C_XFER_COMPLETION_EVENT	BIT(0)

/**
 * struct cdns_i2c_config - Cadence I2C device private constant structure
 * @irq_config_func: function pointer to configure I2C IRQ
 */
struct cdns_i2c_config {
	void (*irq_config_func)(void);
};

/**
 * struct cdns_i2c_data - Cadence I2C device private data structure
 * @membase: Base address of the I2C device.
 * @ctrl_reg: Cached value of the control register.
 * @input_clk: Input clock to I2C controller.
 * @i2c_clk: Actual I2C clock speed.
 * @fifo_depth: The depth of the transfer FIFO.
 * @transfer_size: The maximum number of bytes in one transfer.
 * @bus_hold_flag: Flag used in repeated start for clearing HOLD bit.
 * @xfer_done: Transfer complete event.
 * @err_status: Error status in Interrupt Status Register.
 * @p_msg: Message pointer for I2C communication.
 * @p_send_buf:	Pointer to transmit buffer.
 * @p_recv_buf:	Pointer to receive buffer.
 * @send_count:	Number of bytes still expected to send.
 * @recv_count:	Number of bytes still expected to receive.
 * @curr_recv_count: Number of bytes to be received in current transfer.
 * @bus_mutex: Mutex for bus access synchronization
 */
struct cdns_i2c_data {
	mem_addr_t membase;
	uint32_t ctrl_reg;
	uint32_t input_clk;
	uint32_t i2c_clk;
	uint32_t fifo_depth;
	uint32_t transfer_size;
	uint32_t bus_hold_flag;

	struct k_event xfer_done;
	uint32_t err_status;
	struct i2c_msg *p_msg;
	uint8_t *p_send_buf;
	uint8_t *p_recv_buf;
	uint32_t send_count;
	uint32_t recv_count;
	uint32_t curr_recv_count;

	struct k_mutex bus_mutex;
};

/**
 * cdns_i2c_writereg - Write a 32-bit value to a specific offset in the I2C register space.
 * @i2c_bus: Pointer to the I2C data structure.
 * @value: The 32-bit value to write to the register.
 * @offset: The offset in the I2C register space where the value will be written.
 */
static inline void cdns_i2c_writereg(const struct cdns_i2c_data *i2c_bus,
				     uint32_t value, uintptr_t offset)
{
	uintptr_t reg_address = (uintptr_t)(i2c_bus->membase) + offset;

	sys_write32(value, reg_address);
}

/**
 * cdns_i2c_readreg - Read a 32-bit value from a specific offset in the I2C register space.
 * @i2c_bus: Pointer to the I2C data structure.
 * @offset: The offset in the I2C register space from which the value will be read.
 *
 * Return: The 32-bit value read from the register.
 */
static inline uint32_t cdns_i2c_readreg(const struct cdns_i2c_data *i2c_bus, uintptr_t offset)
{
	uintptr_t reg_address = (uintptr_t)(i2c_bus->membase) + offset;

	return sys_read32(reg_address);
}

/**
 * cdns_i2c_enable_peripheral - Enable the Cadence I2C controller
 * @i2c_bus: Pointer to the device's private data structure for the I2C controller
 */
static void cdns_i2c_enable_peripheral(struct cdns_i2c_data *i2c_bus)
{
	cdns_i2c_writereg(i2c_bus, i2c_bus->ctrl_reg, CDNS_I2C_CR_OFFSET);

	/*
	 * Cadence I2C controller has a bug causing invalid reads after a timeout
	 * in master receiver mode. While the timeout feature is disabled,
	 * writing the max value to the timeout register reduces the issue.
	 */
	cdns_i2c_writereg(i2c_bus, CDNS_I2C_TIMEOUT_MAX, CDNS_I2C_TIME_OUT_OFFSET);
}

/**
 * cdns_i2c_calc_divs - Calculate clock dividers for I2C frequency
 * @f: Pointer to the I2C target frequency (input/output)
 * @input_clk: Input clock frequency in Hz
 * @a: Pointer to the first divider (output)
 * @b: Pointer to the second divider (output)
 *
 * On entry, @f holds the target I2C frequency. On exit, @f will hold the
 * actual I2C frequency generated by the calculated dividers.
 *
 * Return: 0 on success, negative errno otherwise.
 */
static int32_t cdns_i2c_calc_divs(uint32_t *f, uint32_t input_clk,
				  uint32_t *a, uint32_t *b)
{
	uint32_t fscl = *f;
	uint32_t best_fscl = *f;
	uint32_t actual_fscl, temp;
	uint32_t div_a, div_b;
	uint32_t calc_div_a = 0, calc_div_b = 0;
	uint32_t last_error, current_error;
	int32_t ret = 0;

	/* calculate initial estimate for divisor_a and divisor_b */
	temp = input_clk / (CDNS_I2C_CLK_DIV_FACTOR * fscl);

	/* Check if the calculated value is out of range */
	if ((temp == 0U) || (temp > (CDNS_I2C_DIVA_MAX * CDNS_I2C_DIVB_MAX))) {
		ret = -EINVAL;
		goto out;
	}

	/* Initialize the last error to a large value (no error yet) */
	last_error = UINT32_MAX;

	/* Iterate over possible values for divisor_a */
	for (div_a = 0; div_a < CDNS_I2C_DIVA_MAX; div_a++) {
		/* Calculate the corresponding divisor_b for this div_a */
		div_b = DIV_ROUND_UP(input_clk, CDNS_I2C_CLK_DIV_FACTOR * fscl * (div_a + 1U));

		/* Skip invalid values of div_b */
		if ((div_b < 1U) || (div_b > CDNS_I2C_DIVB_MAX)) {
			continue;
		}

		/* Adjust div_b for zero-based indexing */
		div_b--;

		/* Calculate the actual fscl based on the current divisors */
		actual_fscl = input_clk / (CDNS_I2C_CLK_DIV_FACTOR * (div_a + 1U) * (div_b + 1U));

		/* Skip if the actual fscl exceeds the target fscl */
		if (actual_fscl > fscl) {
			continue;
		}

		/* Calculate the error between the target fscl and the actual fscl */
		current_error = fscl - actual_fscl;

		/* Update the best divisors if a smaller error is found */
		if (last_error > current_error) {
			calc_div_a = div_a;
			calc_div_b = div_b;
			best_fscl = actual_fscl;
			last_error = current_error;
		}
	}

	/* Set the output values */
	*a = calc_div_a;
	*b = calc_div_b;
	*f = best_fscl;

out:
	return ret;
}

/**
 * cdns_i2c_setclk - Set the serial clock rate for the I2C device
 * @i2c_bus: Pointer to the I2C data structure
 * @req_i2c_speed: requested I2C clock frequency in Hz
 *
 * This function sets the serial clock rate for the I2C device by configuring
 * the clock divisors in the device's control register. The device must be idle
 * (i.e., not actively transferring data) before calling this function.
 *
 * The clock rate is determined by the following formula:
 *	Fscl = Fpclk / (22 * (divisor_a + 1) * (divisor_b + 1))
 * Where:
 *	- Fscl is the desired I2C clock rate
 *	- Fpclk is the input clock frequency
 *	- divisor_a and divisor_b are the calculated divisors to achieve the desired clock rate
 *
 * The serial clock rate cannot exceed the input clock divided by 22. Common I2C clock
 * rates are 100 KHz and 400 KHz.
 *
 * Return: 0 on success, negative error code on failure
 */
static int32_t cdns_i2c_setclk(struct cdns_i2c_data *i2c_bus, uint32_t req_i2c_speed)
{
	uint32_t div_a, div_b;
	uint32_t ctrl_reg;
	int32_t ret = 0;
	uint32_t fscl = req_i2c_speed;

	/* Calculate the divider values */
	ret = cdns_i2c_calc_divs(&fscl, i2c_bus->input_clk, &div_a, &div_b);
	if (ret != 0) {
		goto out;
	}
	i2c_bus->i2c_clk = fscl; /* Update true SCL value */

	/* Set new divider values in the control register */
	ctrl_reg = i2c_bus->ctrl_reg;
	ctrl_reg &= ~(CDNS_I2C_CR_DIVA_MASK | CDNS_I2C_CR_DIVB_MASK);
	ctrl_reg |= ((div_a << CDNS_I2C_CR_DIVA_SHIFT) |
		     (div_b << CDNS_I2C_CR_DIVB_SHIFT));
	i2c_bus->ctrl_reg = ctrl_reg;
	cdns_i2c_writereg(i2c_bus, ctrl_reg, CDNS_I2C_CR_OFFSET);

out:
	return ret;
}

/**
 * cdns_i2c_configure - Configures the I2C bus speed and initializes the I2C peripheral
 * @dev: Pointer to the device structure representing the I2C bus
 * @dev_config: Configuration value containing the desired I2C bus speed
 *
 * This function configures the I2C bus speed based on the value provided in @dev_config
 * It then sets the appropriate clock for the I2C bus, verifies the clock is valid, and
 * initializes the I2C peripheral. The configuration is saved to the device's data structure.
 *
 * Return: 0 on success, negative error value on failure
 */
static int32_t cdns_i2c_configure(const struct device *dev, uint32_t dev_config)
{
	int32_t ret = 0;
	struct cdns_i2c_data *i2c_bus = (struct cdns_i2c_data *)dev->data;
	uint32_t i2c_speed = 0U;

	(void)k_mutex_lock(&i2c_bus->bus_mutex, K_FOREVER);

	/* Check requested I2C Speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_speed = I2C_BITRATE_STANDARD; /* 100 KHz */
		break;
	case I2C_SPEED_FAST:
		i2c_speed = I2C_BITRATE_FAST; /* 400 KHz */
		break;
	case I2C_SPEED_FAST_PLUS:
		i2c_speed = I2C_BITRATE_FAST_PLUS; /* 1 MHz */
		break;
	default:
		LOG_ERR("Unsupported I2C speed requested: %u", i2c_speed);
		ret = -ERANGE;
		goto out;
	}

	/* Set I2C Speed (SCL frequency) */
	ret = cdns_i2c_setclk(i2c_bus, i2c_speed);
	if (ret != 0) {
		LOG_ERR("Invalid SCL clock: %u Hz", i2c_speed);
		ret = -EIO;
		goto out;
	}

	/* Enable the I2C peripheral */
	i2c_bus->ctrl_reg |= CDNS_I2C_CR_MASTER_EN_MASK;
	cdns_i2c_enable_peripheral(i2c_bus);

out:
	(void)k_mutex_unlock(&i2c_bus->bus_mutex);

	return ret;
}

/**
 * cdns_i2c_get_config - Retrieve the current I2C configuration.
 * @dev: Pointer to the device structure.
 * @dev_config: Pointer to a variable where the configuration will be stored.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int32_t cdns_i2c_get_config(const struct device *dev, uint32_t *dev_config)
{
	int32_t ret = 0;
	struct cdns_i2c_data *i2c_bus = (struct cdns_i2c_data *)dev->data;
	uint32_t bus_speed = i2c_bus->i2c_clk;
	uint32_t speed_cfg = 0U;

	/* Retrieve Speed configuration from Actual Bus Speed */
	if ((bus_speed > 0U) && (bus_speed <= I2C_BITRATE_STANDARD)) {
		speed_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD);
	} else if ((bus_speed > I2C_BITRATE_STANDARD) &&
		   (bus_speed <= I2C_BITRATE_FAST)) {
		speed_cfg = I2C_SPEED_SET(I2C_SPEED_FAST);
	} else if ((bus_speed > I2C_BITRATE_FAST) &&
		   (bus_speed <= I2C_BITRATE_FAST_PLUS)) {
		speed_cfg = I2C_SPEED_SET(I2C_SPEED_FAST_PLUS);
	} else {
		ret = -ERANGE;
		goto out;
	}

	/* Return current configuration */
	*dev_config = (speed_cfg | I2C_MODE_CONTROLLER);

out:
	return ret;
}

/**
 * cdns_i2c_clear_bus_hold - Clear bus hold bit in the controller's register
 * @i2c_bus: Pointer to the I2C controller driver data structure
 */
static void cdns_i2c_clear_bus_hold(struct cdns_i2c_data *i2c_bus)
{
	uint32_t reg = cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);

	if ((reg & CDNS_I2C_CR_HOLD) == CDNS_I2C_CR_HOLD) {
		cdns_i2c_writereg(i2c_bus, reg & ~CDNS_I2C_CR_HOLD, CDNS_I2C_CR_OFFSET);
	}
}

/**
 * cdns_is_fifo_hold_quirk - Check if the FIFO hold quirk is triggered for I2C
 * @i2c_bus: Pointer to the I2C controller driver data structure
 * @hold_wrkaround: Boolean indicating if hold workarounds should be applied
 *
 * Return: True if the quirk condition is met, false otherwise.
 */
static inline bool cdns_is_fifo_hold_quirk(const struct cdns_i2c_data *i2c_bus,
					   bool hold_wrkaround)
{
	return (hold_wrkaround && (i2c_bus->curr_recv_count == (i2c_bus->fifo_depth + 1U)));
}

/**
 * cdns_i2c_master_handle_receive_interrupt - Handles I2C master receive interrupts
 * @i2c_bus: Pointer to the I2C data structure
 * @isr_status: Interrupt status, indicating the cause of the interrupt
 */
static void cdns_i2c_master_handle_receive_interrupt(struct cdns_i2c_data *i2c_bus,
						     uint32_t isr_status)
{
	uint32_t transfer_size;
	uint32_t xfer_size;

	/* Handle reception interrupt (data available or transfer complete) */
	if (((isr_status & CDNS_I2C_IXR_COMP) == 0U) && ((isr_status & CDNS_I2C_IXR_DATA) == 0U)) {
		return;
	}

	/* Receiving Data: Keep reading as long as data is available */
	while ((cdns_i2c_readreg(i2c_bus, CDNS_I2C_SR_OFFSET) & CDNS_I2C_SR_RXDV) != 0U) {
		/* Ensure there's space to store received data */
		if (i2c_bus->recv_count > 0U) {
			*(i2c_bus->p_recv_buf) = (uint8_t)cdns_i2c_readreg(i2c_bus,
									   CDNS_I2C_DATA_OFFSET);
			i2c_bus->p_recv_buf++;
			i2c_bus->recv_count--;
			i2c_bus->curr_recv_count--;
		} else {
			/* Handle receive buffer overflow or unexpected condition */
			LOG_ERR("I2C receive buffer overflow. Transfer aborted!");
			i2c_bus->err_status |= CDNS_I2C_IXR_TO;
			break;
		}

		/* Handle issues with receiving more data than expected */
		if (cdns_is_fifo_hold_quirk(i2c_bus,
					i2c_bus->recv_count > i2c_bus->curr_recv_count)) {
			break;
		}
	}

	/* Workaround for large data receive in case of FIFO space issues */
	if (cdns_is_fifo_hold_quirk(i2c_bus, i2c_bus->recv_count > i2c_bus->curr_recv_count)) {
		transfer_size = i2c_bus->recv_count - i2c_bus->fifo_depth;

		if (transfer_size > i2c_bus->transfer_size) {
			xfer_size = i2c_bus->transfer_size;
		} else {
			xfer_size = transfer_size;
		}

		/* Busy-wait until FIFO has space for more data */
		while (cdns_i2c_readreg(i2c_bus, CDNS_I2C_XFER_SIZE_OFFSET) !=
		       (i2c_bus->curr_recv_count - i2c_bus->fifo_depth)) {
		}

		/* Update the transfer size for the next batch of data */
		cdns_i2c_writereg(i2c_bus, xfer_size, CDNS_I2C_XFER_SIZE_OFFSET);
		i2c_bus->curr_recv_count = xfer_size + i2c_bus->fifo_depth;
	}

	/* Complete transfer if all data has been received and no more data is expected */
	if (((isr_status & CDNS_I2C_IXR_COMP) == CDNS_I2C_IXR_COMP) &&
	    (i2c_bus->recv_count == 0U)) {
		/* Release bus hold if no longer needed */
		if (i2c_bus->bus_hold_flag == 0U) {
			cdns_i2c_clear_bus_hold(i2c_bus);
		}
		/* Notify completion of the transfer */
		(void)k_event_post(&i2c_bus->xfer_done, I2C_XFER_COMPLETION_EVENT);
	}
}

/**
 * cdns_i2c_master_handle_transmit_interrupt - Handles I2C master transmit interrupts
 * @i2c_bus: Pointer to the I2C data structure
 * @isr_status: Interrupt status, indicating the cause of the interrupt
 */
static void cdns_i2c_master_handle_transmit_interrupt(struct cdns_i2c_data *i2c_bus,
						      uint32_t isr_status)
{
	uint32_t avail_bytes;
	uint32_t bytes_to_send;

	/* Handle transmission interrupt (data sent or transfer complete) */
	if ((isr_status & CDNS_I2C_IXR_COMP) == 0U) {
		return;
	}

	/* Sending data: Check if there is any data left to send */
	if (i2c_bus->send_count > 0U) {
		/* Calculate how many bytes can be sent based on FIFO availability */
		avail_bytes = i2c_bus->fifo_depth - cdns_i2c_readreg(i2c_bus,
								     CDNS_I2C_XFER_SIZE_OFFSET);

		if (i2c_bus->send_count > avail_bytes) {
			bytes_to_send = avail_bytes;
		} else {
			bytes_to_send = i2c_bus->send_count;
		}

		/* Write data to the I2C data register */
		while (bytes_to_send > 0U) {
			cdns_i2c_writereg(i2c_bus, *(i2c_bus->p_send_buf), CDNS_I2C_DATA_OFFSET);
			i2c_bus->p_send_buf++;
			i2c_bus->send_count--;
			bytes_to_send--;
		}
	} else {
		/* If there is no data to send, signal transfer completion */
		(void)k_event_post(&i2c_bus->xfer_done, I2C_XFER_COMPLETION_EVENT);
	}

	/* Clear bus hold if no more data is pending */
	if ((i2c_bus->send_count == 0U) && (i2c_bus->bus_hold_flag == 0U)) {
		cdns_i2c_clear_bus_hold(i2c_bus);
	}
}

/**
 * cdns_i2c_master_isr - Interrupt handler for the I2C device in master role
 * @i2c_bus: Pointer to I2C device private data structure
 *
 * This function handles various interrupt events including data received,
 * transfer complete, and error interrupts for the I2C master role.
 */
static void cdns_i2c_master_isr(struct cdns_i2c_data *i2c_bus)
{
	uint32_t isr_status;

	/* Read the interrupt status register */
	isr_status = cdns_i2c_readreg(i2c_bus, CDNS_I2C_ISR_OFFSET);

	/* Clear interrupt status */
	cdns_i2c_writereg(i2c_bus, isr_status, CDNS_I2C_ISR_OFFSET);

	/* Update the error status based on interrupt flags */
	i2c_bus->err_status = isr_status & CDNS_I2C_IXR_ERR_INTR_MASK;

	/* Handling NACK or arbitration lost interrupts */
	if ((isr_status & (CDNS_I2C_IXR_NACK | CDNS_I2C_IXR_ARB_LOST)) != 0U) {
		(void)k_event_post(&i2c_bus->xfer_done, I2C_XFER_COMPLETION_EVENT);
		return;
	}

	/* Handle reception interrupt */
	if (i2c_bus->p_recv_buf != NULL) {
		cdns_i2c_master_handle_receive_interrupt(i2c_bus, isr_status);
	}

	/* Handle transmission interrupt */
	if (i2c_bus->p_recv_buf == NULL) {
		cdns_i2c_master_handle_transmit_interrupt(i2c_bus, isr_status);
	}
}

/**
 * cdns_i2c_isr - Interrupt handler for the I2C controller
 * @dev: Pointer to I2C device
 */
static void cdns_i2c_isr(const struct device *dev)
{
	struct cdns_i2c_data *i2c_bus = (struct cdns_i2c_data *)dev->data;

	/* Handle the interrupt for master mode */
	cdns_i2c_master_isr(i2c_bus);
}

/**
 * cdns_i2c_mrecv - Prepare and start a master receive operation
 * @i2c_bus: Pointer to the I2C data structure
 * @msg_addr: The address of the slave device to receive data from
 */
static void cdns_i2c_mrecv(struct cdns_i2c_data *i2c_bus, uint16_t msg_addr)
{
	uint32_t ctrl_reg;
	uint32_t isr_status;
	bool hold_clear = false;
	uint32_t addr;

	/* Initialize the receive buffer and count */
	i2c_bus->p_recv_buf = i2c_bus->p_msg->buf;
	i2c_bus->recv_count = i2c_bus->p_msg->len;
	i2c_bus->curr_recv_count = i2c_bus->recv_count;

	/* Prepare controller for master receive mode and clear FIFO */
	ctrl_reg = cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);
	ctrl_reg |= CDNS_I2C_CR_RW | CDNS_I2C_CR_CLR_FIFO;

	/* Check if the message size exceeds FIFO depth, hold the bus if true */
	if (i2c_bus->recv_count > i2c_bus->fifo_depth) {
		ctrl_reg |= CDNS_I2C_CR_HOLD;
	}

	cdns_i2c_writereg(i2c_bus, ctrl_reg, CDNS_I2C_CR_OFFSET);

	/* Clear the interrupts in interrupt status register */
	isr_status = cdns_i2c_readreg(i2c_bus, CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(i2c_bus, isr_status, CDNS_I2C_ISR_OFFSET);

	/* Set transfer size register and enable interrupts */
	if ((i2c_bus->recv_count) > (i2c_bus->transfer_size)) {
		cdns_i2c_writereg(i2c_bus, i2c_bus->transfer_size,
				  CDNS_I2C_XFER_SIZE_OFFSET);
		i2c_bus->curr_recv_count = i2c_bus->transfer_size;
	} else {
		cdns_i2c_writereg(i2c_bus, i2c_bus->recv_count, CDNS_I2C_XFER_SIZE_OFFSET);
	}

	/* Determine whether to clear the hold bit based on conditions */
	if ((i2c_bus->bus_hold_flag == 0U) && (i2c_bus->recv_count <= i2c_bus->fifo_depth)) {
		if ((ctrl_reg & CDNS_I2C_CR_HOLD) != 0U) {
			hold_clear = true;
		}
	}

	/* Mask address and prepare for I2C communication */
	addr = msg_addr;
	addr &= CDNS_I2C_ADDR_MASK;

	/* Handle clearing of the hold bit */
	if (hold_clear) {
		ctrl_reg &= ~CDNS_I2C_CR_HOLD;
		ctrl_reg &= ~CDNS_I2C_CR_CLR_FIFO;

		/* Write the address and control register values */
		cdns_i2c_writereg(i2c_bus, addr, CDNS_I2C_ADDR_OFFSET);
		cdns_i2c_writereg(i2c_bus, ctrl_reg, CDNS_I2C_CR_OFFSET);
		/* Read back to ensure write completion */
		(void)cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);
	} else {
		/* Directly write the address if no need to clear the hold bit */
		cdns_i2c_writereg(i2c_bus, addr, CDNS_I2C_ADDR_OFFSET);
	}

	/* Enable interrupts */
	cdns_i2c_writereg(i2c_bus, CDNS_I2C_ENABLED_INTR_MASK, CDNS_I2C_IER_OFFSET);
}

/**
 * cdns_i2c_msend - Prepare and start a master send operation
 * @i2c_bus: Pointer to the I2C data structure
 * @msg_addr: I2C address of the slave to communicate with
 */
static void cdns_i2c_msend(struct cdns_i2c_data *i2c_bus, uint16_t msg_addr)
{
	uint32_t avail_bytes;
	uint32_t bytes_to_send;
	uint32_t ctrl_reg;
	uint32_t isr_status;

	/* Initialize send buffer and update send count */
	i2c_bus->p_recv_buf = NULL;
	i2c_bus->p_send_buf = i2c_bus->p_msg->buf;
	i2c_bus->send_count = i2c_bus->p_msg->len;

	/* Configure the controller in Master transmit mode and clear FIFO. */
	ctrl_reg = cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);
	ctrl_reg &= ~CDNS_I2C_CR_RW;
	ctrl_reg |= CDNS_I2C_CR_CLR_FIFO;

	/* Check if the message size exceeds FIFO depth, hold the bus if true */
	if (i2c_bus->send_count > i2c_bus->fifo_depth) {
		ctrl_reg |= CDNS_I2C_CR_HOLD;
	}
	cdns_i2c_writereg(i2c_bus, ctrl_reg, CDNS_I2C_CR_OFFSET);

	/* Clear any previous interrupt flags */
	isr_status = cdns_i2c_readreg(i2c_bus, CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(i2c_bus, isr_status, CDNS_I2C_ISR_OFFSET);

	/* Calculate available FIFO space and determine how many bytes to send */
	avail_bytes = i2c_bus->fifo_depth - cdns_i2c_readreg(i2c_bus, CDNS_I2C_XFER_SIZE_OFFSET);
	bytes_to_send = (i2c_bus->send_count > avail_bytes) ? avail_bytes : i2c_bus->send_count;

	/* Send data to FIFO until all bytes are transmitted */
	while (bytes_to_send > 0U) {
		cdns_i2c_writereg(i2c_bus, (*(i2c_bus->p_send_buf)), CDNS_I2C_DATA_OFFSET);
		(i2c_bus->p_send_buf)++;
		i2c_bus->send_count--;
		bytes_to_send--;
	}

	/* Clear the 'hold bus' flag if there's no more data and it's the last message */
	if ((i2c_bus->bus_hold_flag == 0U) && (i2c_bus->send_count == 0U)) {
		cdns_i2c_clear_bus_hold(i2c_bus);
	}

	/* Set the slave address to trigger operation. */
	cdns_i2c_writereg(i2c_bus, ((uint32_t)msg_addr & CDNS_I2C_ADDR_MASK),
				   CDNS_I2C_ADDR_OFFSET);

	/* Enable interrupts after data transmission starts */
	cdns_i2c_writereg(i2c_bus, CDNS_I2C_ENABLED_INTR_MASK, CDNS_I2C_IER_OFFSET);
}

/**
 * cdns_i2c_master_reset - Reset the I2C master interface
 * @i2c_bus: Pointer to the i2c driver instance
 *
 * This function performs a full reset of the I2C master interface
 * The reset ensures that the interface is returned to a known idle state.
 */
static void cdns_i2c_master_reset(struct cdns_i2c_data *i2c_bus)
{
	uint32_t regval;

	/* Disable the interrupts */
	cdns_i2c_writereg(i2c_bus, CDNS_I2C_IXR_ALL_INTR_MASK, CDNS_I2C_IDR_OFFSET);

	/* Clear the hold bit and flush FIFOs */
	regval = cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);
	regval &= ~CDNS_I2C_CR_HOLD;
	regval |= CDNS_I2C_CR_CLR_FIFO;
	cdns_i2c_writereg(i2c_bus, regval, CDNS_I2C_CR_OFFSET);

	/* Reset transfer count register to zero */
	cdns_i2c_writereg(i2c_bus, 0, CDNS_I2C_XFER_SIZE_OFFSET);

	/* Clear the interrupt status register */
	regval = cdns_i2c_readreg(i2c_bus, CDNS_I2C_ISR_OFFSET);
	cdns_i2c_writereg(i2c_bus, regval, CDNS_I2C_ISR_OFFSET);

	/* Clear the status register */
	regval = cdns_i2c_readreg(i2c_bus, CDNS_I2C_SR_OFFSET);
	cdns_i2c_writereg(i2c_bus, regval, CDNS_I2C_SR_OFFSET);
}

/**
 * cdns_i2c_process_msg - Processes an I2C message on the specified I2C bus
 * @i2c_bus: Pointer to the I2C data structure
 * @msg: Pointer to the I2C message to be processed
 * @addr: The 7-bit or 10-bit I2C address of the slave device
 *
 * Return: 0 on success, negative error code on failure.
 */
static int32_t cdns_i2c_process_msg(struct cdns_i2c_data *i2c_bus, struct i2c_msg *msg,
				    uint16_t addr)
{
	int32_t ret = 0;
	uint32_t reg;
	k_timeout_t msg_timeout;
	uint32_t events;

	/* Initialize message processing state */
	i2c_bus->p_msg = msg;
	i2c_bus->err_status = 0U;
	(void)k_event_clear(&i2c_bus->xfer_done, (uint32_t)I2C_XFER_COMPLETION_EVENT);

	/* Handle 10-bit addressing mode */
	reg = cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);
	if ((msg->flags & I2C_MSG_ADDR_10_BITS) != 0U) {
		/* Enable 10-bit address mode if not already enabled */
		if ((reg & CDNS_I2C_CR_NEA) == CDNS_I2C_CR_NEA) {
			cdns_i2c_writereg(i2c_bus,
					reg & ~CDNS_I2C_CR_NEA,
					CDNS_I2C_CR_OFFSET);
		}
	} else {
		/* Disable 10-bit address mode if currently enabled */
		if ((reg & CDNS_I2C_CR_NEA) == 0U) {
			cdns_i2c_writereg(i2c_bus,
					reg | CDNS_I2C_CR_NEA,
					CDNS_I2C_CR_OFFSET);
		}
	}

	/* Handle read/write flag and perform the appropriate action */
	if ((msg->flags & I2C_MSG_READ) != 0U) {
		cdns_i2c_mrecv(i2c_bus, addr); /* Receive data */
	} else {
		cdns_i2c_msend(i2c_bus, addr); /* Send data */
	}

	/* Calculate the minimal timeout based on message length */
	msg_timeout.ticks = (((k_ticks_t)(msg->len) * 8)*(CDNS_I2C_TICKS_PER_SEC)) /
			     ((k_ticks_t)(i2c_bus->i2c_clk));
	msg_timeout.ticks += (CDNS_I2C_TICKS_PER_SEC / 2);
	if (msg_timeout.ticks < CDNS_I2C_TIMEOUT_TICKS) {
		msg_timeout.ticks = CDNS_I2C_TIMEOUT_TICKS;
	}

	/* Wait for the completion signal or timeout */
	events = k_event_wait(&i2c_bus->xfer_done, (uint32_t)I2C_XFER_COMPLETION_EVENT,
			      false, msg_timeout);
	if ((events & I2C_XFER_COMPLETION_EVENT) == 0U) {
		/* Timeout occurred, reset the master */
		cdns_i2c_master_reset(i2c_bus);
		ret = -ETIMEDOUT;
		goto out;
	}

	/* Disable interrupt masking for the current transfer */
	cdns_i2c_writereg(i2c_bus, CDNS_I2C_IXR_ALL_INTR_MASK, CDNS_I2C_IDR_OFFSET);

	/* If it is bus arbitration error, try again */
	if ((i2c_bus->err_status & CDNS_I2C_IXR_ARB_LOST) == CDNS_I2C_IXR_ARB_LOST) {
		ret = -EAGAIN;
	}

out:
	return ret;
}

/**
 * cdns_i2c_wait_for_bus_free - Wait for the I2C bus to become free.
 * @i2c_bus: Pointer to the I2C data structure that holds bus state information.
 * @timeout_us: Maximum time (in microseconds) to wait for the bus to become free.
 *
 * This function waits for the I2C bus to become idle. It checks the bus state
 * register periodically until the bus is free or the timeout occurs.
 *
 * Return: true if the bus is free within the timeout, false otherwise.
 */
static bool cdns_i2c_wait_for_bus_free(struct cdns_i2c_data *i2c_bus, uint32_t timeout_us)
{
	bool ret_flag = false;
	uint32_t reg;

	/* Poll until the bus is free or the timeout is reached */
	while (timeout_us > 0U) {
		reg = cdns_i2c_readreg(i2c_bus, CDNS_I2C_SR_OFFSET);
		if ((reg & CDNS_I2C_SR_BA) == 0U) {
			/* Bus Available (BA) bit is cleared, the bus is free */
			ret_flag = true;
			break;
		}

		/* Wait for a small period before checking again */
		(void)k_usleep((int32_t)CDNS_I2C_POLL_US);
		timeout_us -= CDNS_I2C_POLL_US;
	}

	if (timeout_us == 0U) {
		/* Timeout reached, bus not available */
		ret_flag = false;
	}

	return ret_flag;
}

/**
 * cdns_i2c_master_handle_repeated_start - Handle repeated start during I2C master transfer
 * @i2c_bus: Pointer to the I2C data structure that holds bus state information
 * @msgs: Array of I2C messages to be processed
 * @num_msgs: Number of messages in the @msgs array
 *
 * Return: 0 on success
 */
static int32_t cdns_i2c_master_handle_repeated_start(struct cdns_i2c_data *i2c_bus,
						     struct i2c_msg *msgs, uint8_t num_msgs)
{
	uint32_t reg;
	(void)msgs;
	(void)num_msgs;

	/* Set the hold flag and register */
	i2c_bus->bus_hold_flag = 1;
	reg = cdns_i2c_readreg(i2c_bus, CDNS_I2C_CR_OFFSET);
	reg |= CDNS_I2C_CR_HOLD;
	cdns_i2c_writereg(i2c_bus, reg, CDNS_I2C_CR_OFFSET);

	return 0;
}

/**
 * cdns_i2c_master_handle_transfer_error - Handle errors during I2C master transfer.
 * @i2c_bus: Pointer to the I2C data structure
 *
 * Return: -EIO or -ENXIO.
 */
static int32_t cdns_i2c_master_handle_transfer_error(struct cdns_i2c_data *i2c_bus)
{
	int32_t ret;

	/* Perform a reset of the I2C master to clear the error condition */
	cdns_i2c_master_reset(i2c_bus);

	if ((i2c_bus->err_status & CDNS_I2C_IXR_NACK) != 0U) {
		ret = -ENXIO; /* No device found (NACK) */
	} else {
		ret = -EIO; /* General I/O error */
	}

	return ret;
}

/**
 * cdns_i2c_master_transfer - Performs an I2C master transfer using the Cadence I2C controller.
 * @dev: Pointer to the device structure representing the I2C controller.
 * @msgs: Array of I2C message structures representing the messages to be sent/received.
 * @num_msgs: Number of messages in the msgs array.
 * @addr: The 7-bit or 10-bit I2C address of the slave device.
 *
 * Return: 0 on success, negative error code on failure.
 */
static int32_t cdns_i2c_master_transfer(const struct device *dev, struct i2c_msg *msgs,
					uint8_t num_msgs, uint16_t addr)
{
	struct cdns_i2c_data *i2c_bus = (struct cdns_i2c_data *)dev->data;
	int32_t ret = 0;
	uint32_t count;
	struct i2c_msg *msg_ptr = msgs;

	(void)k_mutex_lock(&i2c_bus->bus_mutex, K_FOREVER);

	/* Wait for the bus to be free */
	if (cdns_i2c_wait_for_bus_free(i2c_bus, CDNS_I2C_TIMEOUT_US) == false) {
		ret = -EAGAIN;
		goto out;
	}

	/* Handle repeated start for multiple messages */
	if (num_msgs > 1U) {
		ret = cdns_i2c_master_handle_repeated_start(i2c_bus, msgs, num_msgs);
		if (ret != 0) {
			goto out;
		}
	}

	/* Process each message individually */
	for (count = 0; count < num_msgs; count++) {
		/* Reset hold flag for the last message */
		if (count == ((uint32_t)num_msgs - 1U)) {
			i2c_bus->bus_hold_flag = 0;
		}

		/* Process the current message */
		ret = cdns_i2c_process_msg(i2c_bus, msg_ptr, addr);
		if (ret != 0) {
			goto out;
		}

		/* Handle any errors during the transfer */
		if ((i2c_bus->err_status) != 0U) {
			ret = cdns_i2c_master_handle_transfer_error(i2c_bus);
			if (ret != 0) {
				goto out;
			}
		}

		msg_ptr++;
	}

out:
	(void)k_mutex_unlock(&i2c_bus->bus_mutex);

	return ret;
}

/**
 * cdns_i2c_init - Initialize the Cadence I2C controller
 * @dev: Pointer to the device
 *
 * Return: 0 on success, negative error code on failure.
 */
static int32_t cdns_i2c_init(const struct device *dev)
{
	const struct cdns_i2c_config *config = (const struct cdns_i2c_config *)dev->config;
	struct cdns_i2c_data *i2c_bus = (struct cdns_i2c_data *)dev->data;
	int32_t ret;

	(void)k_mutex_init(&i2c_bus->bus_mutex);
	k_event_init(&i2c_bus->xfer_done);

	/* Configure the control reg flags, transfer size */
	i2c_bus->ctrl_reg = CDNS_I2C_CR_MASTER_EN_MASK;
	i2c_bus->transfer_size = CDNS_I2C_TRANSFER_SIZE_DEFAULT;

	/* Set the I2C clock frequency */
	ret = cdns_i2c_setclk(i2c_bus, i2c_bus->i2c_clk);
	if (ret != 0) {
		LOG_ERR("Invalid SCL clock: %u Hz", i2c_bus->i2c_clk);
		ret = -EINVAL;
		goto out;
	}

	/* Configure IRQ */
	config->irq_config_func();

	/* Enable the I2C peripheral */
	cdns_i2c_enable_peripheral(i2c_bus);

	LOG_INF("%u KHz mmio %08lx", i2c_bus->i2c_clk/1000U, i2c_bus->membase);

out:
	return ret;
}

/* I2C driver API structure for the Cadence I2C controller */
static DEVICE_API(i2c, cdns_i2c_driver_api) = {
	.configure = cdns_i2c_configure,
	.get_config = cdns_i2c_get_config,
	.transfer = cdns_i2c_master_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define CADENCE_I2C_INIT(n, compat)							\
	static void cdns_i2c_config_func_##compat##_##n(void);				\
											\
	static const struct cdns_i2c_config cdns_i2c_config_##compat##_##n = {		\
		.irq_config_func = cdns_i2c_config_func_##compat##_##n,			\
	};										\
											\
	static struct cdns_i2c_data cdns_i2c_data_##compat##_##n = {			\
		.membase = DT_INST_REG_ADDR(n),						\
		.input_clk = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),	\
		.i2c_clk = DT_INST_PROP(n, clock_frequency),				\
		.fifo_depth = DT_INST_PROP(n, fifo_depth),				\
	};										\
											\
	I2C_DEVICE_DT_INST_DEFINE(n, cdns_i2c_init, NULL,				\
				&cdns_i2c_data_##compat##_##n,				\
				&cdns_i2c_config_##compat##_##n, POST_KERNEL,		\
				CONFIG_I2C_INIT_PRIORITY, &cdns_i2c_driver_api);	\
											\
	static void cdns_i2c_config_func_##compat##_##n(void)				\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), cdns_i2c_isr,	\
			    DEVICE_DT_INST_GET(n), 0);					\
											\
		irq_enable(DT_INST_IRQN(n));						\
	}

#define DT_DRV_COMPAT cdns_i2c
DT_INST_FOREACH_STATUS_OKAY_VARGS(CADENCE_I2C_INIT, DT_DRV_COMPAT)
