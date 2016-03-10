/**
 * @file
 *
 * @brief Public APIs for the I2C drivers.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include <stdint.h>
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

/** For internal use. */
#define I2C_SPEED_MASK			(0x7 << 1)	/* 3 bits */

/** Use 10-bit addressing. */
#define I2C_ADDR_10_BITS		(1 << 0)

/** Controller to act as Master. */
#define I2C_MODE_MASTER			(1 << 4)

/** Controller to act as Slave. */
#define I2C_MODE_SLAVE_READ		(1 << 5)


/*
 * I2C_MSG_* are I2C Message flags.
 */

/** Write message to I2C bus. */
#define I2C_MSG_WRITE			(0 << 0)

/** Read message from I2C bus. */
#define I2C_MSG_READ			(1 << 0)

/** For internal use. */
#define I2C_MSG_RW_MASK			(1 << 0)

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
	uint8_t		*buf;

	/** Length of buffer in bytes */
	uint32_t	len;

	/** Flags for this message */
	uint8_t		flags;

	uint8_t		stride[3];
};

union dev_config {
	uint32_t raw;
	struct {
		uint32_t        use_10_bit_addr : 1;
		uint32_t        speed : 3;
		uint32_t        is_master_device : 1;
		uint32_t        is_slave_read : 1;
		uint32_t        reserved : 26;
	} bits;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
typedef int (*i2c_api_configure_t)(struct device *dev,
				   uint32_t dev_config);
typedef int (*i2c_api_full_io_t)(struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs,
				 uint16_t addr);
typedef int (*i2c_api_suspend_t)(struct device *dev);
typedef int (*i2c_api_resume_t)(struct device *dev);

struct i2c_driver_api {
	i2c_api_configure_t configure;
	i2c_api_full_io_t transfer;
	i2c_api_suspend_t suspend;
	i2c_api_resume_t resume;
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
 * @retval Negative errno code if failure.
 */
static inline int i2c_configure(struct device *dev, uint32_t dev_config)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->configure(dev, dev_config);
}

/**
 * @brief Write a set amount of data to an I2C device.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Memory pool from which the data is transferred.
 * @param len Size of the memory pool available for reading.
 * @param addr Address to the target I2C device for writing.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_write(struct device *dev, uint8_t *buf,
			    uint32_t len, uint16_t addr)
{
	struct i2c_driver_api *api;
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = len;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->transfer(dev, &msg, 1, addr);
}

/**
 * @brief Read a set amount of data from an I2C device.
 *
 * This routine reads a set amount of data synchronously.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Memory pool that stores the retrieved data.
 * @param len Size of the memory pool available for writing.
 * @param addr Address of the I2C device being read.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_read(struct device *dev, uint8_t *buf,
			   uint32_t len, uint16_t addr)
{
	struct i2c_driver_api *api;
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = len;
	msg.flags = I2C_MSG_READ | I2C_MSG_STOP;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->transfer(dev, &msg, 1, addr);
}

/**
 * @brief Perform data transfer to another I2C device.
 *
 * This routine provides a generic interface to perform data transfer
 * to another I2C device synchronously. Use i2c_read()/i2c_write()
 * for simple read or write.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int i2c_transfer(struct device *dev,
			       struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->transfer(dev, msgs, num_msgs, addr);
}

/**
 * @brief Suspend an I2C driver.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
static inline int i2c_suspend(struct device *dev)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->suspend(dev);
}

/**
 * @brief Resume an I2C driver.
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 */
static inline int i2c_resume(struct device *dev)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->resume(dev);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* __DRIVERS_I2C_H */
