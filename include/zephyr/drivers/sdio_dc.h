/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SDIO device controllers
 *
 * An SDIO device controller (DC) is the slave/peripheral-side counterpart of
 * an SD host controller (@ref sdhc_interface). It exposes the function I/O space of the
 * local SoC to a remote SDIO host and signals the host through the SDIO interrupt line.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_H_

#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDIO device controller interface
 * @defgroup sdio_dc_interface SDIO device controller interface
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief SDIO device controller capabilities
 *
 * Reported by a controller through @ref sdio_dc_get_caps.
 */
struct sdio_dc_caps {
	/** Number of I/O functions the controller can expose (1-7) */
	uint8_t num_funcs;
	/** Maximum block size supported for block-mode transfers */
	uint16_t max_blk_size;
	/** Controller supports asserting the SDIO interrupt towards the host */
	bool interrupt_supported;
};

/**
 * @brief Direction of a host-initiated access, as seen by the device.
 */
enum sdio_dc_dir {
	/** Host is reading from device function I/O space */
	SDIO_DC_DIR_READ = 0,
	/** Host is writing to device function I/O space */
	SDIO_DC_DIR_WRITE = 1,
};

/**
 * @brief Description of a host-initiated access to device I/O space.
 *
 * Passed by the controller to the registered transfer callback whenever the
 * remote host performs a CMD52/CMD53 targeting a function exposed by this
 * device. The callback is expected to satisfy the access (move data to/from
 * @ref data) and return 0, or a negative errno to signal an I/O error to the
 * host.
 */
struct sdio_dc_xfer {
	/** Targeted function number */
	enum sdio_func_num func;
	/** Access direction */
	enum sdio_dc_dir dir;
	/** Register offset within the function I/O space */
	uint32_t reg;
	/** true: incrementing-address window; false: fixed-address FIFO/data port */
	bool increment;
	/** Data buffer to read from / write into */
	uint8_t *data;
	/** Length of the access in bytes */
	uint32_t len;
};

/**
 * @brief Host-initiated access callback.
 *
 * @param dev   SDIO device controller
 * @param xfer  description of the access; the callback owns satisfying it
 * @param user  user data supplied to @ref sdio_dc_set_xfer_callback
 * @retval 0 access satisfied
 * @retval <0 negative errno reported back to the host as an I/O error
 */
typedef int (*sdio_dc_xfer_cb_t)(const struct device *dev,
				 struct sdio_dc_xfer *xfer, void *user);

/** @cond INTERNAL_HIDDEN */
__subsystem struct sdio_dc_driver_api {
	int (*enable)(const struct device *dev);
	int (*disable)(const struct device *dev);
	int (*set_xfer_callback)(const struct device *dev,
				 sdio_dc_xfer_cb_t cb, void *user);
	int (*raise_interrupt)(const struct device *dev,
			       enum sdio_func_num func);
	int (*get_caps)(const struct device *dev, struct sdio_dc_caps *caps);
};
/** @endcond */

/**
 * @brief Enable the SDIO device controller.
 *
 * After this call the controller will start answering host accesses and
 * dispatching them to the registered callback.
 *
 * @param dev SDIO device controller
 * @retval 0 on success, negative errno otherwise
 */
static inline int sdio_dc_enable(const struct device *dev)
{
	const struct sdio_dc_driver_api *api = (const struct sdio_dc_driver_api *)
		dev->api;

	if (api->enable == NULL) {
		return -ENOSYS;
	}
	return api->enable(dev);
}

/**
 * @brief Disable the SDIO device controller.
 *
 * @param dev SDIO device controller
 * @retval 0 on success, negative errno otherwise
 */
static inline int sdio_dc_disable(const struct device *dev)
{
	const struct sdio_dc_driver_api *api = (const struct sdio_dc_driver_api *)
		dev->api;

	if (api->disable == NULL) {
		return -ENOSYS;
	}
	return api->disable(dev);
}

/**
 * @brief Register the host-initiated access callback.
 *
 * The SDIO device subsystem installs a single callback that demultiplexes
 * accesses to the registered functions; applications normally do not call
 * this directly.
 *
 * @param dev  SDIO device controller
 * @param cb   callback invoked on each host access
 * @param user user data passed to @p cb
 * @retval 0 on success, negative errno otherwise
 */
static inline int sdio_dc_set_xfer_callback(const struct device *dev,
					    sdio_dc_xfer_cb_t cb, void *user)
{
	const struct sdio_dc_driver_api *api = (const struct sdio_dc_driver_api *)
		dev->api;

	if (api->set_xfer_callback == NULL) {
		return -ENOSYS;
	}
	return api->set_xfer_callback(dev, cb, user);
}

/**
 * @brief Assert the SDIO interrupt towards the host for a function.
 *
 * @param dev  SDIO device controller
 * @param func function asserting the interrupt
 * @retval 0 on success
 * @retval -ENOSYS controller cannot assert interrupts
 */
static inline int sdio_dc_raise_interrupt(const struct device *dev,
					  enum sdio_func_num func)
{
	const struct sdio_dc_driver_api *api = (const struct sdio_dc_driver_api *)
		dev->api;

	if (api->raise_interrupt == NULL) {
		return -ENOSYS;
	}
	return api->raise_interrupt(dev, func);
}

/**
 * @brief Query controller capabilities.
 *
 * @param dev  SDIO device controller
 * @param caps filled with controller capabilities
 * @retval 0 on success, negative errno otherwise
 */
static inline int sdio_dc_get_caps(const struct device *dev,
				   struct sdio_dc_caps *caps)
{
	const struct sdio_dc_driver_api *api = (const struct sdio_dc_driver_api *)
		dev->api;

	if (api->get_caps == NULL) {
		return -ENOSYS;
	}
	return api->get_caps(dev, caps);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_H_ */
