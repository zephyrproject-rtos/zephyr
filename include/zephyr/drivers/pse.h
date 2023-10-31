/**
 * @file drivers/pse.h
 *
 * @brief Public APIs for the pse driver.
 */

/*
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_PSE_H_
#define ZEPHYR_INCLUDE_DRIVERS_PSE_H_

/**
 * @brief PSE Interface
 * @defgroup pse_interface PSE Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PSE operational modes.
 */
enum pse_mode {
	PSE_DEACTIVATED,

	PSE_MANUAL_MODE,
	PSE_SEMI_AUTO_MODE,
	PSE_AUTO_MODE,
};

/**
 * @brief PSE channels.
 */
enum pse_channel {
	CHANNEL_0,
	CHANNEL_1,
	CHANNEL_2,
	CHANNEL_3,
};

#ifdef CONFIG_PSE_INFO

struct pse_info {
	const struct device *dev;
	const char *vendor;
	const char *model;
	const char *friendly_name;
};

#define PSE_INFO_INITIALIZER(_dev, _vendor, _model, _friendly_name)                                \
	{                                                                                          \
		.dev = _dev, .vendor = _vendor, .model = _model, .friendly_name = _friendly_name,  \
	}

#define PSE_INFO_DEFINE(name, ...)                                                                 \
	static const STRUCT_SECTION_ITERABLE(pse_info, name) = PSE_INFO_INITIALIZER(__VA_ARGS__)

#define PSE_INFO_DT_NAME(node_id) _CONCAT(__pse_info, DEVICE_DT_NAME_GET(node_id))

#define PSE_INFO_DT_DEFINE(node_id)                                                                \
	PSE_INFO_DEFINE(PSE_INFO_DT_NAME(node_id), DEVICE_DT_GET(node_id),                         \
			DT_NODE_VENDOR_OR(node_id, NULL), DT_NODE_MODEL_OR(node_id, NULL),         \
			DT_PROP_OR(node_id, friendly_name, NULL))

#else

#define PSE_INFO_DEFINE(name, ...)
#define PSE_INFO_DT_DEFINE(node_id)

#endif /* CONFIG_PSE_INFO */

/**
 * @brief Like DEVICE_DT_DEFINE() with pse specifics.
 *
 * @details Defines a device which implements the pse API. May define an
 * element in the pse info iterable section used to enumerate all pse
 * devices.
 *
 * @param node_id The devicetree node identifier.
 *
 * @param init_fn Name of the init function of the driver.
 *
 * @param pm_device PM device resources reference (NULL if device does not use
 * PM).
 *
 * @param data_ptr Pointer to the device's private data.
 *
 * @param cfg_ptr The address to the structure containing the configuration
 * information for this instance of the driver.
 *
 * @param level The initialization level. See SYS_INIT() for details.
 *
 * @param prio Priority within the selected initialization level. See
 * SYS_INIT() for details.
 *
 * @param api_ptr Provides an initial pointer to the API function struct used
 * by the driver. Can be NULL.
 */
#define PSE_DEVICE_DT_DEFINE(node_id, init_fn, pm_device, data_ptr, cfg_ptr, level, prio, api_ptr, \
			     ...)                                                                  \
	DEVICE_DT_DEFINE(node_id, init_fn, pm_device, data_ptr, cfg_ptr, level, prio, api_ptr,     \
			 __VA_ARGS__);                                                             \
                                                                                                   \
	PSE_INFO_DT_DEFINE(node_id);

/**
 * @brief Like PSE_DEVICE_DT_DEFINE() for an instance of a DT_DRV_COMPAT
 * compatible
 *
 * @param inst instance number. This is replaced by
 * <tt>DT_DRV_COMPAT(inst)</tt> in the call to PSE_DEVICE_DT_DEFINE().
 *
 * @param ... other parameters as expected by PSE_DEVICE_DT_DEFINE().
 */
#define PSE_DEVICE_DT_INST_DEFINE(inst, ...) PSE_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @typedef pse_event_trigger_handler_t
 * @brief Callback API upon firing of a trigger
 *
 * @param dev Pointer to the pse device
 */
typedef int (*pse_event_trigger_handler_t)(const struct device *dev);

/**
 * @typedef pse_get_curr_t
 * @brief Callback API for getting the pse current on desired channel
 *
 * See pse_current_get() for argument description
 */
typedef int (*pse_get_curr_t)(const struct device *dev, uint8_t channel, uint16_t *buff);

/**
 * @typedef pse_get_volt_t
 * @brief Callback API for getting the pse voltage on desired channel
 *
 * See pse_voltage_get() for argument description
 */
typedef int (*pse_get_volt_t)(const struct device *dev, uint8_t channel, uint16_t *buff);

/**
 * @typedef pse_get_main_volt_t
 * @brief Callback API for getting the msin pse input voltage
 *
 * See pse_main_voltage_get() for argument description
 */
typedef int (*pse_get_main_volt_t)(const struct device *dev, uint16_t *buff);

/**
 * @typedef pse_get_temp_t
 * @brief Callback API for getting the pse temperature
 *
 * See pse_temperature_get() for argument description
 */
typedef int (*pse_get_temp_t)(const struct device *dev, uint8_t *buff);

/**
 * @typedef pse_get_events_t
 * @brief Callback API for getting the pse event table
 *
 * See pse_events_get() for argument description
 */
typedef int (*pse_get_events_t)(const struct device *dev, uint8_t *buff);

/**
 * @typedef pse_set_events_t
 * @brief Callback API for setting the pse event
 *
 * See pse_events_set() for argument description
 */
typedef int (*pse_set_events_t)(const struct device *dev, uint8_t event);

/**
 * @typedef pse_channel_on_t
 * @brief Callback API for switch-on the given channel.
 *
 * See pse_channel_on() for argument description.
 */
typedef int (*pse_channel_on_t)(const struct device *dev, uint8_t channel);

/**
 * @typedef pse_channel_off_t
 * @brief Callback API for switch-off given channel.
 *
 * See pse_channel_off() for argument description.
 */
typedef int (*pse_channel_off_t)(const struct device *dev, uint8_t channel);

/**
 * @typedef pse_event_trigger_set
 * @brief Callback API for setting a pse event handler
 *
 * See pse_event_trigger_set() for argument description
 */
typedef int (*pse_event_trigger_set_t)(const struct device *dev,
				       pse_event_trigger_handler_t handler);

__subsystem struct pse_driver_api {
	pse_get_curr_t get_current;
	pse_get_volt_t get_voltage;
	pse_get_main_volt_t get_main_voltage;
	pse_get_temp_t get_temperature;
	pse_get_events_t get_events;
	pse_set_events_t set_events;
	pse_channel_on_t channel_on;
	pse_channel_off_t channel_off;
	pse_event_trigger_set_t set_event_trigger;
};

/**
 * @brief Get current consumption of the desired channel
 *
 * @param dev Pointer to the pse device
 * @param channel Channel for reading current
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_current_get(const struct device *dev, uint8_t channel, uint16_t *buff);

static inline int z_impl_pse_current_get(const struct device *dev, uint8_t channel, uint16_t *buff)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->get_current == NULL) {
		return -ENOSYS;
	}

	return api->get_current(dev, channel, buff);
}

/**
 * @brief Get voltage level of the desired channel
 *
 * @param dev Pointer to the pse device
 * @param channel Channel for reading the voltage from
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_voltage_get(const struct device *dev, uint8_t channel, uint16_t *buff);

static inline int z_impl_pse_voltage_get(const struct device *dev, uint8_t channel, uint16_t *buff)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->get_voltage == NULL) {
		return -ENOSYS;
	}

	return api->get_voltage(dev, channel, buff);
}

/**
 * @brief Get pse main input voltage
 *
 * @param dev Pointer to the pse device
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_main_voltage_get(const struct device *dev, uint16_t *buff);

static inline int z_impl_pse_main_voltage_get(const struct device *dev, uint16_t *buff)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->get_main_voltage == NULL) {
		return -ENOSYS;
	}

	return api->get_main_voltage(dev, buff);
}

/**
 * @brief Get pse chip temperature
 *
 * @param dev Pointer to the pse device
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_temperature_get(const struct device *dev, uint8_t *buff);

static inline int z_impl_pse_temperature_get(const struct device *dev, uint8_t *buff)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->get_temperature == NULL) {
		return -ENOSYS;
	}

	return api->get_temperature(dev, buff);
}

/**
 * @brief Get pse events table
 *
 * @param dev Pointer to the pse device
 * @param buff Buffer to store events table
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_event_get(const struct device *dev, uint8_t *events);

static inline int z_impl_pse_event_get(const struct device *dev, uint8_t *events)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->get_events == NULL) {
		return -ENOSYS;
	}

	return api->get_events(dev, events);
}

/**
 * @brief Activate appropriate event
 *
 * @param dev Pointer to the pse device
 * @param event Event to activate
 *
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_event_set(const struct device *dev, uint8_t events);

static inline int z_impl_pse_event_set(const struct device *dev, uint8_t events)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->set_events == NULL) {
		return -ENOSYS;
	}

	return api->set_events(dev, events);
}

/**
 * @brief Switch-on concrete channel
 *
 * @param dev Pointer to the pse device
 * @param event channel to be switch-on
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_channel_on(const struct device *dev, uint8_t channel);

static inline int z_impl_pse_channel_on(const struct device *dev, uint8_t channel)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->channel_on == NULL) {
		return -ENOSYS;
	}

	return api->channel_on(dev, channel);
}

/**
 * @brief Switch-off concrete channel
 *
 * @param dev Pointer to the pse device
 * @param event channel to be switch-off
 * @return 0 if successful, negative errno code if failure.
 */
__syscall int pse_channel_off(const struct device *dev, uint8_t channel);

static inline int z_impl_pse_channel_off(const struct device *dev, uint8_t channel)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->channel_off == NULL) {
		return -ENOSYS;
	}

	return api->channel_off(dev, channel);
}

/**
 * @brief Activate a pase's trigger and set the trigger handler
 *
 * The handler will be called from a thread, so I2C or SPI operations are
 * safe. However, the thread's stack is limited and defined by the
 * driver. It is currently up to the caller to ensure that the handler
 * does not overflow the stack.
 *
 * @funcprops \supervisor
 *
 * @param dev Pointer to the sensor device
 * @param handler The function that should be called when the trigger
 * fires
 *
 * @return 0 if successful, negative errno code if failure.
 */
static inline int pse_event_trigger_set(const struct device *dev,
					pse_event_trigger_handler_t handler)
{
	const struct pse_driver_api *api = (const struct pse_driver_api *)dev->api;

	if (api->set_event_trigger == NULL) {
		return -ENOSYS;
	}

	return api->set_event_trigger(dev, handler);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/pse.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_PSE_H_ */
