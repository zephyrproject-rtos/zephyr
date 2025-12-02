/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for JDI (Japan Display Interface) drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_JDI_H_
#define ZEPHYR_INCLUDE_DRIVERS_JDI_H_

/**
 * @brief JDI driver APIs
 * @defgroup jdi_interface JDI driver APIs
 * @since 3.1
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/jdi/jdi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name JDI Device mode flags.
 * @{
 */

/** Partial update mode */
#define JDI_MODE_PARTIAL_UPDATE BIT(0)

/** @} */

/** JDI device. */
struct jdi_device {
	/**Input pixel format. */
	uint32_t input_pixfmt;
	/**Resolution. */
	uint32_t width;
	uint32_t height;
	/** Mode flags. */
	uint32_t mode_flags;
};

/** JDI message. */
struct jdi_msg {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
	/** Transmit buffer. */
	void *tx_buf;
	/** Transmit length. */
	size_t tx_len;
};

/**
 * @typedef jdi_attach_api
 * @brief Callback API for attaching a JDI device
 * See jdi_attach() for argument description
 */
typedef int (*jdi_attach_api)(const struct device *dev, const struct jdi_device *jdev);

/**
 * @typedef jdi_detach_api
 * @brief Callback API for detaching a JDI device
 * See jdi_detach() for argument description
 */
typedef int (*jdi_detach_api)(const struct device *dev);

/**
 * @typedef jdi_transfer_api
 * @brief Callback API for transferring data
 * See jdi_transfer() for argument description
 */
typedef ssize_t (*jdi_transfer_api)(const struct device *dev, const struct jdi_msg *msg);

/** JDI driver API structure. */
__subsystem struct jdi_driver_api {
	jdi_attach_api attach;
	jdi_detach_api detach;
	jdi_transfer_api transfer;
};

/**
 * @brief Attach a JDI device to the controller
 *
 * @param dev JDI controller device
 * @param jdev JDI device to attach
 * @return 0 on success, negative errno code on failure
 */
static inline int jdi_attach(const struct device *dev, const struct jdi_device *jdev)
{
	const struct jdi_driver_api *api = (const struct jdi_driver_api *)dev->api;

	return api->attach(dev, jdev);
}

/**
 * @brief Detach a JDI device from the controller
 *
 * @param dev JDI controller device
 * @return 0 on success, negative errno code on failure
 */
static inline int jdi_detach(const struct device *dev)
{
	const struct jdi_driver_api *api = (const struct jdi_driver_api *)dev->api;

	return api->detach(dev);
}

/**
 * @brief Transfer data using JDI
 *
 * @param dev JDI controller device
 * @param msg JDI message
 * @return Number of bytes transferred on success, negative errno code on failure
 */
static inline ssize_t jdi_transfer(const struct device *dev, const struct jdi_msg *msg)
{
	const struct jdi_driver_api *api = (const struct jdi_driver_api *)dev->api;

	return api->transfer(dev, msg);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_JDI_H_ */
