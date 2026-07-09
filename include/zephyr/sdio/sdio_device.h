/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDIO device/slave-role API
 *
 * Device-role model layered on top of an SDIO device controller
 * (@ref sdio_dc_interface). It lets an application expose SDIO functions to a
 * remote host.
 */

#ifndef ZEPHYR_INCLUDE_SDIO_SDIO_DEVICE_H_
#define ZEPHYR_INCLUDE_SDIO_SDIO_DEVICE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sd/sd_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDIO device/slave-role abstraction
 * @defgroup sdio_device SDIO device/slave-role abstraction
 * @ingroup sdio_core
 * @{
 */

struct sdio_device_function;

/**
 * @brief FIFO / data-port handler.
 *
 * Invoked when the host performs a fixed-address (FIFO) access to the
 * function's configured data-port register. For a read the handler fills
 * @p data with @p len bytes; for a write it consumes them.
 *
 * @param func function being accessed
 * @param dir  access direction
 * @param data data buffer to fill (read) or consume (write)
 * @param len  number of bytes
 * @param user user data registered with the function
 * @retval 0 on success, negative errno to report an I/O error to the host
 */
typedef int (*sdio_device_fifo_cb_t)(struct sdio_device_function *func,
				     enum sdio_io_dir dir, uint8_t *data,
				     uint32_t len, void *user);

/**
 * @brief Device-side SDIO function.
 *
 * Configure @ref num and at least one of a register window (@ref regs /
 * @ref regs_size) or a FIFO handler (@ref fifo_reg / @ref fifo_cb), then
 * register it with @ref sdio_device_register_function.
 */
struct sdio_device_function {
	/** Function number exposed to the host */
	enum sdio_func_num num;
	/** Backing store for incrementing-address accesses (may be NULL) */
	uint8_t *regs;
	/** Size of @ref regs in bytes */
	size_t regs_size;
	/** Register offset mapped to the FIFO/data port (when @ref fifo_cb set) */
	uint32_t fifo_reg;
	/** Handler for fixed-address FIFO/data-port accesses (may be NULL) */
	sdio_device_fifo_cb_t fifo_cb;
	/** User data passed to @ref fifo_cb */
	void *user;

	/** @cond INTERNAL_HIDDEN */
	sys_snode_t node;
	struct sdio_device *parent;
	/** @endcond */
};

/**
 * @brief Device-role SDIO endpoint.
 *
 * Binds a set of @ref sdio_device_function instances to an SDIO device
 * controller.
 */
struct sdio_device {
	/** SDIO device controller backing this endpoint */
	const struct device *controller;
	/** @cond INTERNAL_HIDDEN */
	sys_slist_t functions;
	struct k_mutex lock;
	/** @endcond */
};

/**
 * @brief Initialize a device-role SDIO endpoint.
 *
 * @param dev        endpoint to initialize
 * @param controller SDIO device controller device
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_device_init(struct sdio_device *dev, const struct device *controller);

/**
 * @brief Expose a function to the remote host.
 *
 * @param dev  device endpoint
 * @param func function description (must outlive the registration)
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 * @retval -EALREADY a function with the same number is already registered
 */
int sdio_device_register_function(struct sdio_device *dev,
				  struct sdio_device_function *func);

/**
 * @brief Start answering host accesses.
 *
 * @param dev device endpoint
 * @retval 0 on success, negative errno otherwise
 */
int sdio_device_enable(struct sdio_device *dev);

/**
 * @brief Stop answering host accesses.
 *
 * @param dev device endpoint
 * @retval 0 on success, negative errno otherwise
 */
int sdio_device_disable(struct sdio_device *dev);

/**
 * @brief Assert the SDIO interrupt towards the host on behalf of a function.
 *
 * @param func function asserting the interrupt
 * @retval 0 on success
 * @retval -ENOSYS controller cannot assert interrupts
 * @retval -EINVAL invalid argument
 */
int sdio_device_raise_interrupt(struct sdio_device_function *func);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SDIO_SDIO_DEVICE_H_ */
