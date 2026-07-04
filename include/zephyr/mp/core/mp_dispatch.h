/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Dispatch for event and query handler
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_DISPATCH_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_DISPATCH_H_

/**
 * @defgroup mp_dispatch Dispatches
 * @ingroup mp_core
 * @brief Dispatch objects exchanged between elements.
 * @{
 */

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_object.h>

/**
 * @enum mp_dispatch_type
 * @brief Supported dispatch types.
 */
enum mp_dispatch_type {
	MP_DISPATCH_UNKNOWN = 0,     /**< Unknown dispatch type */
	MP_DISPATCH_EOS,             /**< EOS dispatch type */
	MP_DISPATCH_CAPS,            /**< CAPS dispatch type */
	MP_DISPATCH_BUFFER_CONFIG,   /**< Buffer config type */
	MP_DISPATCH_END = UINT8_MAX, /**< Maximum dispatch type identifier */
};

/**
 * @struct mp_dispatch
 * @brief Structure representing a dispatch (event or query).
 */
struct mp_dispatch {
	uint8_t type;                          /**< Dispatch type from @ref mp_dispatch_type */
	struct mp_caps *caps;                  /**< Pointer to caps */
	struct mp_buffer_pool *pool;           /**< Buffer pool (not owned) */
	struct mp_buffer_pool_config pool_cfg; /**< Pool config */
};

/**
 * @brief Initialize a dispatch of any type.
 *
 * @param dispatch Pointer to a @ref mp_dispatch to initialize.
 * @param type     Dispatch type from @ref mp_dispatch_type.
 * @param caps     Caps to attach (will be ref'd), or NULL.
 */
void mp_dispatch_init(struct mp_dispatch *dispatch, uint8_t type, struct mp_caps *caps);

/**
 * @brief Initialize a dispatch with end-of-stream type.
 *
 * @param d Pointer to the @ref mp_dispatch structure to initialize.
 */
#define mp_dispatch_eos_init(d) mp_dispatch_init(d, MP_DISPATCH_EOS, NULL)

/**
 * @brief Initialize a dispatch with caps type.
 *
 * @param d Pointer to the @ref mp_dispatch structure to initialize.
 * @param c Pointer to the @ref mp_caps to attach, or NULL.
 */
#define mp_dispatch_caps_init(d, c) mp_dispatch_init(d, MP_DISPATCH_CAPS, c)

/**
 * @brief Initialize a dispatch with buffer-configuration type.
 *
 * @param d Pointer to the @ref mp_dispatch structure to initialize.
 * @param c Pointer to the @ref mp_caps to attach, or NULL.
 */
#define mp_dispatch_buffer_config_init(d, c) mp_dispatch_init(d, MP_DISPATCH_BUFFER_CONFIG, c)

/**
 * @brief Clear a dispatch, releasing any owned resources.
 *
 * @param dispatch Pointer to a @ref mp_dispatch to clear.
 */
void mp_dispatch_clear(struct mp_dispatch *dispatch);

/**
 * @brief Get the caps from a dispatch (borrowed reference).
 *
 * @param dispatch Pointer to a @ref mp_dispatch.
 *
 * @return Pointer to @ref mp_caps, or NULL.
 */
struct mp_caps *mp_dispatch_get_caps(struct mp_dispatch *dispatch);

/**
 * @brief Set (replace) the caps on a dispatch.
 *
 * @param dispatch Pointer to a @ref mp_dispatch.
 * @param caps     Pointer to @ref mp_caps, or NULL.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p dispatch is NULL or type does not carry caps.
 */
int mp_dispatch_set_caps(struct mp_dispatch *dispatch, struct mp_caps *caps);

/**
 * @brief Set the buffer pool on an allocation dispatch.
 *
 * @param dispatch Pointer to a @ref MP_DISPATCH_BUFFER_CONFIG dispatch.
 * @param pool     Pointer to @ref mp_buffer_pool.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p dispatch is NULL or not an allocation dispatch.
 */
int mp_dispatch_set_pool(struct mp_dispatch *dispatch, struct mp_buffer_pool *pool);

/**
 * @brief Set the pool configuration on an allocation dispatch.
 *
 * @param dispatch Pointer to a @ref MP_DISPATCH_BUFFER_CONFIG dispatch.
 * @param config   Pointer to @ref mp_buffer_pool_config.
 *
 * @retval 0       Success.
 * @retval -EINVAL @p dispatch is NULL or not an allocation dispatch.
 */
int mp_dispatch_set_pool_config(struct mp_dispatch *dispatch, struct mp_buffer_pool_config *config);

/**
 * @brief Get the buffer pool from an allocation dispatch.
 *
 * @param dispatch Pointer to a @ref MP_DISPATCH_BUFFER_CONFIG dispatch.
 *
 * @return Pointer to @ref mp_buffer_pool, or NULL.
 */
struct mp_buffer_pool *mp_dispatch_get_pool(struct mp_dispatch *dispatch);

/**
 * @brief Get the pool configuration from an allocation dispatch.
 *
 * @param dispatch Pointer to a @ref MP_DISPATCH_BUFFER_CONFIG dispatch.
 *
 * @return Pointer to @ref mp_buffer_pool_config, or NULL.
 */
struct mp_buffer_pool_config *mp_dispatch_get_pool_config(struct mp_dispatch *dispatch);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_DISPATCH_H_ */
