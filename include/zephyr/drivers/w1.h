/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public 1-Wire Driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_W1_H_
#define ZEPHYR_INCLUDE_DRIVERS_W1_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 1-Wire Interface
 * @defgroup w1_interface 1-Wire Interface
 * @ingroup io_interfaces
 * @{
 */

/**  @cond INTERNAL_HIDDEN */

/*
 * Count the number of slaves expected on the bus.
 * This can be used to decide if the bus has a multidrop topology or
 * only a single slave is present.
 * There is a comma after each ordinal (including the last)
 * Hence FOR_EACH adds "+1" once too often which has to be subtracted in the end.
 */
#define F1(x) 1
#define W1_SLAVE_COUNT(node_id) \
		(FOR_EACH(F1, (+), DT_SUPPORTS_DEP_ORDS(node_id)) - 1)
#define W1_INST_SLAVE_COUNT(inst)  \
		(W1_SLAVE_COUNT(DT_DRV_INST(inst)))

/** @endcond */

/**
 * @brief Defines the 1-Wire master settings types, which are runtime configurable.
 */
enum w1_settings_type {
	/** Overdrive speed is enabled in case a value of 1 is passed and
	 * disabled passing 0.
	 */
	W1_SETTING_SPEED,
	/**
	 * The strong pullup resistor is activated immediately after the next
	 * written data block by passing a value of 1, and deactivated passing 0.
	 */
	W1_SETTING_STRONG_PULLUP,

	/**
	 * Number of different settings types.
	 */
	W1_SETINGS_TYPE_COUNT,
};

/**  @cond INTERNAL_HIDDEN */

/** Configuration common to all 1-Wire master implementations. */
struct w1_master_config {
	/* Number of connected slaves */
	uint16_t slave_count;
};

/** Data common to all 1-Wire master implementations. */
struct w1_master_data {
	/* The mutex used by w1_lock_bus and w1_unlock_bus methods */
	struct k_mutex bus_lock;
};

typedef int (*w1_reset_bus_t)(const struct device *dev);
typedef int (*w1_read_bit_t)(const struct device *dev);
typedef int (*w1_write_bit_t)(const struct device *dev, bool bit);
typedef int (*w1_read_byte_t)(const struct device *dev);
typedef int (*w1_write_byte_t)(const struct device *dev, const uint8_t byte);
typedef int (*w1_read_block_t)(const struct device *dev, uint8_t *buffer,
			       size_t len);
typedef int (*w1_write_block_t)(const struct device *dev, const uint8_t *buffer,
				size_t len);
typedef size_t (*w1_get_slave_count_t)(const struct device *dev);
typedef int (*w1_configure_t)(const struct device *dev,
			      enum w1_settings_type type, uint32_t value);

__subsystem struct w1_driver_api {
	w1_reset_bus_t reset_bus;
	w1_read_bit_t read_bit;
	w1_write_bit_t write_bit;
	w1_read_byte_t read_byte;
	w1_write_byte_t write_byte;
	w1_read_block_t read_block;
	w1_write_block_t write_block;
	w1_configure_t configure;
};
/** @endcond */

/** @cond INTERNAL_HIDDEN */
__syscall int w1_change_bus_lock(const struct device *dev, bool lock);

static inline int z_impl_w1_change_bus_lock(const struct device *dev, bool lock)
{
	struct w1_master_data *ctrl_data = dev->data;

	if (lock) {
		return k_mutex_lock(&ctrl_data->bus_lock, K_FOREVER);
	} else {
		return k_mutex_unlock(&ctrl_data->bus_lock);
	}
}
/** @endcond */

/**
 * @brief Lock the 1-wire bus to prevent simultaneous access.
 *
 * This routine locks the bus to prevent simultaneous access from different
 * threads. The calling thread waits until the bus becomes available.
 * A thread is permitted to lock a mutex it has already locked.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval        0 If successful.
 * @retval -errno Negative error code on error.
 */
static inline int w1_lock_bus(const struct device *dev)
{
	return w1_change_bus_lock(dev, true);
}

/**
 * @brief Unlock the 1-wire bus.
 *
 * This routine unlocks the bus to permit access to bus line.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
static inline int w1_unlock_bus(const struct device *dev)
{
	return w1_change_bus_lock(dev, false);
}

/**
 * @brief 1-Wire data link layer
 * @defgroup w1_data_link 1-Wire data link layer
 * @ingroup w1_interface
 * @{
 */

/**
 * @brief Reset the 1-Wire bus to prepare slaves for communication.
 *
 * This routine resets all 1-Wire bus slaves such that they are
 * ready to receive a command.
 * Connected slaves answer with a presence pulse once they are ready
 * to receive data.
 *
 * In case the driver supports both standard speed and overdrive speed,
 * the reset routine takes care of sendig either a short or a long reset pulse
 * depending on the current state. The speed can be changed using
 * @a w1_configure().
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0      If no slaves answer with a present pulse.
 * @retval 1      If at least one slave answers with a present pulse.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_reset_bus(const struct device *dev);

static inline int z_impl_w1_reset_bus(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->reset_bus(dev);
}

/**
 * @brief Read a single bit from the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval rx_bit The read bit value on success.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_read_bit(const struct device *dev);

static inline int z_impl_w1_read_bit(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->read_bit(dev);
}

/**
 * @brief Write a single bit to the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param bit     Transmitting bit value 1 or 0.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_write_bit(const struct device *dev, const bool bit);

static inline int z_impl_w1_write_bit(const struct device *dev, bool bit)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->write_bit(dev, bit);
}

/**
 * @brief Read a single byte from the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval rx_byte The read byte value on success.
 * @retval -errno  Negative error code on error.
 */
__syscall int w1_read_byte(const struct device *dev);

static inline int z_impl_w1_read_byte(const struct device *dev)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->read_byte(dev);
}

/**
 * @brief Write a single byte to the 1-Wire bus.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param byte    Transmitting byte.
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_write_byte(const struct device *dev, uint8_t byte);

static inline int z_impl_w1_write_byte(const struct device *dev, uint8_t byte)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->write_byte(dev, byte);
}

/**
 * @brief Read a block of data from the 1-Wire bus.
 *
 * @param[in] dev     Pointer to the device structure for the driver instance.
 * @param[out] buffer Pointer to receive buffer.
 * @param len         Length of receiving buffer (in bytes).
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_read_block(const struct device *dev, uint8_t *buffer, size_t len);

/**
 * @brief Write a block of data from the 1-Wire bus.
 *
 * @param[in] dev    Pointer to the device structure for the driver instance.
 * @param[in] buffer Pointer to transmitting buffer.
 * @param len        Length of transmitting buffer (in bytes).
 *
 * @retval 0      If successful.
 * @retval -errno Negative error code on error.
 */
__syscall int w1_write_block(const struct device *dev,
			     const uint8_t *buffer, size_t len);

/**
 * @brief Get the number of slaves on the bus.
 *
 * @param[in] dev  Pointer to the device structure for the driver instance.
 *
 * @retval slave_count  Positive Number of connected 1-Wire slaves on success.
 * @retval -errno       Negative error code on error.
 */
__syscall size_t w1_get_slave_count(const struct device *dev);

static inline size_t z_impl_w1_get_slave_count(const struct device *dev)
{
	const struct w1_master_config *ctrl_cfg =
		(const struct w1_master_config *)dev->config;

	return ctrl_cfg->slave_count;
}

/**
 * @brief Configure parameters of the 1-Wire master.
 *
 * Allowed configuration parameters are defined in enum w1_settings_type,
 * but master devices may not support all types.
 *
 * @param[in] dev  Pointer to the device structure for the driver instance.
 * @param type     Enum specifying the setting type.
 * @param value    The new value for the passed settings type.
 *
 * @retval 0        If successful.
 * @retval -ENOTSUP The master doesn't support the configuration of the supplied type.
 * @retval -EIO     General input / output error, failed to configure master devices.
 */
__syscall int w1_configure(const struct device *dev,
			   enum w1_settings_type type, uint32_t value);

static inline int z_impl_w1_configure(const struct device *dev,
				      enum w1_settings_type type, uint32_t value)
{
	const struct w1_driver_api *api = (const struct w1_driver_api *)dev->api;

	return api->configure(dev, type, value);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <syscalls/w1.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_W1_H_ */
