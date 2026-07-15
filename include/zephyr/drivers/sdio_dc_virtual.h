/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helpers for the virtual (loopback) SDIO device controller
 *
 * The virtual SDIO device controller implements the @ref sdio_dc_interface
 * entirely in software and, in addition, exposes an in-process loopback
 * transport that a host-role @ref sdio_dev can bind to. This lets the full
 * host -> bus -> device path be exercised on platforms without SDIO hardware
 * (e.g. native_sim), and is intended for tests and samples.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_VIRTUAL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_VIRTUAL_H_

#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/sdio/sdio_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Host-side interrupt callback for the loopback controller.
 *
 * Invoked (from the controller context) when the device side raises the SDIO
 * interrupt towards the host.
 *
 * @param dc   virtual controller device
 * @param func function that asserted the interrupt
 * @param user user data registered with @ref sdio_dc_virtual_set_irq_cb
 */
typedef void (*sdio_dc_virtual_irq_cb_t)(const struct device *dc,
					 enum sdio_func_num func, void *user);

/**
 * @brief Get the loopback host transport vtable.
 *
 * Use together with @ref sdio_dc_virtual_loopback_ctx and @ref sdio_dev_init
 * to create a host-role @ref sdio_dev that drives the virtual controller.
 *
 * @return host transport vtable
 */
const struct sdio_transport_api *sdio_dc_virtual_loopback_api(void);

/**
 * @brief Get the loopback transport context for a virtual controller.
 *
 * @param dc virtual controller device
 * @return context pointer to pass to @ref sdio_dev_init
 */
const void *sdio_dc_virtual_loopback_ctx(const struct device *dc);

/**
 * @brief Register a host-side interrupt callback on the loopback controller.
 *
 * @param dc   virtual controller device
 * @param cb   callback invoked when the device raises an interrupt
 * @param user user data passed to @p cb
 */
void sdio_dc_virtual_set_irq_cb(const struct device *dc,
				sdio_dc_virtual_irq_cb_t cb, void *user);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_VIRTUAL_H_ */
