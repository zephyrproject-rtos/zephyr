/*
 * Copyright 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I3C_TARGET_DEVICE_H_
#define ZEPHYR_INCLUDE_DRIVERS_I3C_TARGET_DEVICE_H_

/**
 * @brief I3C Target Device API
 * @defgroup i3c_target_device I3C Target Device API
 * @ingroup i3c_interface
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

struct i3c_driver_api;

/**
 * @brief Configuration parameters for I3C hardware to act as target device.
 *
 * This can also be used to configure the controller if it is to act as
 * a secondary controller on the bus.
 */
struct i3c_config_target {
	/**
	 * If the hardware is to act as a target device
	 * on the bus.
	 */
	bool enable;

	/**
	 * I3C target address.
	 *
	 * Used used when operates as secondary controller
	 * or as a target device.
	 */
	uint8_t static_addr;

	/** Provisioned ID. */
	uint64_t pid;

	/**
	 * True if lower 32-bit of Provisioned ID is random.
	 *
	 * This sets the bit 32 of Provisioned ID which means
	 * the lower 32-bit is random value.
	 */
	bool pid_random;

	/** Bus Characteristics Register (BCR). */
	uint8_t bcr;

	/** Device Characteristics Register (DCR). */
	uint8_t dcr;

	/** Maximum Read Length (MRL). */
	uint16_t max_read_len;

	/** Maximum Write Length (MWL). */
	uint16_t max_write_len;

	/**
	 * Bit mask of supported HDR modes (0 - 7).
	 *
	 * This can be used to enable or disable HDR mode
	 * supported by the hardware at runtime.
	 */
	uint8_t supported_hdr;
};

/**
 * @brief Structure describing a device that supports the I3C target API.
 *
 * Instances of this are passed to the i3c_target_register() and
 * i3c_target_unregister() functions to indicate addition and removal
 * of a target device, respective.
 *
 * Fields other than @c node must be initialized by the module that
 * implements the device behavior prior to passing the object
 * reference to i3c_target_register().
 */
struct i3c_target_config {
	/** Private, do not modify */
	sys_snode_t node;

	/**
	 * Flags for the target device defined by I3C_TARGET_FLAGS_*
	 * constants.
	 */
	uint8_t flags;

	/** Address for this target device */
	uint8_t address;

	/** Callback functions */
	const struct i3c_target_callbacks *callbacks;
};

struct i3c_target_callbacks {
	/**
	 * @brief Function called when a write to the device is initiated.
	 *
	 * This function is invoked by the controller when the bus completes
	 * a start condition for a write operation to the address associated
	 * with a particular device.
	 *
	 * A success return shall cause the controller to ACK the next byte
	 * received. An error return shall cause the controller to NACK the
	 * next byte received.
	 *
	 * @param config Configuration structure associated with the
	 *               device to which the operation is addressed.
	 *
	 * @return 0 if the write is accepted, or a negative error code.
	 */
	int (*write_requested_cb)(struct i3c_target_config *config);

	/**
	 * @brief Function called when a write to the device is continued.
	 *
	 * This function is invoked by the controller when it completes
	 * reception of a byte of data in an ongoing write operation to the
	 * device.
	 *
	 * A success return shall cause the controller to ACK the next byte
	 * received. An error return shall cause the controller to NACK the
	 * next byte received.
	 *
	 * @param config Configuration structure associated with the
	 *               device to which the operation is addressed.
	 *
	 * @param val the byte received by the controller.
	 *
	 * @return 0 if more data can be accepted, or a negative error
	 *         code.
	 */
	int (*write_received_cb)(struct i3c_target_config *config,
				 uint8_t val);

	/**
	 * @brief Function called when a read from the device is initiated.
	 *
	 * This function is invoked by the controller when the bus completes a
	 * start condition for a read operation from the address associated
	 * with a particular device.
	 *
	 * The value returned in @p val will be transmitted. A success
	 * return shall cause the controller to react to additional read
	 * operations. An error return shall cause the controller to ignore
	 * bus operations until a new start condition is received.
	 *
	 * @param config Configuration structure associated with the
	 *               device to which the operation is addressed.
	 *
	 * @param val Pointer to storage for the first byte of data to return
	 *            for the read request.
	 *
	 * @return 0 if more data can be requested, or a negative error code.
	 */
	int (*read_requested_cb)(struct i3c_target_config *config,
				 uint8_t *val);

	/**
	 * @brief Function called when a read from the device is continued.
	 *
	 * This function is invoked by the controller when the bus is ready to
	 * provide additional data for a read operation from the address
	 * associated with the device device.
	 *
	 * The value returned in @p val will be transmitted. A success
	 * return shall cause the controller to react to additional read
	 * operations. An error return shall cause the controller to ignore
	 * bus operations until a new start condition is received.
	 *
	 * @param config Configuration structure associated with the
	 *               device to which the operation is addressed.
	 *
	 * @param val Pointer to storage for the next byte of data to return
	 *            for the read request.
	 *
	 * @return 0 if data has been provided, or a negative error code.
	 */
	int (*read_processed_cb)(struct i3c_target_config *config,
				 uint8_t *val);

	/**
	 * @brief Function called when a stop condition is observed after a
	 * start condition addressed to a particular device.
	 *
	 * This function is invoked by the controller when the bus is ready to
	 * provide additional data for a read operation from the address
	 * associated with the device device. After the function returns the
	 * controller shall enter a state where it is ready to react to new
	 * start conditions.
	 *
	 * @param config Configuration structure associated with the
	 *               device to which the operation is addressed.
	 *
	 * @return Ignored.
	 */
	int (*stop_cb)(struct i3c_target_config *config);
};

__subsystem struct i3c_target_driver_api {
	int (*driver_register)(const struct device *dev);
	int (*driver_unregister)(const struct device *dev);
};

/**
 * @brief Writes to the target's TX FIFO
 *
 * Write to the TX FIFO @p dev I3C bus driver using the provided
 * buffer and length. Some I3C targets will NACK read requests until data
 * is written to the TX FIFO. This function will write as much as it can
 * to the FIFO return the total number of bytes written. It is then up to
 * the application to utalize the target callbacks to write the remaining
 * data. Negative returns indicate error.
 *
 * Most of the existing hardware allows simultaneous support for master
 * and target mode. This is however not guaranteed.
 *
 * @param dev Pointer to the device structure for an I3C controller
 *            driver configured in target mode.
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 *
 * @retval Total number of bytes written
 * @retval -ENOTSUP Not in Target Mode
 * @retval -ENOSPC No space in Tx FIFO
 * @retval -ENOSYS If target mode is not implemented
 */
static inline int i3c_target_tx_write(const struct device *dev,
				      uint8_t *buf, uint16_t len)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->target_tx_write == NULL) {
		return -ENOSYS;
	}

	return api->target_tx_write(dev, buf, len);
}

/**
 * @brief Registers the provided config as target device of a controller.
 *
 * Enable I3C target mode for the @p dev I3C bus driver using the provided
 * config struct (@p cfg) containing the functions and parameters to send bus
 * events. The I3C target will be registered at the address provided as
 * @ref i3c_target_config.address struct member. Any I3C bus events related
 * to the target mode will be passed onto I3C target device driver via a set of
 * callback functions provided in the 'callbacks' struct member.
 *
 * Most of the existing hardware allows simultaneous support for master
 * and target mode. This is however not guaranteed.
 *
 * @param dev Pointer to the device structure for an I3C controller
 *            driver configured in target mode.
 * @param cfg Config struct with functions and parameters used by
 *            the I3C target driver to send bus events
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -EIO General input / output error.
 * @retval -ENOSYS If target mode is not implemented
 */
static inline int i3c_target_register(const struct device *dev,
				      struct i3c_target_config *cfg)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->target_register == NULL) {
		return -ENOSYS;
	}

	return api->target_register(dev, cfg);
}

/**
 * @brief Unregisters the provided config as target device
 *
 * This routine disables I3C target mode for the @p dev I3C bus driver using
 * the provided config struct (@p cfg) containing the functions and parameters
 * to send bus events.
 *
 * @param dev Pointer to the device structure for an I3C controller
 *            driver configured in target mode.
 * @param cfg Config struct with functions and parameters used by
 *            the I3C target driver to send bus events
 *
 * @retval 0 Is successful
 * @retval -EINVAL If parameters are invalid
 * @retval -ENOSYS If target mode is not implemented
 */
static inline int i3c_target_unregister(const struct device *dev,
					struct i3c_target_config *cfg)
{
	const struct i3c_driver_api *api =
		(const struct i3c_driver_api *)dev->api;

	if (api->target_unregister == NULL) {
		return -ENOSYS;
	}

	return api->target_unregister(dev, cfg);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I3C_TARGET_DEVICE_H_ */
