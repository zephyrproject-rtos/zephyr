/* Copyright (C) 2024 BeagleBoard.org Foundation
 * Copyright (C) 2024 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_omap_i2c
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/i2c.h>

#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
#include "i2c_bitbang.h"
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */

LOG_MODULE_REGISTER(omap_i2c, CONFIG_I2C_LOG_LEVEL);

#define I2C_OMAP_TIMEOUT     100U
/* OCP_SYSSTATUS bit definitions */
#define SYSS_RESETDONE_MASK  BIT(0)
#define RETRY                -1
#define I2C_BITRATE_FAST     400000
#define I2C_BITRATE_STANDARD 100000

/* I2C Registers */
typedef struct {
	uint8_t RESERVED_0[0x10]; /**< Reserved, offset: 0x0 */

	__IO uint32_t SYSC;          /**< System Configuration, offset: 0x10 */
	uint8_t RESERVED_1[0x18];    /**< Reserved, offset: 0x14 - 0x2C */
	__IO uint32_t IRQENABLE_SET; /**< Interrupt Enable Set, offset: 0x2C */
	uint8_t RESERVED_2[0x4];     /**< Reserved, offset: 0x30 - 0x34 */
	__IO uint32_t WE;            /**< Wakeup Enable, offset: 0x34 */
	uint8_t RESERVED_3[0x4C];    /**< Reserved, offset: 0x38 - 0x84 */
	__IO uint32_t IE;            /**< Interrupt Enable (Legacy), offset: 0x84 */
	__IO uint32_t STAT;          /**< Status, offset: 0x88 */
	uint8_t RESERVED_4[0x4];     /**< Reserved, offset: 0x8C - 0x90 */
	__IO uint32_t SYSS;          /**< System Status, offset: 0x90 */
	__IO uint32_t BUF;           /**< Buffer, offset: 0x94 */
	__IO uint32_t CNT;           /**< Data Count, offset: 0x98 */
	__IO uint32_t DATA;          /**< Data Access, offset: 0x9C */
	uint8_t RESERVED_5[0x4];     /**< Reserved, offset: 0xA0 - 0xA4 */
	__IO uint32_t CON;           /**< Configuration, offset: 0xA4 */
	__IO uint32_t OA;            /**< Own Address, offset: 0xA8 */
	__IO uint32_t SA;            /**< Target Address, offset: 0xAC */
	__IO uint32_t PSC;           /**< Clock Prescaler, offset: 0xB0 */
	__IO uint32_t SCLL;          /**< SCL Low Time, offset: 0xB4 */
	__IO uint32_t SCLH;          /**< SCL High Time, offset: 0xB8 */
	__IO uint32_t SYSTEST;       /**< System Test, offset: 0xBC */
	__IO uint32_t BUFSTAT;       /**< Buffer Status, offset: 0xC0 */
} i2c_omap_regs_t;

/* I2C Configuration Register (I2C_OMAP_CON) */
#define I2C_OMAP_CON_EN        BIT(15) /* I2C module enable */
#define I2C_OMAP_CON_OPMODE_HS BIT(12) /* High Speed support */
#define I2C_OMAP_CON_MST       BIT(10) /* Controller/target mode */
#define I2C_OMAP_CON_TRX       BIT(9)  /* TX/RX mode (controller only) */
#define I2C_OMAP_CON_STP       BIT(1)  /* Stop condition (controller only) */
#define I2C_OMAP_CON_STT       BIT(0)  /* Start condition (controller) */

/* I2C Buffer Configuration Register (I2C_OMAP_BUF): */
#define I2C_OMAP_BUF_RXFIF_CLR BIT(14) /* RX FIFO Clear */
#define I2C_OMAP_BUF_TXFIF_CLR BIT(6)  /* TX FIFO Clear */

/* I2C Status Register (I2C_OMAP_STAT): */
#define I2C_OMAP_STAT_XDR  BIT(14) /* TX Buffer draining */
#define I2C_OMAP_STAT_RDR  BIT(13) /* RX Buffer draining */
#define I2C_OMAP_STAT_BB   BIT(12) /* Bus busy */
#define I2C_OMAP_STAT_ROVR BIT(11) /* Receive overrun */
#define I2C_OMAP_STAT_XUDF BIT(10) /* Transmit underflow */
#define I2C_OMAP_STAT_AAS  BIT(9)  /* Address as target */
#define I2C_OMAP_STAT_XRDY BIT(4)  /* Transmit data ready */
#define I2C_OMAP_STAT_RRDY BIT(3)  /* Receive data ready */
#define I2C_OMAP_STAT_ARDY BIT(2)  /* Register access ready */
#define I2C_OMAP_STAT_NACK BIT(1)  /* No ack interrupt enable */
#define I2C_OMAP_STAT_AL   BIT(0)  /* Arbitration lost */

/* I2C System Test Register (I2C_OMAP_SYSTEST): */
#define I2C_OMAP_SYSTEST_ST_EN       BIT(15)   /* System test enable */
#define I2C_OMAP_SYSTEST_FREE        BIT(14)   /* Free running mode */
#define I2C_OMAP_SYSTEST_TMODE_MASK  (3 << 12) /* Test mode select mask */
#define I2C_OMAP_SYSTEST_TMODE_SHIFT (12)      /* Test mode select shift */

/* Functional mode */
#define I2C_OMAP_SYSTEST_SCL_I_FUNC BIT(8) /* SCL line input value */
#define I2C_OMAP_SYSTEST_SDA_I_FUNC BIT(6) /* SDA line input value */

/* SDA/SCL IO mode */
#define I2C_OMAP_SYSTEST_SCL_I BIT(3) /* SCL line sense in */
#define I2C_OMAP_SYSTEST_SCL_O BIT(2) /* SCL line drive out */
#define I2C_OMAP_SYSTEST_SDA_I BIT(1) /* SDA line sense in */
#define I2C_OMAP_SYSTEST_SDA_O BIT(0) /* SDA line drive out */

typedef void (*init_func_t)(const struct device *dev);
#define DEV_CFG(dev)      ((const struct i2c_omap_cfg *)(dev)->config)
#define DEV_DATA(dev)     ((struct i2c_omap_data *)(dev)->data)
#define DEV_I2C_BASE(dev) ((i2c_omap_regs_t *)DEVICE_MMIO_NAMED_GET(dev, base))

struct i2c_omap_cfg {
	DEVICE_MMIO_NAMED_ROM(base);
	uint32_t irq;
	uint32_t speed;
};

enum i2c_omap_speed {
	I2C_OMAP_SPEED_STANDARD,
	I2C_OMAP_SPEED_FAST,
	I2C_OMAP_SPEED_FAST_PLUS,
};

struct i2c_omap_speed_config {
	uint32_t pscstate;
	uint32_t scllstate;
	uint32_t sclhstate;
};

struct i2c_omap_data {
	DEVICE_MMIO_NAMED_RAM(base);
	enum i2c_omap_speed speed;
	struct i2c_omap_speed_config speed_config;
	struct i2c_msg current_msg;
	struct k_sem lock;
	bool receiver;
	bool bb_valid;
};

/**
 * @brief Initializes the OMAP I2C driver.
 *
 * This function is responsible for initializing the OMAP I2C driver.
 *
 * @param dev Pointer to the device structure for the I2C driver instance.
 */
static void i2c_omap_init_ll(const struct device *dev)
{

	struct i2c_omap_data *data = DEV_DATA(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	i2c_base_addr->CON = 0;
	i2c_base_addr->PSC = data->speed_config.pscstate;
	i2c_base_addr->SCLL = data->speed_config.scllstate;
	i2c_base_addr->SCLH = data->speed_config.sclhstate;
	i2c_base_addr->CON = I2C_OMAP_CON_EN;
}

/**
 * @brief Reset the OMAP I2C controller.
 *
 * This function resets the OMAP I2C controller specified by the device pointer.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @return 0 on success, negative errno code on failure.
 */
static int i2c_omap_reset(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint64_t timeout;
	uint16_t sysc;

	sysc = i2c_base_addr->SYSC;
	i2c_base_addr->CON &= ~I2C_OMAP_CON_EN;
	timeout = k_uptime_get() + I2C_OMAP_TIMEOUT;
	i2c_base_addr->CON = I2C_OMAP_CON_EN;
	while (!(i2c_base_addr->SYSS & SYSS_RESETDONE_MASK)) {
		if (k_uptime_get() > timeout) {
			LOG_WRN("timeout waiting for controller reset");
			return -ETIMEDOUT;
		}
		k_busy_wait(100);
	}
	i2c_base_addr->SYSC = sysc;
	data->bb_valid = 0;
	return 0;
}

/**
 * @brief Set the speed of the OMAP I2C controller.
 *
 * This function sets the speed of the OMAP I2C controller based on the
 * specified speed parameter. The speed can be set to either Fast mode or
 * Standard mode.
 *
 * @param dev The pointer to the device structure.
 * @param speed The desired speed for the I2C controller.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_omap_set_speed(const struct device *dev, uint32_t speed)
{
	struct i2c_omap_data *data = DEV_DATA(dev);

	/* If configured for High Speed */
	switch (speed) {
	case I2C_BITRATE_FAST:
		/* Fast mode */
		data->speed_config = (struct i2c_omap_speed_config){
			.pscstate = 9,
			.scllstate = 7,
			.sclhstate = 5,
		};
		break;
	case I2C_BITRATE_STANDARD:
		/* Standard mode */
		data->speed_config = (struct i2c_omap_speed_config){
			.pscstate = 23,
			.scllstate = 13,
			.sclhstate = 15,
		};
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

/**
 * @brief Configure the OMAP I2C controller with the specified device configuration.
 *
 * This function configures the OMAP I2C controller with the specified device configuration.
 *
 * @param dev The pointer to the device structure.
 * @param dev_config The device configuration to be applied.
 *
 * @return 0 on success, negative error code on failure.
 */
static int i2c_omap_configure(const struct device *dev, uint32_t dev_config)
{
	uint32_t speed_cfg = I2C_BITRATE_STANDARD;
	struct i2c_omap_data *data = DEV_DATA(dev);

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed_cfg = I2C_BITRATE_STANDARD;
		break;
	case I2C_SPEED_FAST:
		speed_cfg = I2C_BITRATE_FAST;
		break;
	default:
		return -ENOTSUP;
	}
	if ((dev_config & I2C_MODE_CONTROLLER) != I2C_MODE_CONTROLLER) {
		return -ENOTSUP;
	}
	k_sem_take(&data->lock, K_FOREVER);
	i2c_omap_set_speed(dev, speed_cfg);
	i2c_omap_init_ll(dev);
	k_sem_give(&data->lock);
	return 0;
}

/**
 * @brief Transmit or receive data over I2C bus
 *
 * This function transmits or receives data over the I2C bus using the OMAP I2C controller.
 *
 * @param dev Pointer to the I2C device structure
 * @param num_bytes Number of bytes to transmit or receive
 */
static void i2c_omap_transmit_receive_data(const struct device *dev, uint8_t num_bytes)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint8_t *buf_ptr = data->current_msg.buf;

	while (num_bytes--) {
		if (data->receiver) {
			*buf_ptr++ = i2c_base_addr->DATA;
		} else {
			i2c_base_addr->DATA = *(buf_ptr++);
		}
	}
}

/**
 * @brief Resize the FIFO buffer for the OMAP I2C controller.
 *
 * This function resizes the FIFO buffer for the OMAP I2C controller based on the specified size.
 * It clears the RX threshold and sets the new size for the receiver, or clears the TX threshold
 * and sets the new size for the transmitter.
 *
 * @param dev Pointer to the device structure.
 * @param size The new size of the FIFO buffer.
 */
static void i2c_omap_resize_fifo(const struct device *dev, uint8_t size)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);

	if (data->receiver) {
		i2c_base_addr->BUF &= I2C_OMAP_BUF_RXFIF_CLR;
		i2c_base_addr->BUF |= ((size) << 8) | I2C_OMAP_BUF_RXFIF_CLR;
	} else {
		i2c_base_addr->BUF &= I2C_OMAP_BUF_TXFIF_CLR;
		i2c_base_addr->BUF |= (size) | I2C_OMAP_BUF_TXFIF_CLR;
	}
}

#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
/**
 * @brief Get the state of the SDA line.
 *
 * This function retrieves the state of the SDA (data) line for the OMAP I2C controller.
 *
 * @param io_context The I2C context.
 * @return The state of the SDA line.
 */
static int i2c_omap_get_sda(void *io_context)
{
	const struct i2c_omap_cfg *cfg = (const struct i2c_omap_cfg *)io_context;
	i2c_omap_regs_t *i2c_base_addr = (i2c_omap_regs_t *)cfg->base.addr;

	return (i2c_base_addr->SYSTEST & I2C_OMAP_SYSTEST_SDA_I_FUNC) ? 1 : 0;
}

/**
 * @brief Set the state of the SDA line.
 *
 * This function sets the state of the SDA (data) line for the OMAP I2C controller.
 *
 * @param io_context The I2C context.
 * @param state The state to set (0 for low, 1 for high).
 */
static void i2c_omap_set_sda(void *io_context, int state)
{
	const struct i2c_omap_cfg *cfg = (const struct i2c_omap_cfg *)io_context;
	i2c_omap_regs_t *i2c_base_addr = (i2c_omap_regs_t *)cfg->base.addr;

	if (state) {
		i2c_base_addr->SYSTEST |= I2C_OMAP_SYSTEST_SDA_O;
	} else {
		i2c_base_addr->SYSTEST &= ~I2C_OMAP_SYSTEST_SDA_O;
	}
}

/**
 * @brief Set the state of the SCL line.
 *
 * This function sets the state of the SCL (clock) line for the OMAP I2C controller.
 *
 * @param io_context The I2C context.
 * @param state The state to set (0 for low, 1 for high).
 */
static void i2c_omap_set_scl(void *io_context, int state)
{
	const struct i2c_omap_cfg *cfg = (const struct i2c_omap_cfg *)io_context;
	i2c_omap_regs_t *i2c_base_addr = (i2c_omap_regs_t *)cfg->base.addr;

	if (state) {
		i2c_base_addr->SYSTEST |= I2C_OMAP_SYSTEST_SCL_O;
	} else {
		i2c_base_addr->SYSTEST &= ~I2C_OMAP_SYSTEST_SCL_O;
	}
}
/**
 * @brief Recovers the I2C bus using the OMAP I2C controller.
 *
 * This function attempts to recover the I2C bus by performing a bus recovery
 * sequence using the OMAP I2C controller. It uses the provided device
 * configuration and bit-banging operations to recover the bus.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, negative error code on failure.
 */

static int i2c_omap_recover_bus(const struct device *dev)
{
	const struct i2c_omap_cfg *cfg = DEV_CFG(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	struct i2c_omap_data *data = DEV_DATA(dev);

	struct i2c_bitbang bitbang_omap;
	struct i2c_bitbang_io bitbang_omap_io = {
		.get_sda = i2c_omap_get_sda,
		.set_scl = i2c_omap_set_scl,
		.set_sda = i2c_omap_set_sda,
	};
	int error = 0;

	k_sem_take(&data->lock, K_FOREVER);
	i2c_base_addr->SYSTEST |= I2C_OMAP_SYSTEST_ST_EN | (3 << I2C_OMAP_SYSTEST_TMODE_SHIFT) |
				  I2C_OMAP_SYSTEST_SCL_O | I2C_OMAP_SYSTEST_SDA_O;
	i2c_bitbang_init(&bitbang_omap, &bitbang_omap_io, (void *)cfg);
	error = i2c_bitbang_recover_bus(&bitbang_omap);
	if (error != 0) {
		LOG_ERR("failed to recover bus (err %d)", error);
		goto restore;
	}

restore:
	i2c_base_addr->SYSTEST &= ~(I2C_OMAP_SYSTEST_ST_EN | I2C_OMAP_SYSTEST_TMODE_MASK |
				    I2C_OMAP_SYSTEST_SCL_O | I2C_OMAP_SYSTEST_SDA_O);
	i2c_omap_reset(dev);
	k_sem_give(&data->lock);
	return error;
}
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */

/**
 * @brief Wait for the bus to become free (no longer busy).
 *
 * This function waits for the bus to become free by continuously checking the
 * status register of the OMAP I2C controller. If the bus remains busy for a
 * certain timeout period, the function will return attempts to recover the bus by calling
 * i2c_omap_recover_bus().
 *
 * @param dev The I2C device structure.
 * @return 0 if the bus becomes free, or a negative error code if the bus cannot
 * be recovered.
 */
static int i2c_omap_wait_for_bb(const struct device *dev)
{
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint32_t timeout = k_uptime_get_32() + I2C_OMAP_TIMEOUT;

	while (i2c_base_addr->STAT & I2C_OMAP_STAT_BB) {
		if (k_uptime_get_32() > timeout) {
			LOG_ERR("Bus busy timeout");
#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
			return i2c_omap_recover_bus(dev);
#else
			return -ETIMEDOUT;
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */
		}
		k_busy_wait(100);
	}
	return 0;
}

/**
 * @brief Performs data transfer for the OMAP I2C driver.
 *
 * This function is responsible for handling the data transfer logic for the OMAP I2C driver.
 * It reads the status register and performs the necessary actions based on the status flags.
 * It handles both receive and transmit logic, and also handles error conditions such as NACK,
 * arbitration lost, receive overrun, and transmit underflow.
 *
 * @param dev Pointer to the device structure.
 *
 * @return Returns 0 on success, or a negative error code on failure.
 */
static int i2c_omap_transfer_message_ll(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	uint16_t stat = i2c_base_addr->STAT, result = 0;

	if (data->receiver) {
		stat &= ~(I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_XRDY);
	} else {
		stat &= ~(I2C_OMAP_STAT_RDR | I2C_OMAP_STAT_RRDY);
	}
	if (stat & I2C_OMAP_STAT_NACK) {
		result |= I2C_OMAP_STAT_NACK;
		i2c_base_addr->STAT |= I2C_OMAP_STAT_NACK;
	}
	if (stat & I2C_OMAP_STAT_AL) {
		result |= I2C_OMAP_STAT_AL;
		i2c_base_addr->STAT |= I2C_OMAP_STAT_AL;
	}
	if (stat & I2C_OMAP_STAT_ARDY) {
		i2c_base_addr->STAT |= I2C_OMAP_STAT_ARDY;
	}
	if (stat & (I2C_OMAP_STAT_ARDY | I2C_OMAP_STAT_NACK | I2C_OMAP_STAT_AL)) {

		i2c_base_addr->STAT |=
			(I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_RDR | I2C_OMAP_STAT_XRDY |
			 I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_ARDY);
		return result;
	}

	/* Handle receive logic */
	if (stat & (I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_RDR)) {
		int buffer =
			(stat & I2C_OMAP_STAT_RRDY) ? i2c_base_addr->BUF : i2c_base_addr->BUFSTAT;
		i2c_omap_transmit_receive_data(dev, buffer);
		i2c_base_addr->STAT |=
			(stat & I2C_OMAP_STAT_RRDY) ? I2C_OMAP_STAT_RRDY : I2C_OMAP_STAT_RDR;
		return RETRY;
	}

	/* Handle transmit logic */
	if (stat & (I2C_OMAP_STAT_XRDY | I2C_OMAP_STAT_XDR)) {
		int buffer =
			(stat & I2C_OMAP_STAT_XRDY) ? i2c_base_addr->BUF : i2c_base_addr->BUFSTAT;
		i2c_omap_transmit_receive_data(dev, buffer);
		i2c_base_addr->STAT |=
			(stat & I2C_OMAP_STAT_XRDY) ? I2C_OMAP_STAT_XRDY : I2C_OMAP_STAT_XDR;
		return RETRY;
	}

	if (stat & I2C_OMAP_STAT_ROVR) {
		i2c_base_addr->STAT |= I2C_OMAP_STAT_ROVR;
		return I2C_OMAP_STAT_ROVR;
	}
	if (stat & I2C_OMAP_STAT_XUDF) {
		i2c_base_addr->STAT |= I2C_OMAP_STAT_XUDF;
		return I2C_OMAP_STAT_XUDF;
	}
	return RETRY;
}

/**
 * @brief Performs an I2C transfer of a single message.
 *
 * This function is responsible for performing an I2C transfer of a single message.
 * It sets up the necessary configurations, writes the target device address,
 * sets the buffer and buffer length, and handles various error conditions.
 *
 * @param dev The I2C device structure.
 * @param msg Pointer to the I2C message structure.
 * @param polling Flag indicating whether to use polling mode or not.
 * @param addr The target device address.
 *
 * @return 0 on success, negative error code on failure.
 *         Possible error codes include:
 *         - ETIMEDOUT: Timeout occurred during the transfer.
 *         - EIO: I/O error due to receiver overrun or transmit underflow.
 *         - EAGAIN: Arbitration lost error, try again.
 *         - ENOMSG: Message error due to NACK.
 */
static int i2c_omap_transfer_message(const struct device *dev, struct i2c_msg *msg, bool polling,
				     uint16_t addr)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	i2c_omap_regs_t *i2c_base_addr = DEV_I2C_BASE(dev);
	unsigned long time_left = 1000;
	uint16_t control_reg;
	int result = 0;
	/* Determine message direction (read or write) and update the receiver flag */
	data->receiver = msg->flags & I2C_MSG_READ;
	/* Adjust the FIFO size according to the message length */
	i2c_omap_resize_fifo(dev, msg->len);
	/* Set the target I2C address for the transfer */
	i2c_base_addr->SA = addr;
	/* Store the message in the data structure */
	data->current_msg = *msg;
	/* Set the message length in the I2C controller */
	i2c_base_addr->CNT = msg->len;
	/* Clear FIFO buffers */
	control_reg = i2c_base_addr->BUF;
	control_reg |= I2C_OMAP_BUF_RXFIF_CLR | I2C_OMAP_BUF_TXFIF_CLR;
	i2c_base_addr->BUF = control_reg;
	/* If we're not polling, reset the command completion semaphore */
	if (!polling) {
		k_sem_reset(&data->lock);
	}
	/* Reset the command error status */
	/* Prepare the control register for the I2C operation */
	control_reg = I2C_OMAP_CON_EN | I2C_OMAP_CON_MST | I2C_OMAP_CON_STT;
	/* Enable high-speed mode if required by the transfer speed */
	if (data->speed > I2C_BITRATE_FAST) {
		control_reg |= I2C_OMAP_CON_OPMODE_HS;
	}
	/* Set the STOP condition if it's specified in the message flags */
	if (msg->flags & I2C_MSG_STOP) {
		control_reg |= I2C_OMAP_CON_STP;
	}
	/* Set the transmission mode based on whether it's a read or write operation */
	if (!(msg->flags & I2C_MSG_READ)) {
		control_reg |= I2C_OMAP_CON_TRX;
	}
	/* Start the I2C transfer by writing the control register */
	i2c_base_addr->CON = control_reg;
	/* Poll for status until the transfer is complete */
	/* Call a lower-level function to continue the transfer */
	do {
		result = i2c_omap_transfer_message_ll(dev);
		time_left--;
	} while (result == RETRY && time_left);

	/* If no errors occurred, return success */
	if (!result) {
		return 0;
	}

	/* Handle timeout or specific error conditions */
	if (result & (I2C_OMAP_STAT_ROVR | I2C_OMAP_STAT_XUDF)) {
		i2c_omap_reset(dev);
		i2c_omap_init_ll(dev);
		/* Return an error code based on whether it was a timeout or buffer error */
		return -EIO; /* Receiver overrun or transmitter underflow */
	}
	/* Handle arbitration loss and NACK errors */
	if (result & (I2C_OMAP_STAT_AL | -EAGAIN)) {
		return -EAGAIN;
	}
	if (result & I2C_OMAP_STAT_NACK) {
		/* Issue a STOP condition after NACK */
		i2c_base_addr->CON |= I2C_OMAP_CON_STP;
		return -ENOMSG; /* Indicate a message error due to NACK */
	}

	/* Return a general I/O error if no specific error conditions matched */
	return -EIO;
}

/**
 * @brief Performs a common transfer operation for OMAP I2C devices.
 *
 * This function is responsible for transferring multiple I2C messages in a common way
 * for OMAP I2C devices. It waits for the bus to be idle, then iterates through each
 * message in the provided array and transfers them one by one using the i2c_omap_transfer_message()
 * function. After all messages have been transferred, it waits for the bus to be idle again
 * before returning.
 *
 * @param dev The pointer to the I2C device structure.
 * @param msg An array of I2C messages to be transferred.
 * @param num The number of messages in the array.
 * @param polling Specifies whether to use polling or interrupt-based transfer.
 * @param addr The I2C target address.
 * @return 0 on success, or a negative error code on failure.
 */
static int i2c_omap_transfer_main(const struct device *dev, struct i2c_msg msg[], int num,
				  bool polling, uint16_t addr)
{
	int ret;

	ret = i2c_omap_wait_for_bb(dev);
	struct i2c_omap_data *data = DEV_DATA(dev);

	k_sem_take(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}
	for (int msg_idx = 0; msg_idx < num; msg_idx++) {
		ret = i2c_omap_transfer_message(dev, &msg[msg_idx], polling, addr);
		if (ret < 0) {
			break;
		}
	}
	k_sem_give(&data->lock);
	i2c_omap_wait_for_bb(dev);
	return ret;
}

/**
 * @brief OMAP I2C transfer function using polling.
 *
 * This function performs the I2C transfer using the OMAP I2C controller
 * in polling mode. It calls the common transfer function with the
 * specified messages, number of messages, and target address.
 *
 * @param dev Pointer to the I2C device structure.
 * @param msgs Array of I2C messages to be transferred.
 * @param num_msgs Number of I2C messages in the array.
 * @param addr Target address.
 * @return 0 on success, negative error code on failure.
 */
static int i2c_omap_transfer_polling(const struct device *dev, struct i2c_msg msgs[],
						    uint8_t num_msgs, uint16_t addr)
{
	return i2c_omap_transfer_main(dev, msgs, num_msgs, true, addr);
}

static DEVICE_API(i2c, i2c_omap_api) = {
	.transfer = i2c_omap_transfer_polling,
	.configure = i2c_omap_configure,
#ifdef CONFIG_I2C_OMAP_BUS_RECOVERY
	.recover_bus = i2c_omap_recover_bus,
#endif /* CONFIG_I2C_OMAP_BUS_RECOVERY */
};

/**
 * @brief Initialize the OMAP I2C controller.
 *
 * This function initializes the OMAP I2C controller by setting the speed and
 * performing any necessary initialization steps.
 *
 * @param dev Pointer to the device structure for the I2C controller.
 * @return 0 if successful, negative error code otherwise.
 */
static int i2c_omap_init(const struct device *dev)
{
	struct i2c_omap_data *data = DEV_DATA(dev);
	const struct i2c_omap_cfg *cfg = DEV_CFG(dev);

	k_sem_init(&data->lock, 1, 1);
	/* Set the speed for I2C */
	if (i2c_omap_set_speed(dev, cfg->speed)) {
		LOG_ERR("Failed to set speed");
		return -ENOTSUP;
	}
	i2c_omap_init_ll(dev);
	return 0;
}

#define I2C_OMAP_INIT(inst)                                                                        \
	LOG_INSTANCE_REGISTER(omap_i2c, inst, CONFIG_I2C_LOG_LEVEL);                               \
	static const struct i2c_omap_cfg i2c_omap_cfg_##inst = {                                   \
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(inst)),                               \
		.irq = DT_INST_IRQN(inst),                                                         \
		.speed = DT_INST_PROP(inst, clock_frequency),                                      \
	};                                                                                         \
                                                                                                   \
	static struct i2c_omap_data i2c_omap_data_##inst;                                          \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_omap_init, NULL, &i2c_omap_data_##inst,                \
				  &i2c_omap_cfg_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
				  &i2c_omap_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_OMAP_INIT)
