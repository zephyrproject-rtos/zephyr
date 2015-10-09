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
#define I2C_SPEED_STANDARD		(0x1)
#define I2C_SPEED_FAST			(0x2)
#define I2C_SPEED_FAST_PLUS		(0x3)
#define I2C_SPEED_HIGH			(0x4)
#define I2C_SPEED_ULTRA			(0x5)

/* I2C dev_config bitfields */
#define I2C_ADDR_10_BITS		(1 << 0)
#define I2C_SPEED_MASK			(0x7 << 1)	/* 3 bits */
#define I2C_MODE_MASTER			(1 << 4)
#define I2C_MODE_SLAVE_READ		(1 << 5)


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

enum i2c_cb_type {
	I2C_CB_WRITE            = 1,
	I2C_CB_READ             = 2,
	I2C_CB_ERROR		= 3,
};

typedef void (*i2c_callback)(struct device *dev,
			     uint32_t cb_type);

typedef int (*i2c_api_configure_t)(struct device *dev,
				   uint32_t dev_config);
typedef int (*i2c_api_set_callback_t)(struct device *dev,
				      i2c_callback cb);
typedef int (*i2c_api_full_io_t)(struct device *dev,
				 uint8_t *tx_buf,
				 uint32_t tx_len,
				 uint8_t *rx_buf,
				 uint32_t rx_len,
				 uint16_t addr,
				 uint32_t ctrl_flags);
typedef int (*i2c_api_suspend_t)(struct device *dev);
typedef int (*i2c_api_resume_t)(struct device *dev);

struct i2c_driver_api {
	i2c_api_configure_t configure;
	i2c_api_set_callback_t set_callback;
	i2c_api_full_io_t transfer;
	i2c_api_full_io_t poll_transfer;
	i2c_api_suspend_t suspend;
	i2c_api_resume_t resume;
};

/**
 * @brief Configure a host controllers operation
 * @param dev Pointer to the device structure for the driver instance
 * @param dev_config Bit-packed 32-bit value to the device runtime configuration
 *                   for the I2C controller.
 *
 * @return DEV_OK if successful, another DEV_* code otherwise.
 */
static inline int i2c_configure(struct device *dev, uint32_t dev_config)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->configure(dev, dev_config);
}

/**
 * @brief Set an application callback
 * @param dev Pointer to the device structure for the driver instance
 * @param cb A valid pointer to a callback, or NULL
 *
 * @return DEV_OK if successful, another DEV_* code otherwise.
 */
static inline int i2c_set_callback(struct device *dev, i2c_callback cb)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->set_callback(dev, cb);
}

/**
 * @brief Configure a host controllers operation
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred from
 * @param len Size of the memory pool available for reading from
 * @param addr Address of the I2C device to write to
 *
 * @return DEV_OK if successful, another DEV_* code otherwise.
 */
static inline int i2c_write(struct device *dev, uint8_t *buf,
		     uint32_t len, uint16_t addr)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->transfer(dev, buf, len, 0, 0, addr, 0);
}

/**
 * @brief Read a set amount of data from an I2C driver
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred to
 * @param len Size of the memory pool available for writing to
 * @param addr Address of the I2C device to read from
 *
 * @return DEV_OK if successful, another DEV_* code otherwise.
 */
static inline int i2c_read(struct device *dev, uint8_t *buf,
		    uint32_t len, uint16_t addr)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->transfer(dev, 0, 0, buf, len, addr, 0);
}

/**
 * @brief Provide an interrupt free write as master
 *
 * This is for situation where transactions are guaranteed to finish
 * before returning.
 *
 * Note that this is only for I2C device acting as master.
 * Slave mode is currently not supported.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param buf Memory pool that data should be transferred from
 * @param len Size of the memory pool available for reading from
 * @param addr Address of the I2C device to perform transfer
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static inline int i2c_polling_write(struct device *dev, uint8_t *buf,
				    uint32_t len, uint16_t addr)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->poll_transfer(dev, buf, len, 0, 0, addr, 0);
}

/**
 * @brief Performs data transfer to another I2C device
 *
 * This provides a generic interface to perform data transfer
 * to another I2C device. If a simple read or write is needed,
 * use i2c_read()/i2c_write() instead.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param tx_buf Memory pool that data should be transferred from
 * @param tx_len Size of the memory pool available for reading from
 * @param rx_buf Memory pool that data should be transferred to
 * @param rx_len Size of the memory pool available for writing to
 * @param addr Address of the I2C target device
 * @param ctrl_flags Control flags for this transfer
 *
 * @return DEV_OK if successful, another DEV_* code otherwise.
 */
static inline int i2c_transfer(struct device *dev,
			       uint8_t *tx_buf, uint32_t tx_len,
			       uint8_t *rx_buf, uint32_t rx_len,
			       uint16_t addr, uint32_t ctrl_flags)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->transfer(dev, tx_buf, tx_len,
			     rx_buf, rx_len, addr, ctrl_flags);
}

/**
 * @brief Performs interrupt free data transfer
 *
 * This provides a generic interface to perform data transfer
 * to another I2C device. This is for situation where transactions
 * are guaranteed to finish before returning.
 *
 * Note that this is only for I2C device acting as master.
 * Slave mode is currently not supported.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param tx_buf Memory pool that data should be transferred from
 * @param tx_len Size of the memory pool available for reading from
 * @param rx_buf Memory pool that data should be transferred to
 * @param rx_len Size of the memory pool available for writing to
 * @param addr Address of the I2C target device
 * @param ctrl_flags Control flags for this transfer
 *
 * @return DEV_OK if successful, another DEV_* code otherwise.
 */
static inline int i2c_poll_transfer(struct device *dev,
				    uint8_t *tx_buf, uint32_t tx_len,
				    uint8_t *rx_buf, uint32_t rx_len,
				    uint16_t addr, uint32_t ctrl_flags)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->poll_transfer(dev, tx_buf, tx_len,
				  rx_buf, rx_len, addr, ctrl_flags);
}

/**
 * @brief Suspend an I2C driver
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return DEV_OK
 */
static inline int i2c_suspend(struct device *dev)
{
	struct i2c_driver_api *api;

	api = (struct i2c_driver_api *)dev->driver_api;
	return api->suspend(dev);
}

/**
 * @brief Resume an I2C driver
 * @param dev Pointer to the device structure for the driver instance
 *
 * @return DEV_OK
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

#endif /* __DRIVERS_I2C_H */
