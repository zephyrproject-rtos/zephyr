/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup dap_link_interface
 * @brief Main header file for the DAP Link API.
 */

#ifndef ZEPHYR_INCLUDE_DAP_DAP_LINK_H
#define ZEPHYR_INCLUDE_DAP_DAP_LINK_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/swdp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Debug Access Port (DAP) Link backends.
 * @defgroup dap_link_interface DAP Link
 * @ingroup io_interfaces
 * @since 4.4
 * @version 0.1.0
 * @{
 */

/**
 * DAP Link runtime context (opaque type).
 */
struct dap_link_context {
	/** @cond INTERNAL_HIDDEN */
	/* Pointer to SWD or JTAG device. Only SWD is supported yet. */
	const struct device *dev;
	atomic_t state;
	/* Runtime debug port type */
	uint8_t debug_port;
	/* Debug port capabilities */
	uint8_t capabilities;
	/* Packet size used by a backend */
	uint16_t pkt_size;
	struct {
		/* Idle cycles after transfer */
		uint8_t idle_cycles;
		/* Number of retries after WAIT response */
		uint16_t retry_count;
		/* Number of retries if read value does not match */
		uint16_t match_retry;
		/* Match Mask */
		uint32_t match_mask;
	} transfer;
	/** @endcond */
};

/**
 * @brief Statically define a DAP Link context.
 *
 * Example usage:
 *
 * @code{.c}
 * DAP_LINK_CONTEXT_DEFINE(sample_link_ctx,
 *                         DEVICE_DT_GET_ONE(zephyr_swdp_gpio));
 * @endcode
 *
 * @param ctx_name Name of the static context object to define.
 * @param ctx_dev SWDP device used by the DAP Link backend.
 */
#define DAP_LINK_CONTEXT_DEFINE(ctx_name, ctx_dev)				\
	static struct dap_link_context ctx_name = {				\
		.dev = ctx_dev,							\
	}

/**
 * @brief Initialize a DAP Link context.
 *
 * @param[in] dap_link_ctx Context to initialize.
 *
 * @retval 0 Successfully initialized the context.
 * @retval -errno Negative error code on failure.
 */
int dap_link_init(struct dap_link_context *const dap_link_ctx);

/**
 * @brief Set the packet size used by a DAP Link backend.
 *
 * @param[in] dap_link_ctx Context to update.
 * @param[in] pkt_size Packet size (in bytes).
 *
 */
void dap_link_set_pkt_size(struct dap_link_context *const dap_link_ctx,
			   const uint16_t pkt_size);

/**
 * @brief Initialize the DAP Link USB backend.
 *
 * @param[in] dap_link_ctx Context to bind to the USB backend.
 *
 * @retval 0 Successfully initialized the USB backend.
 * @retval -errno Negative error code on failure.
 */
int dap_link_backend_usb_init(struct dap_link_context *const dap_link_ctx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DAP_DAP_LINK_H */
