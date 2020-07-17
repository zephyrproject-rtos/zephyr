/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Charger public API header file.
 */

#ifndef __CHARGER_H__
#define __CHARGER_H__

/**
 * @brief Charger Interface
 * @defgroup charger_interface Charger Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Charger control registers id */
enum charger_control_id {
	CHARGER_CONTROL0 = 0,
	CHARGER_CONTROL1,
	CHARGER_CONTROL2,
	CHARGER_CONTROL3,
	CHARGER_CONTROL4,
	CHARGER_CONTROL5,
	CHARGER_CONTROL6,
	CHARGER_CONTROL7,

	/* Invalid id */
	CHARGER_CONTROL_MAX,
};

/* Charger current limit registers id */
enum charger_curr_limit_id {
	CHARGER_CHARGE_CURR_LIMIT = 0,
	CHARGER_ADP_CURR_LIMIT1,
	CHARGER_ADP_CURR_LIMIT2,

	/* Invalid id */
	CHARGER_CURR_LIMIT_MAX,
};

/* Charger information registers id */
enum charger_information_id {
	CHARGER_INFORMATION1 = 0,
	CHARGER_INFORMATION2,
	CHARGER_INFORMATION3,
	CHARGER_INFORMATION4,

	/* Invalid id */
	CHARGER_INFORMATION_MAX,
};

struct control_reg_cfg {
	enum charger_control_id id;
	u16_t val;
};

struct curr_limit_reg_cfg {
	enum charger_curr_limit_id id;
	u16_t val;
};

/* Charger voltage config */
struct charger_voltage_cfg {
	u16_t max_sys_voltage;
	u16_t min_sys_voltage;
};

/* Charger control register config */
struct charger_control_cfg {
	u8_t num_reg;
	struct control_reg_cfg control[];
};

/* Charger current limit config */
struct charger_curr_limit_cfg {
	u8_t num_reg;
	struct curr_limit_reg_cfg curr_lmt[];
};

/**
 * Charger driver API interface.
 */
struct charger_driver_api {
	/* Configure the charger max and min system voltage */
	int (*config_voltage)(struct device *dev,
			const struct charger_voltage_cfg *cfg);

	/* Configure various charger options */
	int (*config_control)(struct device *dev,
			const struct charger_control_cfg *cfg);

	/* Configure charger current limits */
	int (*config_curr_limit)(struct device *dev,
			const struct charger_curr_limit_cfg *cfg);

	/* Set adapter current prochot threshold */
	int (*set_ac_prochot)(struct device *dev, u16_t current);

	/* Set battery discharging current prochot threshold */
	int (*set_dc_prochot)(struct device *dev, u16_t current);

	/* Get various charger options */
	int (*get_control)(struct device *dev, u16_t *value,
			enum charger_control_id id);

	/* Get various charger status */
	int (*get_information)(struct device *dev, u16_t *value,
			enum charger_information_id id);

	/* Get charger manufacturer id */
	int (*get_manufacturer_id)(struct device *dev, u16_t *value);

	/* Get charger device id */
	int (*get_device_id)(struct device *dev, u16_t *value);
};

/**
 * @brief Configure 'min and max voltage' of charger device.
 *
 * This routine writes the charger min and max voltage to internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Voltage configuration parameters.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_configure_voltage(struct device *dev,
			const struct charger_voltage_cfg *cfg);

static inline int z_impl_charger_configure_voltage(struct device *dev,
				const struct charger_voltage_cfg *cfg)
{
	const struct charger_driver_api *api;

	if (!dev || !cfg) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->config_voltage(dev, cfg);
}

/**
 * @brief Configure 'various charger options' of charger device.
 *
 * This routine writes the various charger options to internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Controls configuration parameters.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_configure_control(struct device *dev,
			const struct charger_control_cfg *cfg);

static inline int z_impl_charger_configure_control(struct device *dev,
				const struct charger_control_cfg *cfg)
{
	const struct charger_driver_api *api;

	if (!dev || !cfg || !cfg->num_reg) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->config_control(dev, cfg);
}

/**
 * @brief Configure 'current limits' of charger device.
 *
 * This routine writes the charger current limits to internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg Current limit configuration parameters.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_configure_curr_limit(struct device *dev,
			const struct charger_curr_limit_cfg *cfg);

static inline int z_impl_charger_configure_curr_limit(struct device *dev,
				const struct charger_curr_limit_cfg *cfg)
{
	const struct charger_driver_api *api;

	if (!dev || !cfg || !cfg->num_reg) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->config_curr_limit(dev, cfg);
}

/**
 * @brief Set 'ac prochot' of charger device.
 *
 * This routine writes the ac prochot threshold to internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param current Current value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_set_ac_prochot(struct device *dev,
				     u16_t current);

static inline int z_impl_charger_set_ac_prochot(struct device *dev,
						u16_t current)
{
	const struct charger_driver_api *api;

	if (!dev) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->set_ac_prochot(dev, current);
}

/**
 * @brief Set 'dc prochot' of charger device.
 *
 * This routine writes the dc prochot threshold to internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param current Current value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_set_dc_prochot(struct device *dev,
				     u16_t current);

static inline int z_impl_charger_set_dc_prochot(struct device *dev,
						u16_t current)
{
	const struct charger_driver_api *api;

	if (!dev) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->set_dc_prochot(dev, current);
}

/**
 * @brief Get 'various charger options' of charger device.
 *
 * This routine reads various charger options from internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value buffer pointer to read internal register value.
 * @param id identity to the internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_get_control(struct device *dev,
				  u16_t *value,
				  enum charger_control_id id);

static inline int z_impl_charger_get_control(struct device *dev,
					     u16_t *value,
					     enum charger_control_id id)
{
	const struct charger_driver_api *api;

	if (!dev) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->get_control(dev, value, id);
}

/**
 * @brief Get 'various information' of charger device.
 *
 * This routine reads various charger information from internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value buffer pointer to read internal register value.
 * @param id identity to the internal register.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_get_information(struct device *dev,
				      u16_t *value,
				      enum charger_information_id id);

static inline int z_impl_charger_get_information(struct device *dev,
						 u16_t *value,
						 enum charger_information_id id)
{
	const struct charger_driver_api *api;

	if (!dev) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->get_information(dev, value, id);
}

/**
 * @brief Get 'manufacturer identity' of charger device.
 *
 * This routine reads manufacturer identity from internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value buffer pointer to read internal register value.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_get_manufacturer_id(struct device *dev,
					  u16_t *value);

static inline int z_impl_charger_get_manufacturer_id(struct device *dev,
						     u16_t *value)
{
	const struct charger_driver_api *api;

	if (!dev) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->get_manufacturer_id(dev, value);
}

/**
 * @brief Get 'device identity' of charger device.
 *
 * This routine reads device identity from internal register
 * of charger device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param value buffer pointer to read internal register value.
 *
 * @retval 0 If successful.
 * @retval error if failed.
 */
__syscall int charger_get_device_id(struct device *dev,
				    u16_t *value);

static inline int z_impl_charger_get_device_id(struct device *dev,
					       u16_t *value)
{
	const struct charger_driver_api *api;

	if (!dev) {
		return -EINVAL;
	}

	api = (const struct charger_driver_api *)dev->driver_api;
	return api->get_device_id(dev, value);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/charger.h>

#endif /* __CHARGER_H__ */
