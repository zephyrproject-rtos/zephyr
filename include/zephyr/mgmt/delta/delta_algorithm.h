/**
 * @file delta_algorithm.h
 * @brief Delta algorithm backend API definition.
 *
 * Copyright (c) 2026 Clovis Corde
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ZEPHYR_MGMT_DELTA_DELTA_ALGORITHM_H_
#define ZEPHYR_INCLUDE_ZEPHYR_MGMT_DELTA_DELTA_ALGORITHM_H_

struct delta_ctx;

/**
 * @brief backend API to define the delta algorithm used for delta firmware update.
 */
struct delta_backend_api {
	/** Apply a delta patch to produce the target firmware */
	int (*patch)(struct delta_ctx *ctx);
	/** Validate the header of the delta patch */
	bool (*validate_header)(struct delta_ctx *ctx);
};

#endif /* ZEPHYR_INCLUDE_ZEPHYR_MGMT_DELTA_DELTA_ALGORITHM_H_ */
