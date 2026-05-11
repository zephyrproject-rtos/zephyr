/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas R-Car generic access to clocks
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RCAR_GENERIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RCAR_GENERIC_H_

/* Gen 5 boards deal with clocks through SCMI. */
#ifdef CONFIG_CLOCK_CONTROL_ARM_SCMI

#include <zephyr/drivers/clock_control.h>

/**
 * @brief Helper similar to DT_INST_CLOCKS_CELL_BY_IDX.
 *
 * Automatically load the correct clock settings according to the target platform.
 * The usage is identical to @ref DT_INST_CLOCKS_CELL_BY_IDX.
 */
#define RCAR_DT_INST_CLOCKS_CELL_BY_IDX(inst, idx)					\
{											\
	.subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, name)	\
}

/**
 * @brief Get access to a specific clock.
 *
 * The way to retrieve the clock data depends on the platform.
 */
#define RCAR_CLOCK_SUBSYS(clk) (clk).subsys

/** Wrap the pointer value into a struct to make the helpers accessing it more robust. */
struct rcar_generic_clk {
	clock_control_subsys_t subsys;
};

/** Abstract the clock type used by the implementation. */
typedef struct rcar_generic_clk rcar_generic_clk_t;

/* Gen 3 and gen 4 boards directly access the clock controller module. */
#else

#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>

/**
 * @brief Helper similar to DT_INST_CLOCKS_CELL_BY_IDX.
 *
 * Automatically load the correct clock settings according to the target platform.
 * The usage is identical to @ref DT_INST_CLOCKS_CELL_BY_IDX.
 */
#define RCAR_DT_INST_CLOCKS_CELL_BY_IDX(inst, idx)			\
{									\
	.module = DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, module),	\
	.domain = DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, domain)		\
}

/**
 * @brief Get access to a specific clock.
 *
 * The way to retrieve the clock data depends on the platform.
 */
#define RCAR_CLOCK_SUBSYS(clk) (clock_control_subsys_t)&(clk)

/** Abstract the clock type used by the implementation. */
typedef struct rcar_cpg_clk rcar_generic_clk_t;

#endif /* CONFIG_CLOCK_CONTROL_ARM_SCMI */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RCAR_GENERIC_H_ */
