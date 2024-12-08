/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NRF5X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NRF5X_H_

/**
 * @defgroup regulator_nrf5x nRF5X regulator devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name nRF5X regulator modes
 * @{
 */
/** LDO mode */
#define NRF5X_REG_MODE_LDO 0
/** DC/DC mode */
#define NRF5X_REG_MODE_DCDC 1
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_NRF5X_H_*/
