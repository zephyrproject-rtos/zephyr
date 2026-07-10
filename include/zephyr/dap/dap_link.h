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
#include <zephyr/kernel.h>

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
	/* Serializes debug port access, see dap_link_lock() */
	struct k_mutex lock;
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
		.lock = Z_MUTEX_INITIALIZER(ctx_name.lock),			\
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
 * @brief Lock the debug port used by a DAP Link context.
 *
 * A backend holds this lock while it executes DAP commands via
 * dap_link_execute_cmd(). An application that accesses the same SWD/JTAG
 * interface directly (for example to poll a SEGGER RTT control block or to
 * program target memory) can hold the lock across its own transfer sequence
 * to prevent interleaving with DAP commands issued by the debug host.
 *
 * The lock may be acquired recursively by the same thread.
 *
 * @warning Keep hold times short. The USB backend calls
 * dap_link_execute_cmd() from its transfer completion handler, which runs
 * in the USB device stack context and blocks on this lock without a
 * timeout. While an application holds the lock (or a DAP command batch
 * executes), that context cannot service other transfers — every USB
 * function on the device stalls, not just DAP. Do not hold the lock
 * across waits for external events; split long transfer sequences into
 * short locked sections.
 *
 * @param[in] dap_link_ctx Context whose debug port is to be locked.
 */
void dap_link_lock(struct dap_link_context *const dap_link_ctx);

/**
 * @brief Unlock the debug port used by a DAP Link context.
 *
 * @param[in] dap_link_ctx Context whose debug port is to be unlocked.
 */
void dap_link_unlock(struct dap_link_context *const dap_link_ctx);

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
