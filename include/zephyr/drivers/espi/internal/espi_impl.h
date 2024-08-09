/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementations for eSPI APIs
 */

#ifndef ZEPHYR_INCLUDE_ESPI_H_
#error "Should only be included by zephyr/drivers/espi.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_INTERNAL_ESPI_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_INTERNAL_ESPI_IMPL_H_

#include <errno.h>
#include <stdint.h>

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_espi_config(const struct device *dev,
				     struct espi_cfg *cfg)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->config(dev, cfg);
}

static inline bool z_impl_espi_get_channel_status(const struct device *dev,
						  enum espi_channel ch)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->get_channel_status(dev, ch);
}

static inline int z_impl_espi_read_request(const struct device *dev,
					   struct espi_request_packet *req)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->read_request) {
		return -ENOTSUP;
	}

	return api->read_request(dev, req);
}

static inline int z_impl_espi_write_request(const struct device *dev,
					    struct espi_request_packet *req)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->write_request) {
		return -ENOTSUP;
	}

	return api->write_request(dev, req);
}

static inline int z_impl_espi_read_lpc_request(const struct device *dev,
					       enum lpc_peripheral_opcode op,
					       uint32_t *data)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->read_lpc_request) {
		return -ENOTSUP;
	}

	return api->read_lpc_request(dev, op, data);
}

static inline int z_impl_espi_write_lpc_request(const struct device *dev,
						enum lpc_peripheral_opcode op,
						uint32_t *data)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->write_lpc_request) {
		return -ENOTSUP;
	}

	return api->write_lpc_request(dev, op, data);
}

static inline int z_impl_espi_send_vwire(const struct device *dev,
					 enum espi_vwire_signal signal,
					 uint8_t level)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->send_vwire(dev, signal, level);
}

static inline int z_impl_espi_receive_vwire(const struct device *dev,
					    enum espi_vwire_signal signal,
					    uint8_t *level)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	return api->receive_vwire(dev, signal, level);
}

static inline int z_impl_espi_send_oob(const struct device *dev,
				       struct espi_oob_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->send_oob) {
		return -ENOTSUP;
	}

	return api->send_oob(dev, pckt);
}

static inline int z_impl_espi_receive_oob(const struct device *dev,
					  struct espi_oob_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->receive_oob) {
		return -ENOTSUP;
	}

	return api->receive_oob(dev, pckt);
}

static inline int z_impl_espi_read_flash(const struct device *dev,
					 struct espi_flash_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->flash_read) {
		return -ENOTSUP;
	}

	return api->flash_read(dev, pckt);
}

static inline int z_impl_espi_write_flash(const struct device *dev,
					  struct espi_flash_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->flash_write) {
		return -ENOTSUP;
	}

	return api->flash_write(dev, pckt);
}

static inline int z_impl_espi_flash_erase(const struct device *dev,
					  struct espi_flash_packet *pckt)
{
	const struct espi_driver_api *api =
		(const struct espi_driver_api *)dev->api;

	if (!api->flash_erase) {
		return -ENOTSUP;
	}

	return api->flash_erase(dev, pckt);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ESPI_INTERNAL_ESPI_IMPL_H_ */
