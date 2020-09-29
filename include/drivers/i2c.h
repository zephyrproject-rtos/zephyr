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
#ifndef ZEPHYR_INCLUDE_DRIVERS_I2C_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2C_H_

/**
 * @brief I2C Interface
 * @defgroup i2c_interface I2C Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following #defines are used to configure the I2C controller.
 */

/** I2C Standard Speed: 100 kHz */
#define I2C_SPEED_STANDARD		(0x1U)

/** I2C Fast Speed: 400 kHz */
#define I2C_SPEED_FAST			(0x2U)

/** I2C Fast Plus Speed: 1 MHz */
#define I2C_SPEED_FAST_PLUS		(0x3U)

/** I2C High Speed: 3.4 MHz */
#define I2C_SPEED_HIGH			(0x4U)

/** I2C Ultra Fast Speed: 5 MHz */
#define I2C_SPEED_ULTRA			(0x5U)

#define I2C_SPEED_SHIFT			(1U)
#define I2C_SPEED_SET(speed)		(((speed) << I2C_SPEED_SHIFT) \
						& I2C_SPEED_MASK)
#define I2C_SPEED_MASK			(0x7U << I2C_SPEED_SHIFT) /* 3 bits */
#define I2C_SPEED_GET(cfg) 		(((cfg) & I2C_SPEED_MASK) \
						>> I2C_SPEED_SHIFT)

/** Use 10-bit addressing. DEPRECATED - Use I2C_MSG_ADDR_10_BITS instead. */
#define I2C_ADDR_10_BITS		BIT(0)

/** Controller to act as Master. */
#define I2C_MODE_MASTER			BIT(4)

/*
 * The following #defines are used to configure the I2C slave device
 */

/** Slave device responds to 10-bit addressing. */
#define I2C_SLAVE_FLAGS_ADDR_10_BITS	BIT(0)

/*
 * I2C_MSG_* are I2C Message flags.
 */

/** Write message to I2C bus. */
#define I2C_MSG_WRITE			(0U << 0U)

/** Read message from I2C bus. */
#define I2C_MSG_READ			BIT(0)

/** @cond INTERNAL_HIDDEN */
#define I2C_MSG_RW_MASK			BIT(0)
/** @endcond  */

/** Send STOP after this message. */
#define I2C_MSG_STOP			BIT(1)

/** RESTART I2C transaction for this message.
 *
 * @note Not all I2C drivers have or require explicit support for this
 * feature. Some drivers require this be present on a read message
 * that follows a write, or vice-versa.  Some drivers will merge
 * adjacent fragments into a single transaction using this flag; some
 * will not. */
#define I2C_MSG_RESTART			BIT(2)

/** Use 10-bit addressing for this message.
 *
 * @note Not all SoC I2C implementations support this feature. */
#define I2C_MSG_ADDR_10_BITS		BIT(3)

/**
 * @brief One I2C Message.
 *
 * This defines one I2C message to transact on the I2C bus.
 *
 * @note Some of the configurations supported by this API may not be
 * supported by specific SoC I2C hardware implementations, in
 * particular features related to bus transactions intended to read or
 * write data from different buffers within a single transaction.
 * Invocations of i2c_transfer() may not indicate an error when an
 * unsupported configuration is encountered.  In some cases drivers
 * will generate separate transactions for each message fragment, with
 * or without presence of @ref I2C_MSG_RESTART in #flags.
 */
struct i2c_msg {
	/** Data buffer in bytes */
	uint8_t		*buf;

	/** Length of buffer in bytes */
	uint32_t	len;

	/** Flags for this message */
	uint8_t		flags;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in
 * public documentation.
 */
struct i2c_slave_config;

typedef int (*i2c_slave_write_requested_cb_t)(
		struct i2c_slave_config *config);
typedef int (*i2c_slave_read_requested_cb_t)(
		struct i2c_slave_config *config, uint8_t *val);
typedef int (*i2c_slave_write_received_cb_t)(
		struct i2c_slave_config *config, uint8_t val);
typedef int (*i2c_slave_read_processed_cb_t)(
		struct i2c_slave_config *config, uint8_t *val);
typedef int (*i2c_slave_stop_cb_t)(struct i2c_slave_config *config);

struct i2c_slave_callbacks {
	/** callback function being called when write is requested */
	i2c_slave_write_requested_cb_t write_requested;
	/** callback function being called when read is requested */
	i2c_slave_read_requested_cb_t read_requested;
	/** callback function being called when byte has been received */
	i2c_slave_write_received_cb_t write_received;
	/** callback function being called when byte has been sent */
	i2c_slave_read_processed_cb_t read_processed;
	/** callback function being called when stop occurs on the bus */
	i2c_slave_stop_cb_t stop;
};

struct i2c_slave_config {
	/** Private, do not modify */
	sys_snode_t node;
	/** Flags for the slave device defined by I2C_SLAVE_FLAGS_* constants */
	uint8_t flags;
	/** Address for this slave device */
	uint16_t address;
	/** Callback functions */
	const struct i2c_slave_callbacks *callbacks;
};

typedef int (*i2c_api_configure_t)(const struct device *dev,
				   uint32_t dev_config);
typedef int (*i2c_api_full_io_t)(const struct device *dev,
				 struct i2c_msg *msgs,
				 uint8_t num_msgs,
				 uint16_t addr);
typedef int (*i2c_api_slave_register_t)(const struct device *dev,
					struct i2c_slave_config *cfg);
typedef int (*i2c_api_slave_unregister_t)(const struct device *dev,
					  struct i2c_slave_config *cfg);
typedef int (*i2c_api_recover_bus_t)(const struct device *dev);

__subsystem struct i2c_driver_api {
	i2c_api_configure_t configure;
	i2c_api_full_io_t transfer;
	i2c_api_slave_register_t slave_register;
	i2c_api_slave_unregister_t slave_unregister;
	i2c_api_recover_bus_t recover_bus;
};

typedef int (*i2c_slave_api_register_t)(const struct device *dev);
typedef int (*i2c_slave_api_unregister_t)(const struct device *dev);

struct i2c_slave_driver_api {
	i2c_slave_api_register_t driver_register;
	i2c_slave_api_unregister_t driver_unregister;
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
__syscall int i2c_configure(const struct device *dev, uint32_t dev_config);

static inline int z_impl_i2c_configure(const struct device *dev,
				       uint32_t dev_config)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

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
 * @note Not all scatter/gather transactions can be supported by all
 * drivers.  As an example, a gather write (multiple consecutive
 * `i2c_msg` buffers all configured for `I2C_MSG_WRITE`) may be packed
 * into a single transaction by some drivers, but others may emit each
 * fragment as a distinct write transaction, which will not produce
 * the same behavior.  See the documentation of `struct i2c_msg` for
 * limitations on support for multi-message bus transactions.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param msgs Array of messages to transfer.
 * @param num_msgs Number of messages to transfer.
 * @param addr Address of the I2C target device.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
__syscall int i2c_transfer(const struct device *dev,
			   struct i2c_msg *msgs, uint8_t num_msgs,
			   uint16_t addr);

static inline int z_impl_i2c_transfer(const struct device *dev,
				      struct i2c_msg *msgs, uint8_t num_msgs,
				      uint16_t addr)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	return api->transfer(dev, msgs, num_msgs, addr);
}

/**
 * @brief Recover the I2C bus
 *
 * Attempt to recover the I2C bus.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @retval 0 If successful
 * @retval -EBUSY If bus is not clear after recovery attempt.
 * @retval -EIO General input / output error.
 * @retval -ENOTSUP If bus recovery is not supported
 */
__syscall int i2c_recover_bus(const struct device *dev);

static inline int z_impl_i2c_recover_bus(const struct device *dev)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	if (api->recover_bus == NULL) {
		return -ENOTSUP;
	}

	return api->recover_bus(dev);
}

/**
 * @brief Registers the provided config as Slave device
 *
 * Enable I2C slave mode for the 'dev' I2C bus driver using the provided
 * 'config' struct containing the functions and parameters to send bus
 * events. The I2C slave will be registered at the address provided as 'address'
 * struct member. Addressing mode - 7 or 10 bit - depends on the 'flags'
 * struct member. Any I2C bus events related to the slave mode will be passed
 * onto I2C slave device driver via a set of callback functions provided in
 * the 'callbacks' struct member.
 *
 * Most of the existing hardware allows simultaneous support for master
 * and slave mode. This is however not guaranteed.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Config struct with functions and parameters used by the I2C driver
 * to send bus events
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error.
 * @retval -ENOTSUP If slave mode is not supported
 */
static inline int i2c_slave_register(const struct device *dev,
				     struct i2c_slave_config *cfg)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	if (api->slave_register == NULL) {
		return -ENOTSUP;
	}

	return api->slave_register(dev, cfg);
}

/**
 * @brief Unregisters the provided config as Slave device
 *
 * This routine disables I2C slave mode for the 'dev' I2C bus driver using
 * the provided 'config' struct containing the functions and parameters
 * to send bus events.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Config struct with functions and parameters used by the I2C driver
 * to send bus events
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -ENOTSUP If slave mode is not supported
 */
static inline int i2c_slave_unregister(const struct device *dev,
				       struct i2c_slave_config *cfg)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->api;

	if (api->slave_unregister == NULL) {
		return -ENOTSUP;
	}

	return api->slave_unregister(dev, cfg);
}

/**
 * @brief Instructs the I2C Slave device to register itself to the I2C Controller
 *
 * This routine instructs the I2C Slave device to register itself to the I2C
 * Controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error.
 */
__syscall int i2c_slave_driver_register(const struct device *dev);

static inline int z_impl_i2c_slave_driver_register(const struct device *dev)
{
	const struct i2c_slave_driver_api *api =
		(const struct i2c_slave_driver_api *)dev->api;

	return api->driver_register(dev);
}

/**
 * @brief Instructs the I2C Slave device to unregister itself from the I2C
 * Controller
 *
 * This routine instructs the I2C Slave device to unregister itself from the I2C
 * Controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 */
__syscall int i2c_slave_driver_unregister(const struct device *dev);

static inline int z_impl_i2c_slave_driver_unregister(const struct device *dev)
{
	const struct i2c_slave_driver_api *api =
		(const struct i2c_slave_driver_api *)dev->api;

	return api->driver_unregister(dev);
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
static inline int i2c_write(const struct device *dev, const uint8_t *buf,
			    uint32_t num_bytes, uint16_t addr)
{
	struct i2c_msg msg;

	msg.buf = (uint8_t *)buf;
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
static inline int i2c_read(const struct device *dev, uint8_t *buf,
			   uint32_t num_bytes, uint16_t addr)
{
	struct i2c_msg msg;

	msg.buf = buf;
	msg.len = num_bytes;
	msg.flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, &msg, 1, addr);
}

/**
 * @brief Write then read data from an I2C device.
 *
 * This supports the common operation "this is what I want", "now give
 * it to me" transaction pair through a combined write-then-read bus
 * transaction.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param addr Address of the I2C device
 * @param write_buf Pointer to the data to be written
 * @param num_write Number of bytes to write
 * @param read_buf Pointer to storage for read data
 * @param num_read Number of bytes to read
 *
 * @retval 0 if successful
 * @retval negative on error.
 */
static inline int i2c_write_read(const struct device *dev, uint16_t addr,
				 const void *write_buf, size_t num_write,
				 void *read_buf, size_t num_read)
{
	struct i2c_msg msg[2];

	msg[0].buf = (uint8_t *)write_buf;
	msg[0].len = num_write;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)read_buf;
	msg[1].len = num_read;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(dev, msg, 2, addr);
}

/**
 * @brief Read multiple bytes from an internal address of an I2C device.
 *
 * This routine reads multiple bytes from an internal address of an
 * I2C device synchronously.
 *
 * Instances of this may be replaced by i2c_write_read().

 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for reading.
 * @param start_addr Internal address from which the data is being read.
 * @param buf Memory pool that stores the retrieved data.
 * @param num_bytes Number of bytes being read.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_burst_read(const struct device *dev,
				 uint16_t dev_addr,
				 uint8_t start_addr,
				 uint8_t *buf,
				 uint32_t num_bytes)
{
	return i2c_write_read(dev, dev_addr,
			      &start_addr, sizeof(start_addr),
			      buf, num_bytes);
}

/**
 * @brief Write multiple bytes to an internal address of an I2C device.
 *
 * This routine writes multiple bytes to an internal address of an
 * I2C device synchronously.
 *
 * @warning The combined write synthesized by this API may not be
 * supported on all I2C devices.  Uses of this API may be made more
 * portable by replacing them with calls to i2c_write() passing a
 * buffer containing the combined address and data.
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
static inline int i2c_burst_write(const struct device *dev,
				  uint16_t dev_addr,
				  uint8_t start_addr,
				  const uint8_t *buf,
				  uint32_t num_bytes)
{
	struct i2c_msg msg[2];

	msg[0].buf = &start_addr;
	msg[0].len = 1U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)buf;
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
static inline int i2c_reg_read_byte(const struct device *dev,
				    uint16_t dev_addr,
				    uint8_t reg_addr, uint8_t *value)
{
	return i2c_write_read(dev, dev_addr,
			      &reg_addr, sizeof(reg_addr),
			      value, sizeof(*value));
}

/**
 * @brief Write internal register of an I2C device.
 *
 * This routine writes a value to an 8-bit internal register of an I2C
 * device synchronously.
 *
 * @note This function internally combines the register and value into
 * a single bus transaction.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dev_addr Address of the I2C device for writing.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_byte(const struct device *dev,
				     uint16_t dev_addr,
				     uint8_t reg_addr, uint8_t value)
{
	uint8_t tx_buf[2] = {reg_addr, value};

	return i2c_write(dev, tx_buf, 2, dev_addr);
}

/**
 * @brief Update internal register of an I2C device.
 *
 * This routine updates the value of a set of bits from an 8-bit internal
 * register of an I2C device synchronously.
 *
 * @note If the calculated new register value matches the value that
 * was read this function will not generate a write operation.
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
static inline int i2c_reg_update_byte(const struct device *dev,
				      uint8_t dev_addr,
				      uint8_t reg_addr, uint8_t mask,
				      uint8_t value)
{
	uint8_t old_value, new_value;
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
 * @brief Dump out an I2C message
 *
 * Dumps out a list of I2C messages. For any that are writes (W), the data is
 * displayed in hex.
 *
 * It looks something like this (with name "testing"):
 *
 * D: I2C msg: testing, addr=56
 * D:    W len=01:
 * D: contents:
 * D: 06                      |.
 * D:    W len=0e:
 * D: contents:
 * D: 00 01 02 03 04 05 06 07 |........
 * D: 08 09 0a 0b 0c 0d       |......
 *
 * @param name Name of this dump, displayed at the top.
 * @param msgs Array of messages to dump.
 * @param num_msgs Number of messages to dump.
 * @param addr Address of the I2C target device.
 */
void i2c_dump_msgs(const char *name, const struct i2c_msg *msgs,
		   uint8_t num_msgs, uint16_t addr);

struct i2c_client_config {
	char *i2c_master;
	uint16_t i2c_addr;
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

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2C_H_ */
