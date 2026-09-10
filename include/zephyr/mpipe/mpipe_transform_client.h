/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transform client: a transform whose work runs on another core.
 * @ingroup mpipe_transform
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_TRANSFORM_CLIENT_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_TRANSFORM_CLIENT_H_

/**
 * @addtogroup mpipe_transform
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Transform client element structure
 *
 * Base structure for all transform elements on the client side of multi-core pipeline.
 * These elements call RPC to the server side to off-load the processing work.
 */
struct mpipe_transform_client {
	/** Base transform structure */
	struct mpipe_transform transform;

	/**
	 * @brief Initialize RPC communication on the client side
	 *
	 * @return 0 on success, an errno on failure
	 */
	int (*init_rpc)(void);
	/**
	 * @brief RPC chain function for processing buffers on the server side
	 *
	 * @param in_buf Address of the input buffer to be processed
	 * @param in_sz Input buffer size
	 * @param out_buf Address of the processed output buffer
	 * @param out_sz Output buffer size
	 * @return 0 on success, an errno on failure
	 */
	int (*chain_fn_rpc)(uint32_t in_buf, uint32_t in_sz, uint32_t out_buf, uint32_t *out_sz);
};

/**
 * @brief Initialize a transform client element
 *
 * Initializes the base transform element and installs the chain function that
 * forwards a buffer over the transport instead of processing it locally.
 *
 * @c init_rpc must already be set when this is called: it runs before the base
 * element is initialized, so a transport that cannot come up is reported
 * through the return value and nothing is left half-built.
 *
 * @param transform_client Pointer to the @ref mpipe_transform_client to
 *                         initialize.
 * @param id               Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_transform_client_init(struct mpipe_transform_client *transform_client, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_TRANSFORM_CLIENT_H_ */
