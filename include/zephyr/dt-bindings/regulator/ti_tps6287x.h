/*
 * Copyright (c) 2026 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup regulator_ti_tps6287x
 * @brief Header file for Texas Instruments TPS6287x Step Down Converters.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_TI_TPS6287X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_TI_TPS6287X_H_

/**
 * @defgroup regulator_ti_tps6287x Texas Instruments TPS6287x Devicetree helpers
 * @brief Texas instruments TPS6287x regulator driver Devicetree helpers
 * @ingroup devicetree-regulator
 * @{
 */

/**
 * @name TI TPS6287x regulator modes
 * @{
 */
/** Power Saving Mode. */
#define TI_TPS6287X_MODE_PFM 0U
/** Forced PWM Mode. */
#define TI_TPS6287X_MODE_PWM 1U

/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_TI_TPS6287X_H_*/
