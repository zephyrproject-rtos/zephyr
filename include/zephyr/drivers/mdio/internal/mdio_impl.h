/*
 * Copyright (c) 2021 IP-Logix Inc.
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for MDIO driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MDIO_H_
#error "Should only be included by zephyr/drivers/mdio.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_MDIO_INTERNAL_MDIO_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_MDIO_INTERNAL_MDIO_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

static inline void z_impl_mdio_bus_enable(const struct device *dev)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->bus_enable != NULL) {
		api->bus_enable(dev);
	}
}

static inline void z_impl_mdio_bus_disable(const struct device *dev)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->bus_disable != NULL) {
		api->bus_disable(dev);
	}
}

static inline int z_impl_mdio_read(const struct device *dev, uint8_t prtad,
				   uint8_t regad, uint16_t *data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->read == NULL) {
		return -ENOSYS;
	}

	return api->read(dev, prtad, regad, data);
}

static inline int z_impl_mdio_write(const struct device *dev, uint8_t prtad,
				    uint8_t regad, uint16_t data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->write == NULL) {
		return -ENOSYS;
	}

	return api->write(dev, prtad, regad, data);
}

static inline int z_impl_mdio_read_c45(const struct device *dev, uint8_t prtad,
				       uint8_t devad, uint16_t regad,
				       uint16_t *data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->read_c45 == NULL) {
		return -ENOSYS;
	}

	return api->read_c45(dev, prtad, devad, regad, data);
}

static inline int z_impl_mdio_write_c45(const struct device *dev, uint8_t prtad,
					uint8_t devad, uint16_t regad,
					uint16_t data)
{
	const struct mdio_driver_api *api =
		(const struct mdio_driver_api *)dev->api;

	if (api->write_c45 == NULL) {
		return -ENOSYS;
	}

	return api->write_c45(dev, prtad, devad, regad, data);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MDIO_INTERNAL_MDIO_IMPL_H_ */
