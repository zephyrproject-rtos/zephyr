/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake source element
 *
 * A simple emulated source element producing fake data buffers used for test purpose.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_FAKE_SRC_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_FAKE_SRC_H_

/**
 * @addtogroup mp_src
 * @{
 */

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_src.h>

/**
 * @brief Fake source element structure
 */
struct mp_fake_src {
	/** Base source element (must be first) */
	struct mp_src src;
	/** Buffer pool for managing output buffers */
	struct mp_buffer_pool pool;
};

/**
 * @brief Initialize a fake source element
 *
 * @param self Pointer to the @ref mp_element to initialize as a fake source
 */
void mp_fake_src_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_FAKE_SRC_H_ */
