/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fake source element
 * @ingroup mpipe_src
 *
 * A simple emulated source element producing fake data buffers used for test purpose.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_FAKE_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_FAKE_SRC_H_

/**
 * @addtogroup mpipe_src
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_src.h>

/**
 * @brief Fake source element structure
 */
struct mpipe_fake_src {
	/** Base source element (must be first) */
	struct mpipe_src src;
	/** Buffer pool for managing output buffers */
	struct mpipe_buffer_pool pool;
};

/**
 * @brief Initialize a fake source element
 *
 * @param fsrc Pointer to the @ref mpipe_fake_src to initialize.
 * @param id   Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_fake_src_init(struct mpipe_fake_src *fsrc, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_FAKE_SRC_H_ */
