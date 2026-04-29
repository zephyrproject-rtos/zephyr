/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_transform_client.
 */

#ifndef __MP_TRANSFORM_CLIENT_H__
#define __MP_TRANSFORM_CLIENT_H__

#include "mp_buffer.h"
#include "mp_transform.h"

#define MP_TRANSFORM_CLIENT(self) ((struct mp_transform_client *)self)

/**
 * @brief Transform client element structure
 *
 * Base structure for all transform elements on the client side of multi-core pipeline.
 * These elements call RPC to the server side to off-load the processing work.
 */
struct mp_transform_client {
	/** Base transform structure */
	struct mp_transform transform;

	/**
	 * @brief Initialize RPC communication on the client side
	 */
	void (*init_rpc)(void);
	/**
	 * @brief RPC chain function for processing buffers on the server side
	 * @param in_buf Address of the input buffer to be processed
	 * @param in_sz Input buffer size
	 * @param out_buf Address of the processed output buffer
	 * @param out_sz Output buffer size
	 * @return true on success, false on failure
	 */
	bool (*chainfn_rpc)(uint32_t in_buf, uint32_t in_sz, uint32_t out_buf, uint32_t *out_sz);
};

/**
 * @brief Initialize a transform element
 *
 * This function initializes the base transform element structure,
 * sets up sink and source pads, and configures default function
 * pointers for element operations.
 *
 * @param self Pointer to the element to initialize (@ref struct mp_element)
 */
void mp_transform_client_init(struct mp_element *self);

#endif /* __MP_TRANSFORM_CLIENT_H__ */
