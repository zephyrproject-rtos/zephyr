/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SF32LB52X_LDO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SF32LB52X_LDO_H_

/**
 * @defgroup regulator_sf32lb52x_ldo SF32LB52x peripheral LDO devicetree helpers.
 * @ingroup regulator_interface
 * @{
 */

/**
 * @name SF32LB52x peripheral LDO indices
 * @{
 */
/** LDO18: 1.8V for PSRAM (VDDSIP). */
#define SF32LB52X_LDO18 0
/** LDO2_3V3: 3.3V for peripherals. */
#define SF32LB52X_LDO2_3V3 1
/** LDO3_3V3: 3.3V for peripherals. */
#define SF32LB52X_LDO3_3V3 2
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_SF32LB52X_LDO_H_ */
