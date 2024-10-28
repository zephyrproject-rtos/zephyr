/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DAC public API header file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DAC driver APIs
 * @defgroup dac_interface DAC driver APIs
 * @since 2.3
 * @version 0.8.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Broadcast channel identifier for DACs that support it.
 * @note Only for use in dac_write_value().
 */
#define DAC_CHANNEL_BROADCAST	0xFF

/**
 * @brief Structure for specifying the configuration of a DAC channel.
 */
struct dac_channel_cfg {
	/** Channel identifier of the DAC that should be configured. */
	uint8_t channel_id;
	/** Desired resolution of the DAC (depends on device capabilities). */
	uint8_t resolution;
	/** Enable output buffer for this channel.
	 * This is relevant for instance if the output is directly connected to the load,
	 * without an amplifierin between. The actual details on this are hardware dependent.
	 */
	bool buffered: 1;
	/** Enable internal output path for this channel. This is relevant for channels that
	 * support directly connecting to on-chip peripherals via internal paths. The actual
	 * details on this are hardware dependent.
	 */
	bool internal: 1;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/*
 * Type definition of DAC API function for configuring a channel.
 * See dac_channel_setup() for argument descriptions.
 */
typedef int (*dac_api_channel_setup)(const struct device *dev,
				     const struct dac_channel_cfg *channel_cfg);

/*
 * Type definition of DAC API function for setting a write request.
 * See dac_write_value() for argument descriptions.
 */
typedef int (*dac_api_write_value)(const struct device *dev,
				    uint8_t channel, uint32_t value);

/*
 * DAC driver API
 *
 * This is the mandatory API any DAC driver needs to expose.
 */
__subsystem struct dac_driver_api {
	dac_api_channel_setup channel_setup;
	dac_api_write_value   write_value;
};

/**
 * @endcond
 */

/**
 * @brief Configure a DAC channel.
 *
 * It is required to call this function and configure each channel before it is
 * selected for a write request.
 *
 * @param dev          Pointer to the device structure for the driver instance.
 * @param channel_cfg  Channel configuration.
 *
 * @retval 0         On success.
 * @retval -EINVAL   If a parameter with an invalid value has been provided.
 * @retval -ENOTSUP  If the requested resolution is not supported.
 */
__syscall int dac_channel_setup(const struct device *dev,
				const struct dac_channel_cfg *channel_cfg);

static inline int z_impl_dac_channel_setup(const struct device *dev,
					   const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_driver_api *api =
				(const struct dac_driver_api *)dev->api;

	return api->channel_setup(dev, channel_cfg);
}

/**
 * @brief Write a single value to a DAC channel
 *
 * @param dev         Pointer to the device structure for the driver instance.
 * @param channel     Number of the channel to be used.
 * @param value       Data to be written to DAC output registers.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If a parameter with an invalid value has been provided.
 */
__syscall int dac_write_value(const struct device *dev, uint8_t channel,
			      uint32_t value);

static inline int z_impl_dac_write_value(const struct device *dev,
						uint8_t channel, uint32_t value)
{
	const struct dac_driver_api *api =
				(const struct dac_driver_api *)dev->api;

	return api->write_value(dev, channel, value);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/dac.h>

#endif  /* ZEPHYR_INCLUDE_DRIVERS_DAC_H_ */
