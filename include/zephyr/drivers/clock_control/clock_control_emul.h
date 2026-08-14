/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-compatible hooks of the emulated clock controller.
 *
 * Each supported compatible provides three functions, named after the
 * devicetree compatible token so that the driver can reach them with
 * @c UTIL_CAT():
 *
 * - @c <compat>_subsys_match()    tells whether a subsystem selector refers to
 *                                 a given entry of the @c clock-ids property,
 * - @c <compat>_rate_to_value()   converts a rate argument to a plain value,
 * - @c <compat>_cells_to_subsys() builds a subsystem selector out of the cells
 *                                 of one @c clock-ids entry.
 *
 * The last one lets a caller drive any variant from its devicetree data alone,
 * without knowing how many cells the selector has or what type it is.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_EMUL_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/renesas_cpg_mssr.h>
#include <zephyr/sys/util.h>

union clock_control_emul_subsys {
	clock_control_subsys_t subsys;
	struct rcar_cpg_clk rcar_cpg_clk;
};

/* Controllers whose selector is a plain clock identifier held in a single cell. */

#define CLOCK_CONTROL_SUBSYS_MATCH_1CELL(compat)                                                   \
	static inline bool compat##_subsys_match(clock_control_subsys_t sys,                       \
						 const uint32_t *cells, size_t num_cells)          \
	{                                                                                          \
		return num_cells == 1U && cells[0] == (uint32_t)(uintptr_t)sys;                    \
	}

#define CLOCK_CONTROL_RATE_TO_VALUE_1CELL(compat)                                                  \
	static inline int compat##_rate_to_value(clock_control_subsys_rate_t rate,                 \
						 uint32_t *value)                                  \
	{                                                                                          \
		*value = (uint32_t)(uintptr_t)rate;                                                \
		return 0;                                                                          \
	}

#define CLOCK_CONTROL_SUBSYS_1CELL(compat)                                                         \
	static inline clock_control_subsys_t compat##_cells_to_subsys(                             \
		const uint32_t *cells, union clock_control_emul_subsys *storage)                   \
	{                                                                                          \
		ARG_UNUSED(storage);                                                               \
                                                                                                   \
		return (clock_control_subsys_t)(uintptr_t)cells[0];                                \
	}

#define CLOCK_CONTROL_EMUL_DEFINE_1CELL(compat)                                                    \
	CLOCK_CONTROL_SUBSYS_MATCH_1CELL(compat)                                                   \
	CLOCK_CONTROL_RATE_TO_VALUE_1CELL(compat)                                                  \
	CLOCK_CONTROL_SUBSYS_1CELL(compat)

CLOCK_CONTROL_EMUL_DEFINE_1CELL(zephyr_clock_controller_emul_id)
CLOCK_CONTROL_EMUL_DEFINE_1CELL(zephyr_clock_controller_emul_clkid)
CLOCK_CONTROL_EMUL_DEFINE_1CELL(zephyr_clock_controller_emul_clk_id)

/* Renesas R-Car CPG/MSSR: the selector is a {domain, module} descriptor. */

static inline bool
zephyr_clock_controller_emul_rcar_cpg_mssr_subsys_match(clock_control_subsys_t sys,
							const uint32_t *cells, size_t num_cells)
{
	const struct rcar_cpg_clk *clk = sys;

	return clk != NULL && num_cells == 2U && cells[0] == clk->domain && cells[1] == clk->module;
}

static inline int
zephyr_clock_controller_emul_rcar_cpg_mssr_rate_to_value(clock_control_subsys_rate_t rate,
							 uint32_t *value)
{
	return zephyr_clock_controller_emul_id_rate_to_value(rate, value);
}

static inline clock_control_subsys_t
zephyr_clock_controller_emul_rcar_cpg_mssr_cells_to_subsys(const uint32_t *cells,
							   union clock_control_emul_subsys *storage)
{
	storage->rcar_cpg_clk.domain = cells[0];
	storage->rcar_cpg_clk.module = cells[1];

	return &storage->rcar_cpg_clk;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_EMUL_H_ */
