/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup energy_meter_interface
 * @brief Main header file for energy meter driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_H_
#define ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_H_

/**
 * @brief Interfaces for electrical energy metering ICs.
 * @defgroup energy_meter_interface Energy meter
 * @since 4.6
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 *
 * An energy meter measures the voltages and currents of a single-phase or poly-phase
 * electrical installation and derives power, power factor, frequency and accumulated energy.
 *
 * A value is addressed by a quantity (@ref energy_meter_quantity) and a line
 * (@ref energy_meter_line). Single-phase meters report their line on
 * @ref ENERGY_METER_LINE_A; three-phase meters use lines A, B and C. Values that belong to the
 * whole installation, such as total power or line frequency, are reported on
 * @ref ENERGY_METER_LINE_TOTAL.
 *
 * All values are signed 64-bit integers in millionths of the SI unit of the quantity:
 * microvolts, microamperes, microwatts, microwatt-hours and so on. Active power is positive
 * when energy flows from the supply to the load (import).
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Quantities measured by an energy meter.
 */
enum energy_meter_quantity {
	/** RMS line-to-neutral voltage, in uV. */
	ENERGY_METER_VOLTAGE = 0,
	/**
	 * RMS line-to-line voltage, in uV. Line A is A-B, line B is B-C and line C is C-A.
	 */
	ENERGY_METER_VOLTAGE_LL,
	/** RMS current, in uA. */
	ENERGY_METER_CURRENT,
	/** Mean active power, in uW. */
	ENERGY_METER_ACTIVE_POWER,
	/** Mean reactive power, in uvar. Positive for inductive loads. */
	ENERGY_METER_REACTIVE_POWER,
	/** Mean apparent power, in uVA. */
	ENERGY_METER_APPARENT_POWER,
	/** Power factor, in millionths, from -1000000 to 1000000. */
	ENERGY_METER_POWER_FACTOR,
	/**
	 * Angle between voltage and current, in microdegrees, greater than -180 and up to
	 * 180 degrees.
	 */
	ENERGY_METER_PHASE_ANGLE,
	/**
	 * Angle of the line voltage relative to the line A voltage, in microdegrees, from 0 up
	 * to but not including 360 degrees.
	 */
	ENERGY_METER_VOLTAGE_ANGLE,
	/** Line frequency, in uHz. */
	ENERGY_METER_FREQUENCY,
	/** Imported (forward) active energy, in uWh. */
	ENERGY_METER_ACTIVE_ENERGY_IMPORT,
	/** Exported (reverse) active energy, in uWh. */
	ENERGY_METER_ACTIVE_ENERGY_EXPORT,
	/** Imported (forward, inductive) reactive energy, in uvarh. */
	ENERGY_METER_REACTIVE_ENERGY_IMPORT,
	/** Exported (reverse, capacitive) reactive energy, in uvarh. */
	ENERGY_METER_REACTIVE_ENERGY_EXPORT,
	/** Apparent energy, in uVAh. */
	ENERGY_METER_APPARENT_ENERGY,
	/** Temperature of the metering IC, in micro degrees Celsius. */
	ENERGY_METER_TEMPERATURE,

	/** Number of quantities defined by this API. */
	ENERGY_METER_QUANTITY_COMMON_COUNT,

	/** First quantity a driver can define for device-specific values. */
	ENERGY_METER_QUANTITY_PRIV_START = ENERGY_METER_QUANTITY_COMMON_COUNT,

	/** Upper limit of quantity values. */
	ENERGY_METER_QUANTITY_MAX = UINT8_MAX,
};

/**
 * @brief Lines of an electrical installation.
 */
enum energy_meter_line {
	/** Phase A, or the line of a single-phase meter. */
	ENERGY_METER_LINE_A = 0,
	/** Phase B. */
	ENERGY_METER_LINE_B,
	/** Phase C. */
	ENERGY_METER_LINE_C,
	/** Neutral. */
	ENERGY_METER_LINE_N,
	/** The installation as a whole: sums over all phases and per-installation values. */
	ENERGY_METER_LINE_TOTAL,
	/** Number of lines. */
	ENERGY_METER_LINE_COUNT,
};

/**
 * @brief A value to read: a quantity on a line.
 */
struct energy_meter_channel {
	/** Quantity, an @ref energy_meter_quantity or a device-specific value. */
	uint8_t quantity;
	/** Line, an @ref energy_meter_line. */
	uint8_t line;
};

/**
 * @brief Description of what an energy meter measures.
 */
struct energy_meter_info {
	/** Number of phases: 1 or 3. */
	uint8_t phases;
	/**
	 * Supported common quantities on each line, as bit masks of BIT(quantity), indexed by
	 * @ref energy_meter_line.
	 */
	uint32_t quantities[ENERGY_METER_LINE_COUNT];
};

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(ENERGY_METER_QUANTITY_COMMON_COUNT <= 32,
	     "quantity masks in struct energy_meter_info are 32-bit");
/** @endcond */

/**
 * @brief Types of energy meter events.
 */
enum energy_meter_event_type {
	/** An energy pulse output toggled; the index is the output number, from 0. */
	ENERGY_METER_EVENT_PULSE,
	/** An interrupt or alarm output asserted; the index is the output number, from 0. */
	ENERGY_METER_EVENT_ALARM,
	/** The device signaled a fault, such as a calibration checksum error. */
	ENERGY_METER_EVENT_FAULT,
};

/**
 * @brief An energy meter event.
 */
struct energy_meter_event {
	/** Event type, an @ref energy_meter_event_type. */
	uint8_t type;
	/** Index of the output that caused the event. */
	uint8_t index;
};

/**
 * @brief Energy meter event callback.
 *
 * Drivers document the context the callback runs in.
 *
 * @param dev Energy meter device.
 * @param event Event that occurred.
 * @param user_data User data given to energy_meter_set_event_callback().
 */
typedef void (*energy_meter_event_cb_t)(const struct device *dev,
					const struct energy_meter_event *event, void *user_data);

/** @cond INTERNAL_HIDDEN */

typedef int (*energy_meter_get_info_t)(const struct device *dev, struct energy_meter_info *info);

typedef int (*energy_meter_get_t)(const struct device *dev,
				  const struct energy_meter_channel *channels, int64_t *values,
				  size_t count);

typedef int (*energy_meter_reset_energy_t)(const struct device *dev);

typedef int (*energy_meter_set_event_callback_t)(const struct device *dev,
						 energy_meter_event_cb_t cb, void *user_data);

__subsystem struct energy_meter_driver_api {
	energy_meter_get_info_t get_info;
	energy_meter_get_t get;
	energy_meter_reset_energy_t reset_energy;
	energy_meter_set_event_callback_t set_event_callback;
};

/** @endcond */

/**
 * @brief Describe what an energy meter measures.
 *
 * @param dev Energy meter device.
 * @param info Filled with the description.
 *
 * @retval 0 Success.
 * @retval -errno Other negative errno code on failure.
 */
__syscall int energy_meter_get_info(const struct device *dev, struct energy_meter_info *info);

static inline int z_impl_energy_meter_get_info(const struct device *dev,
					       struct energy_meter_info *info)
{
	return DEVICE_API_GET(energy_meter, dev)->get_info(dev, info);
}

/**
 * @brief Read values from an energy meter.
 *
 * The values are read in a single access to the device where the hardware allows it, so
 * they belong to the same measurement period. Reading an energy quantity returns the energy
 * accumulated since initialization or since the last energy_meter_reset_energy().
 *
 * From user mode, the channels are passed to the driver in groups of
 * @ref ENERGY_METER_QUANTITY_COMMON_COUNT, so only channels of the same group share a
 * measurement period.
 *
 * @param dev Energy meter device.
 * @param channels Values to read.
 * @param values Filled with one value per channel, in the units of its quantity.
 * @param count Number of channels.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP A channel is not supported by the device.
 * @retval -EBUSY The device is suspended.
 * @retval -EAGAIN A value has not been measured yet, for example right after the device was
 *         initialized or resumed. Read again later.
 * @retval -EIO Communication with the device failed.
 * @retval -errno Other negative errno code on failure.
 *
 * On failure, the content of @p values is undefined.
 */
__syscall int energy_meter_get(const struct device *dev,
			       const struct energy_meter_channel *channels, int64_t *values,
			       size_t count);

static inline int z_impl_energy_meter_get(const struct device *dev,
					  const struct energy_meter_channel *channels,
					  int64_t *values, size_t count)
{
	return DEVICE_API_GET(energy_meter, dev)->get(dev, channels, values, count);
}

/**
 * @brief Read a single value from an energy meter.
 *
 * @param dev Energy meter device.
 * @param quantity Quantity to read.
 * @param line Line to read.
 * @param value Filled with the value.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The quantity is not supported on this line.
 * @retval -errno Other negative errno code on failure.
 */
static inline int energy_meter_get_one(const struct device *dev, uint8_t quantity, uint8_t line,
				       int64_t *value)
{
	const struct energy_meter_channel channel = {.quantity = quantity, .line = line};

	return energy_meter_get(dev, &channel, value, 1);
}

/**
 * @brief Reset the accumulated energy of an energy meter to zero.
 *
 * @param dev Energy meter device.
 *
 * @retval 0 Success.
 * @retval -ENOSYS The driver does not support resetting energy.
 * @retval -errno Other negative errno code on failure.
 */
__syscall int energy_meter_reset_energy(const struct device *dev);

static inline int z_impl_energy_meter_reset_energy(const struct device *dev)
{
	const struct energy_meter_driver_api *api = DEVICE_API_GET(energy_meter, dev);

	if (api->reset_energy == NULL) {
		return -ENOSYS;
	}

	return api->reset_energy(dev);
}

/**
 * @brief Set the callback for energy meter events.
 *
 * @param dev Energy meter device.
 * @param cb Callback, or NULL to stop receiving events.
 * @param user_data Passed to the callback.
 *
 * @retval 0 Success.
 * @retval -ENOSYS The driver does not report events.
 * @retval -errno Other negative errno code on failure.
 */
static inline int energy_meter_set_event_callback(const struct device *dev,
						  energy_meter_event_cb_t cb, void *user_data)
{
	const struct energy_meter_driver_api *api = DEVICE_API_GET(energy_meter, dev);

	if (api->set_event_callback == NULL) {
		return -ENOSYS;
	}

	return api->set_event_callback(dev, cb, user_data);
}

/**
 * @brief Values of one line, as filled by energy_meter_get_snapshot().
 *
 * Units are those of the matching @ref energy_meter_quantity.
 */
struct energy_meter_line_values {
	int64_t voltage;                /**< @ref ENERGY_METER_VOLTAGE */
	int64_t voltage_ll;             /**< @ref ENERGY_METER_VOLTAGE_LL */
	int64_t current;                /**< @ref ENERGY_METER_CURRENT */
	int64_t active_power;           /**< @ref ENERGY_METER_ACTIVE_POWER */
	int64_t reactive_power;         /**< @ref ENERGY_METER_REACTIVE_POWER */
	int64_t apparent_power;         /**< @ref ENERGY_METER_APPARENT_POWER */
	int64_t power_factor;           /**< @ref ENERGY_METER_POWER_FACTOR */
	int64_t phase_angle;            /**< @ref ENERGY_METER_PHASE_ANGLE */
	int64_t voltage_angle;          /**< @ref ENERGY_METER_VOLTAGE_ANGLE */
	int64_t frequency;              /**< @ref ENERGY_METER_FREQUENCY */
	int64_t active_energy_import;   /**< @ref ENERGY_METER_ACTIVE_ENERGY_IMPORT */
	int64_t active_energy_export;   /**< @ref ENERGY_METER_ACTIVE_ENERGY_EXPORT */
	int64_t reactive_energy_import; /**< @ref ENERGY_METER_REACTIVE_ENERGY_IMPORT */
	int64_t reactive_energy_export; /**< @ref ENERGY_METER_REACTIVE_ENERGY_EXPORT */
	int64_t apparent_energy;        /**< @ref ENERGY_METER_APPARENT_ENERGY */
	int64_t temperature;            /**< @ref ENERGY_METER_TEMPERATURE */
};

/** @cond INTERNAL_HIDDEN */
BUILD_ASSERT(sizeof(struct energy_meter_line_values) ==
		     ENERGY_METER_QUANTITY_COMMON_COUNT * sizeof(int64_t),
	     "struct energy_meter_line_values must have one field per common quantity");

/* energy_meter_line_value() indexes the fields by quantity */
#define Z_ENERGY_METER_FIELD_CHECK(field, quantity)                                                \
	BUILD_ASSERT(offsetof(struct energy_meter_line_values, field) ==                           \
			     (quantity) * sizeof(int64_t),                                         \
		     "field " #field " is out of quantity order")

Z_ENERGY_METER_FIELD_CHECK(voltage, ENERGY_METER_VOLTAGE);
Z_ENERGY_METER_FIELD_CHECK(voltage_ll, ENERGY_METER_VOLTAGE_LL);
Z_ENERGY_METER_FIELD_CHECK(current, ENERGY_METER_CURRENT);
Z_ENERGY_METER_FIELD_CHECK(active_power, ENERGY_METER_ACTIVE_POWER);
Z_ENERGY_METER_FIELD_CHECK(reactive_power, ENERGY_METER_REACTIVE_POWER);
Z_ENERGY_METER_FIELD_CHECK(apparent_power, ENERGY_METER_APPARENT_POWER);
Z_ENERGY_METER_FIELD_CHECK(power_factor, ENERGY_METER_POWER_FACTOR);
Z_ENERGY_METER_FIELD_CHECK(phase_angle, ENERGY_METER_PHASE_ANGLE);
Z_ENERGY_METER_FIELD_CHECK(voltage_angle, ENERGY_METER_VOLTAGE_ANGLE);
Z_ENERGY_METER_FIELD_CHECK(frequency, ENERGY_METER_FREQUENCY);
Z_ENERGY_METER_FIELD_CHECK(active_energy_import, ENERGY_METER_ACTIVE_ENERGY_IMPORT);
Z_ENERGY_METER_FIELD_CHECK(active_energy_export, ENERGY_METER_ACTIVE_ENERGY_EXPORT);
Z_ENERGY_METER_FIELD_CHECK(reactive_energy_import, ENERGY_METER_REACTIVE_ENERGY_IMPORT);
Z_ENERGY_METER_FIELD_CHECK(reactive_energy_export, ENERGY_METER_REACTIVE_ENERGY_EXPORT);
Z_ENERGY_METER_FIELD_CHECK(apparent_energy, ENERGY_METER_APPARENT_ENERGY);
Z_ENERGY_METER_FIELD_CHECK(temperature, ENERGY_METER_TEMPERATURE);

#undef Z_ENERGY_METER_FIELD_CHECK
/** @endcond */

/**
 * @brief All common values of an energy meter, as filled by energy_meter_get_snapshot().
 *
 * A single-phase meter fills line A and, if it measures them, line N and the total.
 * A three-phase meter fills lines A to C, N and the total.
 */
struct energy_meter_snapshot {
	/** Number of phases of the meter: 1 or 3. */
	uint8_t phases;
	/** Bit masks of BIT(quantity) for the fields that hold a value, per line. */
	uint32_t valid[ENERGY_METER_LINE_COUNT];
	/** Values, indexed by @ref energy_meter_line. */
	struct energy_meter_line_values line[ENERGY_METER_LINE_COUNT];
};

/**
 * @brief Read every common value an energy meter supports.
 *
 * The values of each line are read with one energy_meter_get() call. Fields the meter does
 * not support are set to 0 and their bit in @ref energy_meter_snapshot.valid is cleared.
 *
 * @param dev Energy meter device.
 * @param snapshot Filled with the values.
 *
 * @retval 0 Success.
 * @retval -errno Negative errno code from energy_meter_get_info() or energy_meter_get().
 */
int energy_meter_get_snapshot(const struct device *dev, struct energy_meter_snapshot *snapshot);

/**
 * @brief Get the value of a quantity from a snapshot line.
 *
 * @param values Line values.
 * @param quantity Common quantity.
 *
 * @return The value, in the units of the quantity.
 */
static inline int64_t energy_meter_line_value(const struct energy_meter_line_values *values,
					      enum energy_meter_quantity quantity)
{
	return ((const int64_t *)values)[quantity];
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/energy_meter.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_ENERGY_METER_H_ */
