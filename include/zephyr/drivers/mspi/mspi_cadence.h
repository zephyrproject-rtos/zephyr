/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MSPI_CADENCE_H_
#define ZEPHYR_INCLUDE_DRIVERS_MSPI_CADENCE_H_

/**
 * @file mspi_cadence.h
 * @brief Cadence MSPI controller specific definitions
 * @ingroup mspi_cadence_interface
 *
 */

#include <zephyr/sys/util_macro.h>
#include <stdint.h>

/**
 * @brief Configuration interfaces for Cadence MSPI controller
 * @defgroup mspi_cadence_interface Cadence MSPI interface
 * @ingroup mspi_configure_api
 * @{
 */

/**
 * @brief Timing configuration for the TI K3 MSPI peripheral.
 */
struct mspi_cadence_timing_cfg {
	/** Amount of read data capture cycles that are applied to internal data capture circuit */
	uint8_t rd_delay;
};

/**
 * @brief Enum which timing parameters should be modified
 */
enum mspi_cadence_timing_param {
	/** Read delay timing parameter */
	MSPI_CADENCE_TIMING_PARAM_RD_DELAY = BIT(0)
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MSPI_CADENCE_H_ */
