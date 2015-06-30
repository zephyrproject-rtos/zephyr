/* i2c.h - public I2C driver API */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __DRIVERS_I2C_H
#define __DRIVERS_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <device.h>

/* I2C speeds */
#define I2C_STANDARD			0x00000001
#define I2C_FAST			0x00000002
#define I2C_FAST_PLUS			0x00000003
#define I2C_SPEED_HIGH			0x00000004
#define I2C_SPEED_ULTRA			0x00000005

/* I2C addressing modes */
#define I2C_ADDR_7_BITS			(0 << 4)
#define I2C_ADDR_10_BITS		(1 << 4)

/* I2C mode types */
#define I2C_MODE_MASTER			(0 << 5)
#define I2C_MODE_SLAVE			(1 << 5)
#define I2C_ADDR_MASK			(0xFF1 << 6)
#define I2C_ADDR_GET(_in_) ((I2C_ADDR_MASK & _in_) >> 6)

/* application callback function signature */
typedef void (*i2c_callback)(struct device *dev, uint32_t context);

struct i2c_rom_config {
	uint32_t        base_address;
	uint32_t        interrupt_vector;
	uint32_t        interrupt_mask;
};

/*
 * dev_config is a bit field with the following break down:
 * speed		[0 : 3 ] - from the I2C speeds
 * address_mode		[ 4 ]    - from the I2C addressing modes
 * mode_type		[ 5 ]    - from the I2C mode types
 * slave_address	[ 6 : 15 ]
 * RESERVED             [ 16: 31 ] - not yet defined for use
 */
struct i2c_config {
	uint32_t       dev_config;
	i2c_callback   cb_receive;
	i2c_callback   cb_transmit;
};

typedef int (*i2c_api_configure)(struct device *dev, struct i2c_config *config);
typedef int (*i2c_api_io)(struct device *dev, unsigned char *buf, uint32_t len);
typedef int (*i2c_api_suspend)(struct device *dev);
typedef int (*i2c_api_resume)(struct device *dev);

struct i2c_driver_api {
	i2c_api_configure configure;
	i2c_api_io read;
	i2c_api_io write;
	i2c_api_suspend suspend;
	i2c_api_resume resume;
};

/*!
 * @brief Configure a host controllers operation
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to the application provided configuration
 */
inline int i2c_configure(struct device *dev, struct i2c_config *config)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->configure(dev, config);
}

/*!
 * @brief Configure a host controllers operation
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred from
 * @param len Size of the memory pool available for reading from
 */
inline int i2c_write(struct device *dev, unsigned char *buf, uint32_t len)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->write(dev, buf, len);
}

/*!
 * @brief Read a set amount of data from an I2C driver
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred to
 * @param len Size of the memory pool available for writing to
 */
inline int i2c_read(struct device *dev, unsigned char *buf, uint32_t len)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->read(dev, buf, len);
}

/*!
 * @brief Suspend an I2C driver
 * @param dev Pointer to the device structure for the driver instance
 */
inline int i2c_suspend(struct device *dev)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->suspend(dev);
}

/*!
 * @brief Resume an I2C driver
 * @param dev Pointer to the device structure for the driver instance
 */
inline int i2c_resume(struct device *dev)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->resume(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_I2C_H */
