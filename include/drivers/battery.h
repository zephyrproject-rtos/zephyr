/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Battery public API header file.
 */

#ifndef __BATTERY_H__
#define __BATTERY_H__

/**
 * @brief Battery Interface
 * @defgroup battery_interface Battery Interface
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Battery parameter id */
enum battery_parameter_id {
	/* Manufacturer access */
	MFR_ACCESS = 0,
	/* Battery mode */
	BATTERY_MODE,
	/* Temperature */
	TEMPERATURE,
	/* Voltage */
	VOLTAGE,
	/* Current */
	CURRENT,
	/* Average current */
	AVERAGE_CURRENT,
	/* Relative state of charge */
	REL_STATE_CHARGE,
	/* Remaining capacity */
	REM_CAPACITY,
	/* Full charge capacity */
	FULL_CHRG_CAPACITY,
	/* Charging current */
	CHARGING_CURRENT,
	/* Charging voltage */
	CHARGING_VOLTAGE,
	/* Battery status */
	BATTERY_STATUS,
	/* Cycle count */
	CYCLE_COUNT,
	/* Design capacity */
	DESIGN_CAPACITY,
	/* Design voltage */
	DESIGN_VOLTAGE,
	/* Rsvd(0x1D) - Fast charging current */
	RSVD_FAST_CHRG_CURRENT,
	/* Rsvd(0x1E) - Fast charging voltage */
	RSVD_FAST_CHRG_VOLTAGE,
	/* Mfg fun4(0x3C) - Cell3 voltage */
	MFG_FUN4_CELL3_VOLTAGE,
	/* Mfg fun3(0x3D) - Cell2 voltage */
	MFG_FUN3_CELL2_VOLTAGE,
	/* Mfg fun2(0x3E) - Cell1 voltage */
	MFG_FUN2_CELL1_VOLTAGE,
	/* Mfg fun1(0x3F) - Cell0 voltage */
	MFG_FUN1_CELL0_VOLTAGE,
	/* Max peak power */
	PMAX,
	/* Max sustained power */
	PBSS,
	/* Max peak current */
	CMPP,
	/* Battery resistance */
	RBHF,
	/* Battery no load voltage */
	VBNL,

	/* Invalid id */
	BAT_PARAM_ID_MAX,
};

/* Battery mode config */
struct battery_mode_cfg {
	/* 0: Enable, 1: Disable */
	u8_t alarm_mode:1;
	/* 0: Enable, 1: Disable */
	u8_t charger_mode:1;
	/* 0: Report in mA or mAh, 1: Report in 10mW or 10mWh */
	u8_t capacity_mode:1;
};

/**
 * Battery driver API interface.
 */
struct battery_driver_api {
	/* Set battery mode */
	int (*set_mode)(struct device *dev, struct battery_mode_cfg *cfg);

	/* Set manufacturer access */
	int (*set_mfr_access)(struct device *dev, u16_t value);

	/* Set system resistance */
	int (*set_sys_resistance)(struct device *dev, u16_t value);

	/* Set min system voltage */
	int (*set_min_sys_voltage)(struct device *dev, u16_t voltage);

	/* Get various battery parameters */
	int (*get_parameter)(struct device *dev, u16_t *value,
			     enum battery_parameter_id id);

	/* Get manufacturer name */
	int (*get_manfctr_name)(struct device *dev, u8_t *data);

	/* Get device name */
	int (*get_device_name)(struct device *dev, u8_t *data);
};

/**
 * @brief Set 'mode' of battery device.
 *
 * This routine writes capacity report mode to battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Battery configuration parameters.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_set_mode(struct device *dev,
			       struct battery_mode_cfg *cfg);

static inline int z_impl_battery_set_mode(struct device *dev,
					  struct battery_mode_cfg *cfg)
{
	const struct battery_driver_api *api;

	if (!dev) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->set_mode(dev, cfg);
}

/**
 * @brief Set 'manufacturer access' of battery device.
 *
 * This routine writes manufacturer access to battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_set_manufacturer_access(struct device *dev,
					      u16_t value);

static inline int z_impl_battery_set_manufacturer_access(struct device *dev,
							 u16_t value)
{
	const struct battery_driver_api *api;

	if (!dev) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->set_mfr_access(dev, value);
}

/**
 * @brief Set 'system resistance' of battery device.
 *
 * This routine writes system resistance to battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_set_sys_resistance(struct device *dev,
					 u16_t value);

static inline int z_impl_battery_set_sys_resistance(struct device *dev,
						    u16_t value)
{
	const struct battery_driver_api *api;

	if (!dev) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->set_sys_resistance(dev, value);
}

/**
 * @brief Set 'min system voltage' of battery device.
 *
 * This routine writes min system voltage to battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param voltage Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_set_min_sys_voltage(struct device *dev,
					  u16_t voltage);

static inline int z_impl_battery_set_min_sys_voltage(struct device *dev,
						     u16_t voltage)
{
	const struct battery_driver_api *api;

	if (!dev) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->set_min_sys_voltage(dev, voltage);
}

/**
 * @brief Get 'various parameters' of battery device.
 *
 * This routine reads various parameters from battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value buffer pointer to read internal register value.
 * @param id battery parameter id of internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_get_parameter(struct device *dev,
				    u16_t *value,
				    enum battery_parameter_id id);

static inline int z_impl_battery_get_parameter(struct device *dev,
					       u16_t *value,
					       enum battery_parameter_id id)
{
	const struct battery_driver_api *api;

	if (!dev || (id >= BAT_PARAM_ID_MAX)) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->get_parameter(dev, value, id);
}

/**
 * @brief Get 'manufacturer name' of battery device.
 *
 * This routine reads manufacturer name from battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param data buffer pointer to read internal register value.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_get_manufacturer_name(struct device *dev,
					    u8_t *data);

static inline int z_impl_battery_get_manufacturer_name(struct device *dev,
						       u8_t *data)
{
	const struct battery_driver_api *api;

	if (!dev) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->get_manfctr_name(dev, data);
}

/**
 * @brief Get 'device name' of battery device.
 *
 * This routine reads device name from battery internal register
 * of battery device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param data buffer pointer to read internal register value.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int battery_get_device_name(struct device *dev,
				      u8_t *data);

static inline int z_impl_battery_get_device_name(struct device *dev,
						 u8_t *data)
{
	const struct battery_driver_api *api;

	if (!dev) {
		return -ENOTSUP;
	}

	api = (const struct battery_driver_api *)dev->driver_api;
	return api->get_device_name(dev, data);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/battery.h>

#endif /* __BATTERY_H__ */
