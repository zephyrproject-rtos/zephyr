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
 * @defgroup MpQuery
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
#define MP_QUERY_CREATE_TYPE(num, flags) (((num) << 8) | (flags))

/**
 * @enum MpQueryDirection
 * Direction flags for queries.
 */
typedef enum {
	MP_QUERY_DIRECTION_UNKNOWN = 0,            /**< Unknown direction */
	MP_QUERY_DIRECTION_UPSTREAM = BIT(0),      /**< Query flows upstream */
	MP_QUERY_DIRECTION_DOWNSTREAM = BIT(1),    /**< Query flows downstream */
	MP_QUERY_DIRECTION_BOTH = BIT(0) | BIT(1), /**< Query can flow in both directions */
} MpQueryDirection;

/**
 * @enum MpQueryType
 * Supported query types.
 */
typedef enum {
	MP_QUERY_UNKNOWN = MP_QUERY_CREATE_TYPE(0, 0), /**< Unknown query type */
	MP_QUERY_ALLOCATION =
		MP_QUERY_CREATE_TYPE(1, MP_QUERY_DIRECTION_BOTH),         /**< Allocation query */
	MP_QUERY_CAPS = MP_QUERY_CREATE_TYPE(2, MP_QUERY_DIRECTION_BOTH), /**< Capabilities query */
} MpQueryType;

/**
 * @struct MpQuery
 * Structure of query
 */
typedef struct {
	int type;              /**< Type of the query */
	MpStructure structure; /**< Associated field-value structure */
} MpQuery;

/**
 * Destroy a query and its associated resources.
 *
 * @param query Pointer to the @ref MpQuery to destroy
 */
void mp_query_destroy(MpQuery *query);

/**
 * @brief Create a new capabilities query
 *
 * Creates a query for negotiating media capabilities between elements.
 *
 * @param caps Pointer to @ref MpCaps to include in the query
 * @return Pointer to newly created @ref MpQuery, or NULL on allocation failure
 */
MpQuery *mp_query_new_caps(MpCaps *caps);

/**
 * @brief Set capabilities in a capabilities query
 *
 * Updates or sets the @ref MpCaps in a @ref MP_QUERY_CAPS type query.
 * If capabilities are already present, they will be replaced.
 *
 * @param query Pointer to a @ref MpQuery of type @ref MP_QUERY_CAPS
 * @param caps Pointer to @ref MpCaps to set in the query
 * @return true if successful, false if query is NULL or wrong type
 */
bool mp_query_set_caps(MpQuery *query, MpCaps *caps);

/**
 * @brief Get capabilities from a capabilities query
 *
 * Retrieves the @ref MpCaps from a @ref MP_QUERY_CAPS type query.
 *
 * @param query Pointer to the @ref MpQuery of type @ref MP_QUERY_CAPS
 * @return Pointer to @ref MpCaps if available, or NULL if not found or wrong type
 */
MpCaps *mp_query_get_caps(MpQuery *query);

/**
 * @brief Create a new allocation query
 *
 * Creates a query for negotiating buffer allocation parameters. This query
 * is used to establish buffer pools, memory requirements, and allocation
 * strategies between pipeline elements.
 *
 * @param caps Pointer to @ref MpCaps describing the media format for allocation
 * @return Pointer to newly created @ref MpQuery, or NULL on allocation failure
 */
MpQuery *mp_query_new_allocation(MpCaps *caps);

/**
 * @brief Set buffer pool in an allocation query
 *
 * Sets the buffer pool parameter in a @ref MP_QUERY_ALLOCATION type query.
 * This is used to propose a specific buffer pool for allocation.
 *
 * @param query Pointer to @ref MpQuery of type @ref MP_QUERY_ALLOCATION
 * @param pool Pointer to @ref MpBufferPool to set in the query
 * @return true if successful, false if query is NULL or wrong type
 */
bool mp_query_set_pool(MpQuery *query, MpBufferPool *pool);

/**
 * @brief Set buffer pool configuration in an allocation query
 *
 * Sets the buffer pool configuration parameter in a @ref MP_QUERY_ALLOCATION
 * type query. This is used to propose a specific buffer pool configuration
 * for allocation. If a pool is proposed, the configs should be set via the
 * config field of the proposed pool instead.
 *
 * @param query Pointer to @ref MpQuery of type @ref MP_QUERY_ALLOCATION
 * @param config Pointer to @ref MpBufferPoolConfig to set in the query
 * @return true if successful, false if query is NULL or wrong type
 */
bool mp_query_set_pool_config(MpQuery *query, MpBufferPoolConfig *config);

/**
 * @brief Get buffer pool from an allocation query
 *
 * Retrieves the buffer pool from a @ref MP_QUERY_ALLOCATION type query.
 *
 * @param query Pointer to @ref MpQuery of type @ref MP_QUERY_ALLOCATION
 * @return Pointer to @ref MpBufferPool if available, or NULL if not found or wrong type
 */
MpBufferPool *mp_query_get_pool(MpQuery *query);

/**
 * @brief Get buffer pool configuration from an allocation query
 *
 * Retrieves the buffer pool configuration from a @ref MP_QUERY_ALLOCATION
 * type query.
 *
 * @param query Pointer to @ref MpQuery of type @ref MP_QUERY_ALLOCATION
 * @return Pointer to @ref MpBufferPoolConfig if available, or NULL if not found or wrong type
 */
MpBufferPoolConfig *mp_query_get_pool_config(MpQuery *query);

/** @} */

#endif /* __MP_QUERY_H__ */
