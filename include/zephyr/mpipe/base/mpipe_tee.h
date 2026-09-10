/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tee element for pipeline branching.
 * @ingroup mpipe_tee
 *
 * The tee element splits a single input branch into multiple output branches.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_TEE_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_TEE_H_

/**
 * @defgroup mpipe_tee Tee
 * @ingroup mpipe_base
 * @brief Pipeline branching element: one sink pad, N source pads.
 * @{
 */

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>

/**
 * @brief Tee property identifiers
 *
 * Enumeration of properties that can be configured for a tee element
 */
enum mpipe_prop_base_tee {
	/** Number of source pads */
	MPIPE_PROP_BASE_TEE_SRC_PADS_NUM,
};

/**
 * @brief Tee Element Structure
 *
 * The tee element receives buffers on its single sink pad and
 * broadcasts them to all of its source pads.
 */
struct mpipe_tee {
	/** Base element structure */
	struct mpipe_element element;
	/** Sink pad for receiving input data */
	struct mpipe_pad sink_pad;
	/** Source pad array for sending data */
	struct mpipe_pad src_pads[CONFIG_MPIPE_BASE_TEE_MAX_SRC_PADS_NUM];
	/** Number of source pads */
	uint8_t src_pads_num;
};

/**
 * @brief Initialize a tee element
 *
 * Initializes the tee element with one sink pad and two default src_pads.
 *
 * @param tee Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_tee_init(struct mpipe_tee *tee, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_TEE_H_ */
