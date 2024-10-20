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
#include "i2c_bitbang.h"

LOG_MODULE_REGISTER(omap_i2c, CONFIG_I2C_LOG_LEVEL);

#define I2C_OMAP_TIMEOUT    100U
/* OCP_SYSSTATUS bit definitions */
#define SYSS_RESETDONE_MASK BIT(0)

#define I2C_BITRATE_FAST     400000
#define I2C_BITRATE_STANDARD 100000

/* I2C Registers */
typedef struct {
	uint8_t RESERVED_0[0x10]; /**< Reserved, offset: 0x0 */
	__IO uint32_t SYSC;       /**< System Configuration, offset: 0x10 */

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
} I2C_OMAP_Type, *I2C_OMAP_Type_Ptr;

/* I2C Interrupt Enable Register (I2C_OMAP_IE) */
#define I2C_OMAP_IE_XDR  BIT(14) /* TX Buffer drain interrupt enable */
#define I2C_OMAP_IE_RDR  BIT(13) /* RX Buffer drain interrupt enable */
#define I2C_OMAP_IE_XRDY BIT(4)  /* TX data ready interrupt enable */
#define I2C_OMAP_IE_RRDY BIT(3)  /* RX data ready interrupt enable */
#define I2C_OMAP_IE_ARDY BIT(2)  /* Access ready interrupt enable */
#define I2C_OMAP_IE_NACK BIT(1)  /* No acknowledgment interrupt enable */
#define I2C_OMAP_IE_AL   BIT(0)  /* Arbitration lost interrupt enable */

/* I2C Configuration Register (I2C_OMAP_CON) */
#define I2C_OMAP_CON_EN        BIT(15) /* I2C module enable */
#define I2C_OMAP_CON_OPMODE_HS BIT(12) /* High Speed support */
#define I2C_OMAP_CON_MST       BIT(10) /* Controller/target mode */
#define I2C_OMAP_CON_TRX       BIT(9)  /* TX/RX mode (controller only) */
#define I2C_OMAP_CON_STP       BIT(1)  /* Stop condition (controller only) */
#define I2C_OMAP_CON_STT       BIT(0)  /* Start condition (controller) */

/* I2C_OMAP_WE wakeup enable register */
#define I2C_OMAP_WE_XDR_WE  BIT(14) /* TX drain wakeup */
#define I2C_OMAP_WE_RDR_WE  BIT(13) /* RX drain wakeup */
#define I2C_OMAP_WE_AAS_WE  BIT(9)  /* Address as target wakeup */
#define I2C_OMAP_WE_BF_WE   BIT(8)  /* Bus free wakeup */
#define I2C_OMAP_WE_STC_WE  BIT(6)  /* Start condition wakeup */
#define I2C_OMAP_WE_GC_WE   BIT(5)  /* General call wakeup */
#define I2C_OMAP_WE_DRDY_WE BIT(3)  /* TX/RX data ready wakeup */
#define I2C_OMAP_WE_ARDY_WE BIT(2)  /* Register access ready wakeup */
#define I2C_OMAP_WE_NACK_WE BIT(1)  /* No acknowledgment wakeup */
#define I2C_OMAP_WE_AL_WE   BIT(0)  /* Arbitration lost wakeup */

#define I2C_OMAP_WE_ALL                                                                            \
	(I2C_OMAP_WE_XDR_WE | I2C_OMAP_WE_RDR_WE | I2C_OMAP_WE_AAS_WE | I2C_OMAP_WE_BF_WE |        \
	 I2C_OMAP_WE_STC_WE | I2C_OMAP_WE_GC_WE | I2C_OMAP_WE_DRDY_WE | I2C_OMAP_WE_ARDY_WE |      \
	 I2C_OMAP_WE_NACK_WE | I2C_OMAP_WE_AL_WE)

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
#define DEV_I2C_BASE(dev) ((I2C_OMAP_Type *)DEVICE_MMIO_NAMED_GET(dev, base))

struct i2c_omap_cfg {
	DEVICE_MMIO_NAMED_ROM(base);
	uint32_t irq;
	uint32_t speed;
	init_func_t init_func;
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
	struct k_sem cmd_complete;
	int cmd_err;
	bool receiver;
	bool bb_valid;
};

/**
 * @brief Acknowledge the I2C status by writing to the I2C_OMAP_STAT register.
 *
 * This function is used to acknowledge the I2C status by writing the provided
 * status value to the I2C_OMAP_STAT register. It is an inline function that is
 * specific to the OMAP I2C driver.
 *
 * @param dev Pointer to the device structure.
 * @param stat The status value to be written to the I2C_OMAP_STAT register.
 */
static inline void i2c_omap_ack_stat(const struct device *dev, uint16_t stat)
{
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
	i2c_base_addr->STAT = stat;
}

/**
 * @brief Initializes the OMAP I2C driver.
 *
 * This function is responsible for initializing the OMAP I2C driver.
 *
 * @param dev Pointer to the device structure for the I2C driver instance.
 */
static void i2c_omap_init_ll(const struct device *dev)
{

	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
	i2c_base_addr->CON = 0;

	i2c_base_addr->PSC = data->speed_config.pscstate;
	i2c_base_addr->SCLL = data->speed_config.scllstate;
	i2c_base_addr->SCLH = data->speed_config.sclhstate;
	i2c_base_addr->WE = I2C_OMAP_WE_ALL;
	i2c_base_addr->CON = I2C_OMAP_CON_EN;
	k_busy_wait(1);
	uint16_t iestate = (I2C_OMAP_IE_XRDY | I2C_OMAP_IE_RRDY | I2C_OMAP_IE_ARDY |
			    I2C_OMAP_IE_NACK | I2C_OMAP_IE_AL) |
			   ((data->current_msg.len) ? (I2C_OMAP_IE_RDR | I2C_OMAP_IE_XDR) : 0);
	i2c_base_addr->IRQENABLE_SET = iestate;
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
	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
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
		k_msleep(1);
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
	struct i2c_omap_data *data = dev->data;

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
	struct i2c_omap_data *data = dev->data;
	const struct i2c_omap_cfg *cfg = dev->config;
	k_sem_init(&data->cmd_complete, 0, 1);

	/* Set the speed for I2C */
	if (i2c_omap_set_speed(dev, cfg->speed)) {
		LOG_ERR("Failed to set speed");
		return -ENOTSUP;
	}
	i2c_omap_init_ll(dev);
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
	uint32_t speed_cfg;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);

	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		speed_cfg = I2C_BITRATE_STANDARD;
		goto out;
	case I2C_SPEED_FAST:
		speed_cfg = I2C_BITRATE_FAST;
		goto out;
	default:
		return -ENOTSUP;
	}
	if ((dev_config & I2C_MODE_CONTROLLER) != I2C_MODE_CONTROLLER) {
		return -ENOTSUP;
	}
	if ((dev_config & I2C_MSG_ADDR_10_BITS) != I2C_MSG_ADDR_10_BITS) {
		return -ENOTSUP;
	}
	i2c_base_addr->CON = 0;
	i2c_omap_set_speed(dev, speed_cfg);
	i2c_omap_init_ll(dev);
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
	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
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
	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
	uint16_t buf_cfg;
	buf_cfg = i2c_base_addr->BUF;

	if (data->receiver) {
		buf_cfg &= ~(0x3f << 8);
		buf_cfg |= ((size) << 8) | I2C_OMAP_BUF_RXFIF_CLR;
	} else {
		buf_cfg &= ~0x3f;
		buf_cfg |= (size) | I2C_OMAP_BUF_TXFIF_CLR;
	}
	i2c_base_addr->BUF = buf_cfg;
}

/**
 * @brief Wait for the bus to become free (no longer busy).
 *
 * This function waits for the bus to become free by continuously checking the
 * status register of the OMAP I2C controller. If the bus remains busy for a
 * certain timeout period, the function will return timeout error.
 *
 * @param dev The I2C device structure.
 * @return 0 if the bus becomes free, or a negative error code if the bus cannot
 * be recovered.
 */
static int i2c_omap_wait_for_bb(const struct device *dev)
{
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
	uint32_t timeout = k_uptime_get_32() + I2C_OMAP_TIMEOUT;

	while (i2c_base_addr->STAT & I2C_OMAP_STAT_BB) {
		if (k_uptime_get_32() > timeout) {
			LOG_ERR("Bus busy timeout");
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
	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
	uint16_t bits, stat;
	int err = 0;

	do {
		bits = i2c_base_addr->IE;
		stat = i2c_base_addr->STAT;
		stat |= bits;
		if (data->receiver) {
			stat &= ~(I2C_OMAP_STAT_XDR | I2C_OMAP_STAT_XRDY);
		} else {
			stat &= ~(I2C_OMAP_STAT_RDR | I2C_OMAP_STAT_RRDY);
		}
		if (stat & I2C_OMAP_STAT_NACK) {
			err |= I2C_OMAP_STAT_NACK;
			i2c_omap_ack_stat(dev, I2C_OMAP_STAT_NACK);
		}
		if (stat & I2C_OMAP_STAT_AL) {
			err |= I2C_OMAP_STAT_AL;
			i2c_omap_ack_stat(dev, I2C_OMAP_STAT_AL);
		}
		if (stat & I2C_OMAP_STAT_ARDY) {
			i2c_omap_ack_stat(dev, I2C_OMAP_STAT_ARDY);
		}
		if (stat & (I2C_OMAP_STAT_ARDY | I2C_OMAP_STAT_NACK | I2C_OMAP_STAT_AL)) {
			i2c_omap_ack_stat(dev, I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_RDR |
						       I2C_OMAP_STAT_XRDY | I2C_OMAP_STAT_XDR |
						       I2C_OMAP_STAT_ARDY);
			break;
		}
		/* Handle receive logic */
		if (stat & (I2C_OMAP_STAT_RRDY | I2C_OMAP_STAT_RDR)) {
			int buffer = (stat & I2C_OMAP_STAT_RRDY) ? i2c_base_addr->BUF
								 : i2c_base_addr->BUFSTAT;
			i2c_omap_transmit_receive_data(dev, buffer);
			i2c_omap_ack_stat(dev, stat & I2C_OMAP_STAT_RRDY ? I2C_OMAP_STAT_RRDY
									 : I2C_OMAP_STAT_RDR);
			continue;
		}
		/* Handle transmit logic */
		if (stat & (I2C_OMAP_STAT_XRDY | I2C_OMAP_STAT_XDR)) {
			int buffer = (stat & I2C_OMAP_STAT_XRDY) ? i2c_base_addr->BUF
								 : i2c_base_addr->BUFSTAT;
			i2c_omap_transmit_receive_data(dev, buffer);
			i2c_omap_ack_stat(dev, (stat & I2C_OMAP_STAT_XRDY) ? I2C_OMAP_STAT_XRDY
									   : I2C_OMAP_STAT_XDR);
			continue;
		}
		if (stat & I2C_OMAP_STAT_ROVR) {
			LOG_ERR("Receive overrun");
			err |= I2C_OMAP_STAT_ROVR;
			i2c_omap_ack_stat(dev, I2C_OMAP_STAT_ROVR);
			break;
		}
		if (stat & I2C_OMAP_STAT_XUDF) {
			LOG_ERR("Transmit underflow");
			err |= I2C_OMAP_STAT_XUDF;
			i2c_omap_ack_stat(dev, I2C_OMAP_STAT_XUDF);
			break;
		}
		if (!stat) {
			data->cmd_err |= -EAGAIN;
			return -EAGAIN;
		}
		data->cmd_err = err;
	} while (stat);
	return err;
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
	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);
	unsigned long time_left = 0;
	uint16_t control_reg;
	int result;
	/* Determine message direction (read or write) and update the receiver flag */
	data->receiver = msg->flags & I2C_MSG_READ;
	/* Adjust the FIFO size according to the message length */
	i2c_omap_resize_fifo(dev, msg->len);
	/* Set the target I2C address for the transfer */
	i2c_base_addr->SA = addr;
	/* Store the message in the data structure */
	data->current_msg = *msg;
	/* Insert a memory barrier to ensure proper instruction ordering */
	compiler_barrier();
	/* Set the message length in the I2C controller */
	i2c_base_addr->CNT = msg->len;
	/* Clear FIFO buffers */
	control_reg = i2c_base_addr->BUF;
	control_reg |= I2C_OMAP_BUF_RXFIF_CLR | I2C_OMAP_BUF_TXFIF_CLR;
	i2c_base_addr->BUF = control_reg;
	/* If we're not polling, reset the command completion semaphore */
	if (!polling) {
		k_sem_reset(&data->cmd_complete);
	}
	/* Reset the command error status */
	data->cmd_err = 0;
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
	/* Wait for the transfer to complete by polling */
	do {
		/* Poll for status until the transfer is complete */
		for (uint8_t retries = 0; retries < 5; retries++) {
			if (i2c_base_addr->STAT) {
				break;
			}
			result = -EAGAIN;
		}
		/* Call a lower-level function to continue the transfer */
		result = i2c_omap_transfer_message_ll(dev);
	} while (result == -EAGAIN);
	time_left = !result;
	/* Handle timeout or specific error conditions */
	if (time_left == 0 || (data->cmd_err & (I2C_OMAP_STAT_ROVR | I2C_OMAP_STAT_XUDF))) {
		i2c_omap_reset(dev);
		i2c_omap_init_ll(dev);
		/* Return an error code based on whether it was a timeout or buffer error */
		if (time_left == 0) {
			return -ETIMEDOUT;
		}
		return -EIO; /* Receiver overrun or transmitter underflow */
	}
	/* Handle arbitration loss and NACK errors */
	if (data->cmd_err & (I2C_OMAP_STAT_AL | I2C_OMAP_STAT_NACK)) {
		if (data->cmd_err & I2C_OMAP_STAT_AL) {
			return -EAGAIN; /* Retry on arbitration loss */
		}
		if (data->cmd_err & I2C_OMAP_STAT_NACK) {
			if (msg->flags) {
				return 0; /* If this message can tolerate a NACK, return success */
			}
			/* Issue a STOP condition after NACK */
			i2c_base_addr->CON |= I2C_OMAP_CON_STP;
			return -ENOMSG; /* Indicate a message error due to NACK */
		}
	}
	/* If no errors occurred, return success */
	if (!data->cmd_err) {
		return 0;
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

	if (ret < 0) {
		return ret;
	}
	for (int msg_idx = 0; msg_idx < num; msg_idx++) {
		ret = i2c_omap_transfer_message(dev, &msg[msg_idx], polling, addr);
		if (ret < 0) {
			break;
		}
	}
	i2c_omap_wait_for_bb(dev);
	return ret;
}

/**
 * @brief OMAP I2C ISR function.
 *
 * This function is the interrupt service routine for OMAP I2C.
 * It checks the status and interrupt enable registers to determine if the
 * transfer is complete, and signals the completion using a semaphore.
 *
 * @param dev Pointer to the I2C device structure.
 */
static void i2c_omap_isr(const struct device *dev)
{
	struct i2c_omap_data *data = dev->data;
	I2C_OMAP_Type *i2c_base_addr = DEV_I2C_BASE(dev);

	if (i2c_base_addr->STAT & i2c_base_addr->IE & ~I2C_OMAP_STAT_NACK) {
		k_sem_give(&data->cmd_complete);
	}
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
static int __maybe_unused i2c_omap_transfer_polling(const struct device *dev, struct i2c_msg msgs[],
						    uint8_t num_msgs, uint16_t addr)
{
	return i2c_omap_transfer_main(dev, msgs, num_msgs, true, addr);
}

static const struct i2c_driver_api i2c_omap_api = {
	.transfer = i2c_omap_transfer_polling,
	.configure = i2c_omap_configure,
};

#define I2C_OMAP_INIT(inst)                                                                        \
	LOG_INSTANCE_REGISTER(omap_i2c, inst, CONFIG_I2C_LOG_LEVEL);                               \
	static void i2c_omap_##inst##_init(const struct device *dev);                              \
	static const struct i2c_omap_cfg i2c_omap_cfg_##inst = {                                   \
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(inst)),                               \
		.irq = DT_INST_IRQN(inst),                                                         \
		.init_func = i2c_omap_##inst##_init,                                               \
		.speed = DT_INST_PROP(inst, clock_frequency),                                      \
	};                                                                                         \
                                                                                                   \
	static struct i2c_omap_data i2c_omap_data_##inst;                                          \
                                                                                                   \
	I2C_DEVICE_DT_INST_DEFINE(inst, i2c_omap_init, NULL, &i2c_omap_data_##inst,                \
				  &i2c_omap_cfg_##inst, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,     \
				  &i2c_omap_api);                                                  \
                                                                                                   \
	static void i2c_omap_##inst##_init(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), i2c_omap_isr,         \
			    DEVICE_DT_INST_GET(inst), DT_INST_IRQ(inst, sense));                   \
                                                                                                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	};
DT_INST_FOREACH_STATUS_OKAY(I2C_OMAP_INIT)
