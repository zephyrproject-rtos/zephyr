/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Hardware-backed video transform element.
 *
 * Wraps a Zephyr memory-to-memory video device to perform
 * hardware-accelerated operations such as format conversion and scaling.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_TRANSFORM_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_TRANSFORM_H_

#include <zephyr/mp/core/mp_transform.h>
#include <zephyr/mp/zvid/mp_zvid_object.h>

/**
 * @brief Hardware video transform element structure.
 *
 * Extends @ref mp_transform with separate input and output
 * @ref mp_zvid_object instances, enabling memory-to-memory video
 * transformations through the Zephyr video subsystem.
 */
struct mp_zvid_transform {
	/** Base transform element */
	struct mp_transform transform;
	/** Input video object for receiving video data */
	struct mp_zvid_object zvid_obj_in;
	/** Output video object for producing transformed video data */
	struct mp_zvid_object zvid_obj_out;
};

/**
 * @brief Initialize a hardware video transform element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_zvid_transform_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_TRANSFORM_H_ */
