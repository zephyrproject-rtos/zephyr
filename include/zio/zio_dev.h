/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZIO Device, a pollable sampled IO device
 *
 * ZIO devices are composed of device attributes (zio_attr),
 * channels (zio_dev_chan) and channel attributes (zio_attr) in the
 * following manner:
 *
 * ZIO Device----------------------------------------------------------------+
 * |                                                                         |
 * | Device Attributes-----------------------------------------------------+ |
 * | | +-----------------------------------------------------------------+ | |
 * | | |                Device Attribute 0 (Ex: Op. Mode)                | | |
 * | | +-----------------------------------------------------------------+ | |
 * |                                                                         |
 * | +---------------------------------------------------------------------+ |
 * | Channels--------------------------------------------------------------+ |
 * | | Channel 0---------------------------------------------------------+ | |
 * | | | Channel Attributes---------------------------------------------+| | |
 * | | | | +----------------------------------------------------------+ || | |
 * | | | | |      Channel Attribute 0 (Ex: Sample Rate)               | || | |
 * | | | | +----------------------------------------------------------+ || | |
 * | | | | |      Channel Attribute 1 (Ex: Scale)                     | || | |
 * | | | | +----------------------------------------------------------+ || | |
 * | | | | |      Channel Attribute 2 (Ex: Low-Pass Filter Frequency) | || | |
 * | | | | +----------------------------------------------------------+ || | |
 * | | | +--------------------------------------------------------------+| | |
 * | | +-----------------------------------------------------------------+ | |
 * | | |Channel 1                                                        | | |
 * | | +-----------------------------------------------------------------+ | |
 * | | |Channel 2                                                        | | |
 * | | +-----------------------------------------------------------------+ | |
 * | +---------------------------------------------------------------------+ |
 * +-------------------------------------------------------------------------+
 *
 * Each device describes available attributes and channels through a set of
 * arrays containing description structs available by the zio_dev API. The
 * device driver can and probably does define these statically, though they
 * could be dynamic if needed.
 *
 * The interface expects attributes are infrequently read or manipulated where
 * as datum reads or writes to the set of device channels to be very frequent.
 *
 * Manipulating attributes of a device is done by referencing the
 * index of the attribute in the devices array of attribute descriptions.
 *
 * Channel attributes may also be manipulated in the same manner as device
 * attributes.
 */

#ifndef ZEPHYR_INCLUDE_ZIO_DEV_H_
#define ZEPHYR_INCLUDE_ZIO_DEV_H_

#include <zephyr/types.h>
#include <device.h>

#include <zio/zio_attr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ZIO device API definitions.
 * @defgroup zio_device ZIO device definitions
 * @ingroup zio
 * @{
 */

struct zio_buf;

/**
 * @brief Pre-defined channel types with the ability to extend by a driver
 * implementation.
 *
 * Extended values should always be based on ZIO_LAST_CHAN_TYPE with an
 * additive and defined in a publicly available driver header.
 */
enum zio_channel_type {
	ZIO_VOLTAGE,

	/* Last type is always begins the user definable type range */
	ZIO_CHAN_TYPES
};


/**
 * @brief Channel description struct
 *
 * A description of a the format and type of data coming for a channel
 */
struct zio_dev_chan {
	/**
	 * Name of the channel, may be null
	 */
	const char *name;

	/**
	 * Type of channel
	 *
	 * This describes the type of data the channel provides, such as
	 * X axis Acceleration, or a specific frequency of light.
	 *
	 * An enum defines many common channel types with a user definable range
	 * above a certain value defined in the enum. The enum will be defined
	 * in a future zio_defs.h file.
	 */
	const u16_t type;

	/**
	 * Bit width of channel data, ex: 12 bit samples from a 12 bit adc
	 */
	const u8_t bit_width;

	/**
	 * Byte size of channel data, ex: 2 bytes (s16_t)
	 */
	const u8_t byte_size;

	/**
	 * Byte ordering of channel data
	 */
	const enum byteorder {
		ZIO_BYTEORDER_ARCH,
			ZIO_BYTEORDER_BIG,
			ZIO_BYTEORDER_LITTLE
	} byte_order;

	const enum sign_bit {
		ZIO_SIGN_NONE,
			ZIO_SIGN_MSB,
			ZIO_SIGN_LSB,
	} sign_bit;

};

/**
 * @brief Function to set a device attribute
 *
 * @return -EINVAL if an invalid attribute id was given, 0 otherwise
 */
typedef int (*zio_dev_set_attr_t)(struct device *dev, const u32_t attr_idx,
				  const struct zio_variant var);

/**
 * @brief Function to get a device attribute
 *
 * @return -EINVAL if an invalid attribute id was given, 0 otherwise
 */
typedef int (*zio_dev_get_attr_t)(struct device *dev, uint32_t attr_idx,
				  struct zio_variant *attr);


/**
 * @brief Function to get a pointer to the array of attribute descriptions
 */
typedef int (*zio_dev_get_attrs_t)(struct device *dev,
				   struct zio_attr **attrs,
				   u16_t *num_attrs);

/**
 * @brief Function to get a pointer to the array of channel descriptions
 */
typedef int (*zio_dev_get_chans_t)(struct device *dev,
				   const struct zio_dev_chan **chans,
				   u32_t *num_chans);

/**
 * @brief Function to get a channels attribute descriptions
 *
 * @return -EINVAL if an invalid channel id was given, 0 otherwise
 */
typedef int (*zio_chan_get_attrs_t)(struct device *dev, const u32_t chan_idx,
				    struct zio_attr *attrs,
				    u32_t *num_attrs);

/**
 * @brief Function to set a channel attribute for a device
 *
 * @return -EINVAL if an invalid attribute id, type, or range was given,
 * 0 otherwise
 */
typedef int (*zio_chan_set_attr_t)(struct device *dev, const u32_t chan_idx,
				   const u32_t attr_idx,
				   const struct zio_variant var);
/**
 * @brief Function to get a channel attribute for a device
 *
 * @return -EINVAL if an invalid attribute id was given, 0 otherwise
 */
typedef int (*zio_chan_get_attr_t)(struct device *dev, const u32_t chan_idx,
					const u32_t attr_idx,
					struct zio_variant *var);

/**
 * @brief Function to enable a channel for a device
 *
 * Not all devices support enabling/disabling channels
 *
 * @return -EINVAL if an invalid chan was given, 0 otherwise
 */
typedef int (*zio_chan_enable_t)(struct device *dev, u32_t chan_idx);

/**
 * @brief Function to disable a channel for a device
 *
 * Not all devices support enabling/disabling channels
 *
 * @return -EINVAL if an invalid chan was given, 0 otherwise
 */
typedef int (*zio_chan_disable_t)(struct device *dev, u32_t chan_idx);

/**
 * @brief Function to determine if a channel for a device is enabled
 *
 * Not all devices support enabling/disabling channels
 *
 * @return -EINVAL if an invalid chan was given, 0 if not enabled, 1 if enabled
 */
typedef int (*zio_chan_is_enabled_t)(struct device *dev,
				     struct zio_dev_chan *chan);


/**
 * @brief Function to trigger a read or write of the device
 *
 * Trigger implementation must be callable in the context of an ISR,
 * and they should do as little as possible to begin the process of
 * reading or writing from the device in a timely manner.
 *
 * As an example if the device were using SPI the device driver should
 * in the best scenario begin the spi transaction but wait for the results
 * coming from a second interrupt rather than blocking the call context.
 *
 * This is feasible with CONFIG_SPI_ASYNC. IF the bus or the device does not
 * support this form of fire and forget triggering the trigger *should*
 * push the work to a thread or work queue task to avoid blocking.
 *
 * By pushing the work to a thread or work queue task the timing of the
 * trigger has a large possibility of being unpredictable and should be
 * noted by the device driver.
 */
typedef int (*zio_dev_trigger_t)(struct device *dev);

/**
 * @private
 * @brief Function to attach a zio_buf to a device.
 */
typedef int (*zio_dev_attach_buf_t)(struct device *dev, struct zio_buf *buf);

/**
 * @brief Function to detach a buffer from the device
 */
typedef int (*zio_dev_detach_buf_t)(struct device *dev);


/**
 * @brief Functions for ZIO device implementations
 */
struct zio_dev_api {
	zio_dev_set_attr_t set_attr;
	zio_dev_get_attr_t get_attr;
	zio_dev_get_attrs_t get_attrs;

	zio_dev_get_chans_t get_chans;
	zio_chan_get_attrs_t get_chan_attrs;
	zio_chan_set_attr_t set_chan_attr;
	zio_chan_get_attr_t get_chan_attr;
	zio_chan_enable_t enable_chan;
	zio_chan_disable_t disable_chan;
	zio_chan_is_enabled_t is_chan_enabled;

	zio_dev_trigger_t trigger; /**< manually trigger, driver optional */
	/* TODO set/get/enable/disable triggers and trigger options */

	zio_dev_attach_buf_t attach_buf;
	zio_dev_detach_buf_t detach_buf;

	/* TODO read raw datum out to a void* */
	/* TODO write raw datum from a void* */
	/* TODO convert raw datum to common SI units */
};

/**
 * @brief Set a device attribute to a given value
 *
 * @param dev Pointer to the ZIO device
 * @param attr_idx Index of the attribute to setattr
 * @param val Value of any taggable type for a zio_variant, ex: a u8_t
 *
 * @return 0 if successful, negative errno code if failure.
 */
#define zio_dev_set_attr(dev, attr_idx, val) \
	({ \
		int res = 0; \
		const struct zio_dev_api *api = dev->driver_api; \
		if (!api->set_attr) { \
			res = -ENOTSUP; \
		} else { \
			struct zio_variant data = zio_variant_wrap(val); \
			res = api->set_attr(dev, attr_idx, data); \
		} \
		res; \
	})

/**
 * @brief Get a device attribute value
 *
 * @param dev Pointer to the ZIO device
 * @param attr_idx Index of the attribute to setattr
 * @param val Pointer to a value to copy the attribute value into
 *
 * @return 0 if successful, negative errno code if failure.
 */
#define zio_dev_get_attr(dev, attr_idx, val) \
	({ \
		int res = 0; \
		const struct zio_dev_api *api = dev->driver_api; \
		if (!api->set_attr) { \
			res = -ENOTSUP; \
		} else { \
			struct zio_variant data; \
			res = api->get_attr(dev, attr_idx, &data); \
			if (res == 0) { \
				res = zio_variant_unwrap(data, val); \
			} \
			res; \
		} \
	})

/**
 * @brief Get the all of the device attributes
 *
 * @param dev Pointer to the ZIO device
 * @param attrs Double pointer, value is set to the address of this devices
 *	attrs
 * @param num_attrs Pointer to a u32_t which is set to the number of attrs of
 *	the device
 */
static inline int zio_dev_get_attrs(struct device *dev,
		struct zio_attr **attrs, u16_t *num_attrs)
{
	const struct zio_dev_api *api = dev->driver_api;

	if (!api->get_attrs) {
		return -ENOTSUP;
	}
	api->get_attrs(dev, attrs, num_attrs);
	return 0;
}

/**
 * @brief Enable a device channel by index
 *
 * @param dev Pointer to the ZIO device
 * @param chan_idx The channel index
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int zio_dev_enable_chan(struct device *dev, u32_t chan_idx)
{
	const struct zio_dev_api *api = dev->driver_api;

	if (!api->enable_chan) {
		return -ENOTSUP;
	}
	return api->enable_chan(dev, chan_idx);
}

/**
 * @brief Disable a device channel by index
 *
 * @param dev Pointer to the sensor device
 * @param chan_idx The channel index
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int zio_dev_disable_chan(struct device *dev, u32_t chan_idx)
{
	const struct zio_dev_api *api = dev->driver_api;

	if (!api->enable_chan) {
		return -ENOTSUP;
	}
	return api->enable_chan(dev, chan_idx);
}

/**
 * @brief Manually trigger the device for read or write
 */
static inline int zio_dev_trigger(struct device *dev)
{
	const struct zio_dev_api *api = dev->driver_api;

	if (!api->trigger) {
		return -ENOTSUP;
	}
	return api->trigger(dev);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZIO_DEV_H_ */
