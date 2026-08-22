/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Affine mapping between precision time domains.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_MAPPING_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_MAPPING_H_

#include <stdbool.h>

#include <zephyr/precision_timing/precision_time.h>
#include <zephyr/sys/timeutil.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Time Mapping
 * @defgroup precision_mapping Precision Time Mapping
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Affine mapping between a source domain and a local domain. */
struct precision_time_mapping {
	/** Domain used as the mapping reference. */
	struct precision_time_domain source_domain;
	/** Domain used as the mapping local clock. */
	struct precision_time_domain local_domain;
	/** Underlying affine synchronization state. */
	struct timeutil_sync_state state;
	/** Bias used to represent signed source values in @ref timeutil_sync_state. */
	precision_time_t source_bias;
	/** Bias used to represent signed local values in @ref timeutil_sync_state. */
	precision_time_t local_bias;
	/** Whether the mapping has at least one valid observation. */
	bool valid;
};

/**
 * @brief Initialize an empty precision time mapping.
 *
 * A null @p mapping is ignored.
 *
 * @param mapping Mapping to initialize.
 * @param source_domain Source domain accepted by the mapping.
 * @param local_domain Local domain produced by the mapping.
 */
void precision_time_mapping_init(struct precision_time_mapping *mapping,
				 struct precision_time_domain source_domain,
				 struct precision_time_domain local_domain);

/**
 * @brief Invalidate a precision time mapping while preserving its domains.
 *
 * A null @p mapping is ignored.
 *
 * @param mapping Mapping to invalidate.
 */
void precision_time_mapping_invalidate(struct precision_time_mapping *mapping);

/**
 * @brief Update a precision time mapping from a simultaneous observation.
 *
 * @param mapping Mapping to update.
 * @param observation Valid source and local observation.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument, validity flag, domain, or update is invalid.
 * @retval -ERANGE if an observation cannot be represented by the mapping.
 */
int precision_time_mapping_update(struct precision_time_mapping *mapping,
				  const struct precision_time_observation *observation);

/**
 * @brief Convert a source-domain time point to the local domain.
 *
 * @param mapping Valid mapping to use.
 * @param source Source-domain time point.
 * @param local Destination for the converted local-domain time point.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or source domain is invalid.
 * @retval -EAGAIN if the mapping is not valid yet.
 * @retval -ERANGE if the converted value is not representable.
 */
int precision_time_mapping_source_to_local(const struct precision_time_mapping *mapping,
					   const struct precision_time_point *source,
					   struct precision_time_point *local);

/**
 * @brief Convert a local-domain time point to the source domain.
 *
 * @param mapping Valid mapping to use.
 * @param local Local-domain time point.
 * @param source Destination for the converted source-domain time point.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument or local domain is invalid.
 * @retval -EAGAIN if the mapping is not valid yet.
 * @retval -ERANGE if the converted value is not representable.
 */
int precision_time_mapping_local_to_source(const struct precision_time_mapping *mapping,
					   const struct precision_time_point *local,
					   struct precision_time_point *source);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_MAPPING_H_ */
