/**
 * @file
 *
 * @brief Public APIs for the I2C drivers.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DRIVERS_I2C_H
#define __DRIVERS_I2C_H

/**
 * @brief I2C Interface
 * @defgroup i2c_interface I2C Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <device.h>

/*
 * The following #defines are used to configure the I2C controller.
 */

/** I2C Standard Speed */
#define I2C_SPEED_STANDARD		(0x1)

/** I2C Fast Speed */
#define I2C_SPEED_FAST			(0x2)

/** I2C Fast Plus Speed */
#define I2C_SPEED_FAST_PLUS		(0x3)

/** I2C High Speed */
#define I2C_SPEED_HIGH			(0x4)

/** I2C Ultra Fast Speed */
#define I2C_SPEED_ULTRA			(0x5)

#define I2C_SPEED_SHIFT			(1)
#define I2C_SPEED_SET(speed)		(((speed) << I2C_SPEED_SHIFT) \
						& I2C_SPEED_MASK)
#define I2C_SPEED_MASK			(0x7 << I2C_SPEED_SHIFT) /* 3 bits */
#define I2C_SPEED_GET(cfg) 		(((cfg) & I2C_SPEED_MASK) \
						>> I2C_SPEED_SHIFT)

/** Use 10-bit addressing. */
#define I2C_ADDR_10_BITS		(1 << 0)

/** Controller to act as Master. */
#define I2C_MODE_MASTER			(1 << 4)

/*
 * I2C_MSG_* are I2C Message flags.
 */

/** Write message to I2C bus. */
#define I2C_MSG_WRITE			(0 << 0)

/** Read message from I2C bus. */
#define I2C_MSG_READ			(1 << 0)

/** @cond INTERNAL_HIDDEN */
#define I2C_MSG_RW_MASK			(1 << 0)
/** @endcond  */

/** Send STOP after this message. */
#define I2C_MSG_STOP			(1 << 1)

/** RESTART I2C transaction for this message. */
#define I2C_MSG_RESTART			(1 << 2)

/**
 * @brief One I2C Message.
 *
 * This defines one I2C message to transact on the I2C bus.
 */
struct i2c_msg {
	/** Data buffer in bytes */
	u8_t		*buf;

	/** Length of buffer in bytes */
	u32_t	len;

	/** Flags for this message */
	u8_t		flags;
};

union __deprecated dev_config {
	u32_t raw;
	struct __bits {
		u32_t        use_10_bit_addr : 1;
		u32_t        speed : 3;
		u32_t        is_master_device : 1;
		u32_t        reserved : 26;
	} bits;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
typedef int (*i2c_api_configure_t)(struct device *dev,
				   u32_t dev_config);
typedef int (*i2c_api_full_io_t)(struct device *dev,
				 struct i2c_msg *msgs,
				 u8_t num_msgs,
				 u16_t addr);

struct i2c_driver_api {
	i2c_api_configure_t configure;
	i2c_api_full_io_t transfer;
};
/**
 * @endcond
 */

/**
 * @brief Configure operation of a host controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_config Bit-packed 32-bit value to the device runtime configuration
 * for the I2C controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 */
__syscall int i2c_configure(struct device *dev, u32_t dev_config);

static inline int _impl_i2c_configure(struct device *dev, u32_t dev_config)
{
	const struct i2c_driver_api *api = dev->driver_api;

	return api->configure(dev, dev_config);
}

/**
 * @brief Perform data transfer to another I2C device.
 *
 * This routine provides a generic interface to perform data transfer
 * to another I2C device synchronously. Use i2c_read()/i2c_write()
 * for simple read or write.
 *
 * The array of message @a msgs must not be NULL.  The number of
 * message @a num_msgs may be zero,in which case no transfer occurs.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
__syscall int i2c_transfer(struct device *dev,
			   struct i2c_msg *msgs, u8_t num_msgs,
			   u16_t addr);

static inline int _impl_i2c_transfer(struct device *dev,
				     struct i2c_msg *msgs, u8_t num_msgs,
				     u16_t addr)
{
	const struct i2c_driver_api *api = dev->driver_api;

	return api->transfer(dev, msgs, num_msgs, addr);
}

/*
 * Derived i2c APIs -- all implemented in terms of i2c_transfer()
 */

/**
 * @brief Write a set amount of data to an I2C device.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes to write.
 * @param addr Address to the target I2C device for writing.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_write(struct device *dev, u8_t *buf,
			    u32_t num_bytes, u16_t addr)
{
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, &msg, 1, addr);
}

/**
 * @brief Read a set amount of data from an I2C device.
 *
 * This routine reads a set amount of data synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes to read.
 * @param addr Address of the I2C device being read.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_read(struct device *dev, u8_t *buf,
			   u32_t num_bytes, u16_t addr)
{
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, &msg, 1, addr);
}


/**
 * @brief Read multiple bytes from an internal address of an I2C device.
 *
 * This routine reads multiple bytes from an internal address of an
 * I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param start_addr Internal address from which the data is being read.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes being read.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_burst_read(struct device *dev, u16_t dev_addr,
				 u8_t start_addr, u8_t *buf,
				 u8_t num_bytes)
{
	struct i2c_msg msg[2];

	msg[0].buf = &start_addr;
	msg[0].len = 1;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Write multiple bytes to an internal address of an I2C device.
 *
 * This routine writes multiple bytes to an internal address of an
 * I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param start_addr Internal address to which the data is being written.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes being written.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_burst_write(struct device *dev, u16_t dev_addr,
				  u8_t start_addr, u8_t *buf,
				  u8_t num_bytes)
{
	struct i2c_msg msg[2];

	msg[0].buf = &start_addr;
	msg[0].len = 1;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Read internal register of an I2C device.
 *
 * This routine reads the value of an 8-bit internal register of an I2C
 * device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param reg_addr Address of the internal register being read.
 * @param value Memory pool that stores the retrieved register value.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_read_byte(struct device *dev, u16_t dev_addr,
				    u8_t reg_addr, u8_t *value)
{
	return i2c_burst_read(dev, dev_addr, reg_addr, value, 1);
}

/**
 * @brief Write internal register of an I2C device.
 *
 * This routine writes a value to an 8-bit internal register of an I2C
 * device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_byte(struct device *dev, u16_t dev_addr,
				     u8_t reg_addr, u8_t value)
{
	u8_t tx_buf[2] = {reg_addr, value};

	return i2c_write(dev, tx_buf, 2, dev_addr);
}

/**
 * @brief Update internal register of an I2C device.
 *
 * This routine updates the value of a set of bits from an 8-bit internal
 * register of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for updating.
 * @param reg_addr Address of the internal register being updated.
 * @param mask Bitmask for updating internal register.
 * @param value Value for updating internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_update_byte(struct device *dev, u8_t dev_addr,
				      u8_t reg_addr, u8_t mask,
				      u8_t value)
{
	u8_t old_value, new_value;
	int rc;

	rc = i2c_reg_read_byte(dev, dev_addr, reg_addr, &old_value);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return i2c_reg_write_byte(dev, dev_addr, reg_addr, new_value);
}

/**
 * @brief Read multiple bytes from an internal 16 bit address of an I2C device.
 *
 * This routine reads multiple bytes from a 16 bit internal address of an
 * I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param start_addr Internal 16 bit address from which the data is being read.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes being read.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_burst_read16(struct device *dev, u16_t dev_addr,
				   u16_t start_addr, u8_t *buf,
				   u8_t num_bytes)
{
	u8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Write multiple bytes to a 16 bit internal address of an I2C device.
 *
 * This routine writes multiple bytes to a 16 bit internal address of an
 * I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param start_addr Internal 16 bit address to which the data is being written.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes being written.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_burst_write16(struct device *dev, u16_t dev_addr,
				    u16_t start_addr, u8_t *buf,
				    u8_t num_bytes)
{
	u8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Read internal 16 bit address register of an I2C device.
 *
 * This routine reads the value of an 16-bit internal register of an I2C
 * device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param reg_addr 16 bit address of the internal register being read.
 * @param value Memory pool that stores the retrieved register value.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_reg_read16(struct device *dev, u16_t dev_addr,
				 u16_t reg_addr, u8_t *value)
{
	return i2c_burst_read16(dev, dev_addr, reg_addr, value, 1);
}

/**
 * @brief Write internal 16 bit address register of an I2C device.
 *
 * This routine writes a value to an 16-bit internal register of an I2C
 * device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr 16 bit address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_reg_write16(struct device *dev, u16_t dev_addr,
				  u16_t reg_addr, u8_t value)
{
	return i2c_burst_write16(dev, dev_addr, reg_addr, &value, 1);
}

/**
 * @brief Update internal 16 bit address register of an I2C device.
 *
 * This routine updates the value of a set of bits from a 16-bit internal
 * register of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for updating.
 * @param reg_addr 16 bit address of the internal register being updated.
 * @param mask Bitmask for updating internal register.
 * @param value Value for updating internal register.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_reg_update16(struct device *dev, u16_t dev_addr,
				   u16_t reg_addr, u8_t mask,
				   u8_t value)
{
	u8_t old_value, new_value;
	int rc;

	rc = i2c_reg_read16(dev, dev_addr, reg_addr, &old_value);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return i2c_reg_write16(dev, dev_addr, reg_addr, new_value);
}

/**
 * @brief Read multiple bytes from an internal variable byte size
 * address of an I2C device.
 *
 * This routine reads multiple bytes from an addr_size byte internal
 * address of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param start_addr Array to an internal register address from which
 *		     the data is being read.
 * @param addr_size Size in bytes of the register address.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes being read.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_burst_read_addr(struct device *dev, u16_t dev_addr,
				      u8_t *start_addr,
				      const u8_t addr_size,
				      u8_t *buf, u8_t num_bytes)
{
	struct i2c_msg msg[2];

	msg[0].buf = start_addr;
	msg[0].len = addr_size;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Write multiple bytes to an internal variable bytes size
 * address of an I2C device.
 * This routine writes multiple bytes to an addr_size byte internal
 * address of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param start_addr Array to an internal register address from which
 *		     the data is being read.
 * @param addr_size Size in bytes of the register address.
 * @param buf Memory pool from which the data is transferred.
 * @param num_bytes Number of bytes being written.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_burst_write_addr(struct device *dev, u16_t dev_addr,
				       u8_t *start_addr,
				       const u8_t addr_size,
				       u8_t *buf, u8_t num_bytes)
{
	struct i2c_msg msg[2];

	msg[0].buf = start_addr;
	msg[0].len = addr_size;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, dev_addr);
}

/**
 * @brief Read internal variable byte size address register
 * of an I2C device.
 *
 * This routine reads the value of an addr_size byte internal register
 * of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param reg_addr Array to an internal register address from which
 *		     the data is being read.
 * @param addr_size Size in bytes of the register address.
 * @param value Memory pool that stores the retrieved register value.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_reg_read_addr(struct device *dev,
				    u16_t dev_addr,
				    u8_t *reg_addr,
				    const u8_t addr_size,
				    u8_t *value)
{
	return i2c_burst_read_addr(dev, dev_addr, reg_addr,
				   addr_size, value, 1);
}

/**
 * @brief Write internal variable byte size address register
 * of an I2C device.
 *
 * This routine writes a value to an addr_size byte internal register
 * of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr Array to an internal register address from which
 *		     the data is being read.
 * @param addr_size Size in bytes of the register address.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_reg_write_addr(struct device *dev,
				     u16_t dev_addr,
				     u8_t *reg_addr,
				     const u8_t addr_size,
				     u8_t value)
{
	return i2c_burst_write_addr(dev, dev_addr, reg_addr,
				    addr_size, &value, 1);
}

/**
 * @brief Update internal variable byte size address register
 * of an I2C device.
 *
 * This routine updates the value of a set of bits from a addr_size byte
 * internal register of an I2C device synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for updating.
 * @param reg_addr Array to an internal register address from which
 *		     the data is being read.
 * @param addr_size Size in bytes of the register address.
 * @param mask Bitmask for updating internal register.
 * @param value Value for updating internal register.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_reg_update_addr(struct device *dev,
				      u16_t dev_addr,
				      u8_t *reg_addr,
				      u8_t addr_size,
				      u8_t mask,
				      u8_t value)
{
	u8_t old_value, new_value;
	int rc;

	rc = i2c_reg_read_addr(dev, dev_addr, reg_addr,
			       addr_size, &old_value);
	if (rc != 0) {
		return rc;
	}

	new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return i2c_reg_write_addr(dev, dev_addr, reg_addr,
				  addr_size, new_value);
}

struct i2c_client_config {
	char *i2c_master;
	u16_t i2c_addr;
};

#define I2C_DECLARE_CLIENT_CONFIG	struct i2c_client_config i2c_client

#define I2C_CLIENT(_master, _addr)		\
	.i2c_client = {				\
		.i2c_master = (_master),	\
		.i2c_addr = (_addr),		\
	}

#define I2C_GET_MASTER(_conf)		((_conf)->i2c_client.i2c_master)
#define I2C_GET_ADDR(_conf)		((_conf)->i2c_client.i2c_addr)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/i2c.h>

#endif /* __DRIVERS_I2C_H */
