/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for analog switch drivers.
 * @ingroup analog_switch_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ANALOG_SWITCH_H_
#define ZEPHYR_INCLUDE_DRIVERS_ANALOG_SWITCH_H_

/**
 * @brief Analog Switch Interface
 * @defgroup analog_switch_interface Analog Switch
 * @since 4.6
 * @version 0.1.0
 * @ingroup io_interfaces
 *
 * Generic interface for analog signal switches (SPST, SPDT, MEMS, etc.).
 *
 * Each device exposes one or more independently controllable channels.
 * A channel's state is a small unsigned integer; the meaning of each
 * value is part-specific and documented in the corresponding devicetree
 * binding. For SPST-style devices, 0 means open and 1 means closed.
 * For SPDT-style devices, 0 selects path B and 1 selects path A.
 *
 * @{
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @def_driverbackendgroup{Analog Switch, analog_switch_interface}
 * @{
 */

/**
 * @brief Set the state of a single channel.
 *
 * See analog_switch_set() for argument descriptions.
 */
typedef int (*analog_switch_set_t)(const struct device *dev, uint8_t channel,
				   uint8_t state);

/**
 * @brief Get the state of a single channel.
 *
 * See analog_switch_get() for argument descriptions.
 */
typedef int (*analog_switch_get_t)(const struct device *dev, uint8_t channel,
				   uint8_t *state);

/**
 * @brief Set all channels at once via a bitmask.
 *
 * See analog_switch_set_all() for argument descriptions.
 */
typedef int (*analog_switch_set_all_t)(const struct device *dev, uint32_t mask);

/**
 * @brief Get all channel states as a bitmask.
 *
 * See analog_switch_get_all() for argument descriptions.
 */
typedef int (*analog_switch_get_all_t)(const struct device *dev,
				       uint32_t *mask);

/**
 * @brief Enable the device.
 *
 * See analog_switch_enable() for argument descriptions.
 */
typedef int (*analog_switch_enable_t)(const struct device *dev);

/**
 * @brief Disable the device.
 *
 * See analog_switch_disable() for argument descriptions.
 */
typedef int (*analog_switch_disable_t)(const struct device *dev);

/**
 * @brief Reset all channels to the device's default state.
 *
 * See analog_switch_reset() for argument descriptions.
 */
typedef int (*analog_switch_reset_t)(const struct device *dev);

/**
 * @driver_ops{Analog Switch}
 */
__subsystem struct analog_switch_driver_api {
	/** @driver_ops_mandatory @copybrief analog_switch_set */
	analog_switch_set_t set;
	/** @driver_ops_mandatory @copybrief analog_switch_get */
	analog_switch_get_t get;
	/** @driver_ops_mandatory @copybrief analog_switch_set_all */
	analog_switch_set_all_t set_all;
	/** @driver_ops_mandatory @copybrief analog_switch_get_all */
	analog_switch_get_all_t get_all;
	/** @driver_ops_mandatory @copybrief analog_switch_reset */
	analog_switch_reset_t reset;
	/** @driver_ops_optional @copybrief analog_switch_enable */
	analog_switch_enable_t enable;
	/** @driver_ops_optional @copybrief analog_switch_disable */
	analog_switch_disable_t disable;
	/**
	 * Number of channels exposed by the device.
	 *
	 * Used by applications to validate channel indices before calling
	 * set/get and to determine which bits are meaningful in bitmask
	 * operations (set_all/get_all).
	 */
	uint8_t num_channels;
};

/** @} */

/**
 * @brief Set the state of a single channel.
 *
 * @param dev      Analog switch device.
 * @param channel  Zero-based channel index, less than the value returned
 *                 by analog_switch_get_num_channels().
 * @param state    Desired channel state. Valid values and their meaning
 *                 are part-specific; see the device's devicetree
 *                 binding. For binary parts, 0 and 1 are the only
 *                 supported values.
 *
 * @retval 0        Channel updated successfully.
 * @retval -EINVAL  @p channel is out of range or @p state is unsupported.
 * @retval -errno   Negative errno on bus or hardware failure.
 */
static inline int analog_switch_set(const struct device *dev, uint8_t channel,
				    uint8_t state)
{
	return DEVICE_API_GET(analog_switch, dev)->set(dev, channel, state);
}

/**
 * @brief Read the current state of a single channel.
 *
 * @param dev      Analog switch device.
 * @param channel  Zero-based channel index, less than the value returned
 *                 by analog_switch_get_num_channels().
 * @param state    Pointer to a location that receives the channel state.
 *
 * @retval 0        Channel read successfully.
 * @retval -EINVAL  @p channel is out of range or @p state is NULL.
 * @retval -errno   Negative errno on bus or hardware failure.
 */
static inline int analog_switch_get(const struct device *dev, uint8_t channel,
				    uint8_t *state)
{
	return DEVICE_API_GET(analog_switch, dev)->get(dev, channel, state);
}

/**
 * @brief Set the state of every channel atomically via a bitmask.
 *
 * Bit @c n of @p mask sets the state of channel @c n. Bits beyond the
 * device's channel count are ignored. Drivers attached to a serial bus
 * issue a single write; GPIO-driven backends update channels
 * sequentially.
 *
 * @param dev   Analog switch device.
 * @param mask  Bitmask of channel states.
 *
 * @retval 0        Channels updated successfully.
 * @retval -errno   Negative errno on bus or hardware failure.
 */
static inline int analog_switch_set_all(const struct device *dev, uint32_t mask)
{
	return DEVICE_API_GET(analog_switch, dev)->set_all(dev, mask);
}

/**
 * @brief Read every channel state into a bitmask.
 *
 * Bit @c n of @p mask reflects the state of channel @c n. Bits beyond
 * the device's channel count are cleared.
 *
 * @param dev   Analog switch device.
 * @param mask  Pointer to a location that receives the channel states.
 *
 * @retval 0        Channels read successfully.
 * @retval -EINVAL  @p mask is NULL.
 * @retval -errno   Negative errno on bus or hardware failure.
 */
static inline int analog_switch_get_all(const struct device *dev,
					uint32_t *mask)
{
	return DEVICE_API_GET(analog_switch, dev)->get_all(dev, mask);
}

/**
 * @brief Reset every channel to the device's default state.
 *
 * The default state is part-specific. SPST and MEMS parts typically
 * reset to "all open"; SPDT parts reset to a single documented path.
 *
 * @param dev  Analog switch device.
 *
 * @retval 0       Channels reset successfully.
 * @retval -errno  Negative errno on bus or hardware failure.
 */
static inline int analog_switch_reset(const struct device *dev)
{
	return DEVICE_API_GET(analog_switch, dev)->reset(dev);
}

/**
 * @brief Enable the device.
 *
 * Only meaningful on parts that expose a hardware enable line. When the
 * device is disabled all channels are forced open regardless of the
 * last requested state.
 *
 * @param dev  Analog switch device.
 *
 * @retval 0        Device enabled.
 * @retval -ENOTSUP The part has no enable line.
 * @retval -errno   Negative errno on hardware failure.
 */
static inline int analog_switch_enable(const struct device *dev)
{
	const struct analog_switch_driver_api *api =
		DEVICE_API_GET(analog_switch, dev);

	if (api->enable == NULL) {
		return -ENOTSUP;
	}

	return api->enable(dev);
}

/**
 * @brief Disable the device.
 *
 * Only meaningful on parts that expose a hardware enable line. While
 * disabled all channels are forced open. Channel state set via
 * analog_switch_set() while disabled takes effect on the next
 * analog_switch_enable() call, unless the driver documents otherwise.
 *
 * @param dev  Analog switch device.
 *
 * @retval 0        Device disabled.
 * @retval -ENOTSUP The part has no enable line.
 * @retval -errno   Negative errno on hardware failure.
 */
static inline int analog_switch_disable(const struct device *dev)
{
	const struct analog_switch_driver_api *api =
		DEVICE_API_GET(analog_switch, dev);

	if (api->disable == NULL) {
		return -ENOTSUP;
	}

	return api->disable(dev);
}

/**
 * @brief Number of channels the device exposes.
 *
 * @param dev  Analog switch device.
 *
 * @return Channel count (always non-zero for an initialized device).
 */
static inline uint8_t analog_switch_get_num_channels(const struct device *dev)
{
	return DEVICE_API_GET(analog_switch, dev)->num_channels;
}

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_ANALOG_SWITCH_H_ */
