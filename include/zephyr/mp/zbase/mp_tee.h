/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tee element for pipeline branching.
 *
 * The tee element splits a single input branch into multiple output branches.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_TEE_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_TEE_H_

/**
 * @defgroup mp_tee Tee
 * @ingroup mp_zbase
 * @brief Pipeline branching element (1 sink pad , N source pad)
 * @{
 */

#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>

/**
 * @brief Tee property identifiers
 *
 * Enumeration of properties that can be configured for a tee element
 */
enum prop_tee {
	/** Number of source pads */
	PROP_TEE_SRCPADS_NUM,
};

/**
 * @brief Tee Element Structure
 *
 * The tee element receives buffers on its single sink pad and
 * broadcasts them to all of its source pads.
 */
struct mp_tee {
	/** Base element structure */
	struct mp_element element;
	/** @cond INTERNAL_HIDDEN */
	/** Pointer to the supported caps (ANY) */
	struct mp_caps *caps;
	/** @endcond */
	/** Sink pad for receiving input data */
	struct mp_pad sinkpad;
	/** Source pad array for sending data */
	struct mp_pad srcpads[CONFIG_MP_TEE_MAX_SRCPADS_NUM];
	/** Number of source pads */
	uint8_t srcpads_num;
};

/**
 * @brief Initialize a tee element
 *
 * Initializes the tee element with one sink pad and two default srcpads.
 *
 * @param self Pointer to the @ref mp_element to initialize as a tee
 */
void mp_tee_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_TEE_H_ */
