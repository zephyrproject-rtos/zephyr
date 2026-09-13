/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Helpers for the virtual SDIO device controller
 *
 * The virtual SDIO device controller implements the @ref sdio_dc_interface
 * entirely in software. Accesses are injected directly (by a test or a
 * host-side bridge), so the device path can be exercised without SDIO
 * hardware. For tests and samples.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_VIRTUAL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_VIRTUAL_H_

#include <zephyr/device.h>
#include <zephyr/sd/sd_spec.h>
#include <zephyr/drivers/sdio_dc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Host-side interrupt callback for the virtual controller.
 *
 * Invoked when the device side raises the SDIO interrupt towards the host.
 *
 * @param dc   virtual controller device
 * @param func function that asserted the interrupt
 * @param user user data registered with @ref sdio_dc_virtual_set_irq_cb
 */
typedef void (*sdio_dc_virtual_irq_cb_t)(const struct device *dc,
					 enum sdio_func_num func, void *user);

/**
 * @brief Register a host-side interrupt callback on the virtual controller.
 *
 * @param dc   virtual controller device
 * @param cb   callback invoked when the device raises an interrupt
 * @param user user data passed to @p cb
 */
void sdio_dc_virtual_set_irq_cb(const struct device *dc,
				sdio_dc_virtual_irq_cb_t cb, void *user);

/**
 * @brief Inject a decoded host access into the virtual device controller.
 *
 * Injects a decoded host access (e.g. from a test or a host-side bridge);
 * routes it to the registered device functions.
 *
 * @param dc   virtual controller device
 * @param xfer decoded access
 * @retval 0 on success, negative errno otherwise
 */
int sdio_dc_virtual_access(const struct device *dc, struct sdio_dc_xfer *xfer);

/**
 * @brief Last buffer filled through the zero-copy RX path (test inspection).
 *
 * @param dc virtual controller device
 * @return pointer to the most recently completed RX buffer, or NULL
 */
uint8_t *sdio_dc_virtual_last_rx(const struct device *dc);

/**
 * @brief Last buffer consumed through the zero-copy TX path (test inspection).
 *
 * @param dc virtual controller device
 * @return pointer to the most recently completed TX buffer, or NULL
 */
uint8_t *sdio_dc_virtual_last_tx(const struct device *dc);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SDIO_DC_VIRTUAL_H_ */
