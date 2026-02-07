/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Query header file.
 */

#ifndef __MP_QUERY_H__
#define __MP_QUERY_H__

#include "mp_buffer.h"
#include "mp_caps.h"

/**
 * @defgroup mp_query
 * @brief Pipeline query
 *
 * @{
 */

/**
 * Create an query type from an id number and flags.
 *
 * @param num	Query id number (A new type of query must have unique id number)
 * @param flags Query flags
 */
#define MP_QUERY_CREATE_TYPE(num, flags) (((num) << 2) | (flags))

/**
 * @enum mp_query_direction
 * Direction flags for queries.
 */
enum mp_query_direction {
	MP_QUERY_DIRECTION_UNKNOWN = 0,            /**< Unknown direction */
	MP_QUERY_DIRECTION_UPSTREAM = BIT(0),      /**< Query flows upstream */
	MP_QUERY_DIRECTION_DOWNSTREAM = BIT(1),    /**< Query flows downstream */
	MP_QUERY_DIRECTION_BOTH = BIT(0) | BIT(1), /**< Query can flow in both directions */
};

/**
 * @enum mp_query_type
 * Supported query types.
 */
enum mp_query_type {
	MP_QUERY_UNKNOWN = MP_QUERY_CREATE_TYPE(0, 0), /**< Unknown query type */
	MP_QUERY_ALLOCATION =
		MP_QUERY_CREATE_TYPE(1, MP_QUERY_DIRECTION_BOTH),         /**< Allocation query */
	MP_QUERY_CAPS = MP_QUERY_CREATE_TYPE(2, MP_QUERY_DIRECTION_BOTH), /**< Capabilities query */

	MP_QUERY_END = UINT8_MAX, /**< Maximum query type identifer */
};

/**
 * @struct mp_query
 * Structure of query
 */
struct mp_query {
	uint8_t type;                  /**< Type of the query */
	struct mp_structure structure; /**< Associated field-value structure */
};

/**
 * Destroy a query and its associated resources.
 *
 * @param query Pointer to the @ref struct mp_query to destroy
 */
void mp_query_destroy(struct mp_query *query);

/**
 * @brief Create a new capabilities query
 *
 * Creates a query for negotiating media capabilities between elements.
 *
 * @param caps Pointer to @ref struct mp_caps to include in the query
 * @return Pointer to newly created @ref struct mp_query, or NULL on allocation failure
 */
struct mp_query *mp_query_new_caps(struct mp_caps *caps);

/**
 * @brief Set capabilities in a capabilities query
 *
 * Updates or sets the @ref struct mp_caps in a @ref MP_QUERY_CAPS type query.
 * If capabilities are already present, they will be replaced.
 *
 * @param query Pointer to a @ref struct mp_query of type @ref MP_QUERY_CAPS
 * @param caps Pointer to @ref struct mp_caps to set in the query
 * @return true if successful, false if query is NULL or wrong type
 */
bool mp_query_set_caps(struct mp_query *query, struct mp_caps *caps);

/**
 * @brief Get capabilities from a capabilities query
 *
 * Retrieves the @ref struct mp_caps from a @ref MP_QUERY_CAPS type query.
 *
 * @param query Pointer to the @ref struct mp_query of type @ref MP_QUERY_CAPS
 * @return Pointer to @ref struct mp_caps if available, or NULL if not found or wrong type
 */
struct mp_caps *mp_query_get_caps(struct mp_query *query);

/**
 * @brief Create a new allocation query
 *
 * Creates a query for negotiating buffer allocation parameters. This query
 * is used to establish buffer pools, memory requirements, and allocation
 * strategies between pipeline elements.
 *
 * @param caps Pointer to @ref struct mp_caps describing the media format for allocation
 * @return Pointer to newly created @ref struct mp_query, or NULL on allocation failure
 */
struct mp_query *mp_query_new_allocation(struct mp_caps *caps);

/**
 * @brief Set buffer pool in an allocation query
 *
 * Sets the buffer pool parameter in a @ref MP_QUERY_ALLOCATION type query.
 * This is used to propose a specific buffer pool for allocation.
 *
 * @param query Pointer to @ref struct mp_query of type @ref MP_QUERY_ALLOCATION
 * @param pool Pointer to @ref struct mp_buffer_pool to set in the query
 * @return true if successful, false if query is NULL or wrong type
 */
bool mp_query_set_pool(struct mp_query *query, struct mp_buffer_pool *pool);

/**
 * @brief Set buffer pool configuration in an allocation query
 *
 * Sets the buffer pool configuration parameter in a @ref MP_QUERY_ALLOCATION
 * type query. This is used to propose a specific buffer pool configuration
 * for allocation. If a pool is proposed, the configs should be set via the
 * config field of the proposed pool instead.
 *
 * @param query Pointer to @ref struct mp_query of type @ref MP_QUERY_ALLOCATION
 * @param config Pointer to @ref struct mp_buffer_pool_config to set in the query
 * @return true if successful, false if query is NULL or wrong type
 */
bool mp_query_set_pool_config(struct mp_query *query, struct mp_buffer_pool_config *config);

/**
 * @brief Get buffer pool from an allocation query
 *
 * Retrieves the buffer pool from a @ref MP_QUERY_ALLOCATION type query.
 *
 * @param query Pointer to @ref struct mp_query of type @ref MP_QUERY_ALLOCATION
 * @return Pointer to @ref struct mp_buffer_pool if available, or NULL if not found or wrong type
 */
struct mp_buffer_pool *mp_query_get_pool(struct mp_query *query);

/**
 * @brief Get buffer pool configuration from an allocation query
 *
 * Retrieves the buffer pool configuration from a @ref MP_QUERY_ALLOCATION
 * type query.
 *
 * @param query Pointer to @ref struct mp_query of type @ref MP_QUERY_ALLOCATION
 * @return Pointer to @ref struct mp_buffer_pool_config if available, or NULL if not found or wrong
 * type
 */
struct mp_buffer_pool_config *mp_query_get_pool_config(struct mp_query *query);

/** @} */

#endif /* __MP_QUERY_H__ */
