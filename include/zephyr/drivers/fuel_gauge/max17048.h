/*
 * Copyright (c) 2023 Fraunhofer AICOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_MAX17048_H_
#define ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_MAX17048_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type of event triggers on MAX17048
 */
enum max17048_trigger_type {
	/*!< Voltage is above over-voltage threshold (single trigger) */
	MAX17048_TRIGGER_OVERVOLTAGE,
	/*!< Voltage is under under-voltage threshold (single trigger) */
	MAX17048_TRIGGER_UNDERVOLTAGE,
	/*!< Voltage is under low-soc threshold */
	MAX17048_TRIGGER_LOW_SOC,
};

typedef void (*max17048_trigger_handler_t)(const struct device *dev,
					   const enum max17048_trigger_type evt_type);

/**
 * @brief Enables alarm for specified MAX17048 trigger event.
 *
 * This function enables the trigger for \p trigger_type events and sets \p handler as a callback
 * function. Be aware that some events are single triggered (MAX17048_TRIGGER_OVERVOLTAGE and
 * MAX17048_TRIGGER_UNDERVOLTAGE) and require you to call this function after the alarm is triggered
 * to reactivate the alarm for such events.
 *
 * @param dev Pointer to the max17048 device.
 * @param trigger_type Type of trigger to be enabled by the function.
 * @param handler Event handler to be called uppon \p trigger_event event occurs.
 */
int max17048_trigger_set(const struct device *dev, const enum max17048_trigger_type trigger_type,
			 max17048_trigger_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FUEL_GAUGE_MAX17048_H_ */
