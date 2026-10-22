/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_REGULATOR_PCA9420_H_
#define ZEPHYR_INCLUDE_DRIVERS_REGULATOR_PCA9420_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup regulator_parent_pca9420 PCA9420 Utilities.
 * @ingroup regulator_parent_interface
 * @{
 */

/** @brief DVS state 'MODE0' */
#define REGULATOR_PCA9420_DVS_MODE0 0
/** @brief DVS state 'MODE1' */
#define REGULATOR_PCA9420_DVS_MODE1 1
/** @brief DVS state 'MODE2' */
#define REGULATOR_PCA9420_DVS_MODE2 2
/** @brief DVS state 'MODE3' */
#define REGULATOR_PCA9420_DVS_MODE3 3

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_REGULATOR_PCA9420_H_ */
