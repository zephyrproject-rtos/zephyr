/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header file for DAP Link API.
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
 * @brief Zephyr DAP Link API
 * @defgroup DAP API
 * @ingroup io_interfaces
 * @since 4.4
 * @version 0.1.0
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 * Degug Access Port (DAP) Link runtime context
 *
 */
struct dap_link_context {
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
};

/** @endcond */

/**
 * @brief Define Zephyr DAP Link context structure
 *
 * Example of use:
 *
 * @code{.c}
 * DAP_LINK_CONTEXT_DEFINE(sample_link_ctx,
 *                         DEVICE_DT_GET_ONE(zephyr_swdp_gpio));
 * @endcode
 *
 * @param ctx_name DAP Link context name
 * @param ctx_dev Pointer to SWDP device
 */
#define DAP_LINK_CONTEXT_DEFINE(ctx_name, ctx_dev)				\
	static struct dap_link_context ctx_name = {				\
		.dev = ctx_dev,							\
	}

/**
 * @brief Initialize DAP Link
 *
 * @param[in] dap_link_ctx Pointer to DAP Link context
 *
 * @return 0 on success, or error code
 */
int dap_link_init(struct dap_link_context *const dap_link_ctx);

/**
 * @brief Set packet size used by a DAP Link backend
 *
 * @param[in] dap_link_ctx Pointer to DAP Link context
 * @param[in] pkt_size Packet size
 *
 */
void dap_link_set_pkt_size(struct dap_link_context *const dap_link_ctx,
			   const uint16_t pkt_size);

/**
 * @brief Initialize DAP Link USB backend
 *
 * @param[in] dap_link_ctx Pointer to DAP Link context
 *
 * @return 0 on success, or error code
 */
int dap_link_backend_usb_init(struct dap_link_context *const dap_link_ctx);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DAP_DAP_LINK_H */
