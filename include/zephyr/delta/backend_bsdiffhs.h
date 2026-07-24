/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DELTA_BACKEND_BSDIFFHS_H_
#define ZEPHYR_INCLUDE_DELTA_BACKEND_BSDIFFHS_H_

#include <zephyr/delta/delta.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief bsdiff + heatshrink delta backend.
 * @defgroup delta_backend_bsdiffhs bsdiff+heatshrink backend
 * @ingroup delta_api
 * @{
 */

/**
 * bsdiff + heatshrink delta backend.
 */
extern const struct delta_backend delta_backend_bsdiffhs;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DELTA_BACKEND_BSDIFFHS_H_ */
