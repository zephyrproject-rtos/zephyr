/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for the Renesas R-Car CPG/MSSR controller.
 * @ingroup clock_control_renesas_cpg_mssr
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RCAR_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RCAR_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/renesas_cpg_mssr.h>

/**
 * @defgroup clock_control_renesas_cpg_mssr Renesas R-Car CPG/MSSR
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief R-Car CPG/MSSR clock subsystem selector. */
struct rcar_cpg_clk {
	uint32_t domain; /**< Clock domain. */
	uint32_t module; /**< Module (MSSR) identifier within the domain. */
	uint32_t rate;   /**< Requested clock rate, in Hz. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RCAR_CLOCK_CONTROL_H_ */
