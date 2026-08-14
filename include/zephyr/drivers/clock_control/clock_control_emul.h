/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Common definitions for clock control emul
 *
 * Each supported compatible provides three functions, named after the
 * devicetree compatible token so that the driver can reach them with
 * @c UTIL_CAT():
 *
 * - @c [compat]_subsys_match()    tells whether a subsystem selector refers to
 *                                 a given entry of the @c clock-ids property,
 * - @c [compat]_rate_to_value()   converts a rate argument to a plain value,
 * - @c [compat]_cells_to_subsys() builds a subsystem selector out of the cells
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

/**
 * Storage for the different subsystem selector representations.
 *
 * Use the member matching the clock controller's subsystem representation.
 */
union clock_control_emul_subsys {
	/** Scalar clock subsystem selector encoded as an opaque pointer. */
	clock_control_subsys_t subsys;
	/** R-Car CPG/MSSR selector containing a domain and module identifier. */
	struct rcar_cpg_clk rcar_cpg_clk;
};

/* Controllers whose selector is a plain clock identifier held in a single cell. */

/**
 * Define a subsystem matcher for a one-cell clock selector.
 *
 * @param compat Token used to form the generated function name.
 */
#define CLOCK_CONTROL_SUBSYS_MATCH_1CELL(compat)                                                   \
	/**                                                                                        \
	 * Match a one-cell clock selector against a subsystem.                                    \
	 *                                                                                         \
	 * @param sys Subsystem selector encoded as a scalar clock identifier.                     \
	 * @param cells One-cell clock identifier from the devicetree clock-ids property.          \
	 * @param num_cells Number of cells in @p cells; must be one.                              \
	 * @return true if @p sys matches the clock identifier in @p cells, or false otherwise.    \
	 */                                                                                        \
	static inline bool compat##_subsys_match(clock_control_subsys_t sys,                       \
						 const uint32_t *cells, size_t num_cells)          \
	{                                                                                          \
		return num_cells == 1U && cells[0] == (uint32_t)(uintptr_t)sys;                    \
	}

/**
 * Define a rate converter for a one-cell clock selector.
 *
 * @param compat Token used to form the generated function name.
 */
#define CLOCK_CONTROL_RATE_TO_VALUE_1CELL(compat)                                                  \
	/**                                                                                        \
	 * Convert a subsystem rate to a one-cell value.                                           \
	 *                                                                                         \
	 * @param rate Scalar rate encoded in the opaque subsystem-rate pointer.                   \
	 * @param[out] value Converted rate value.                                                 \
	 * @retval 0 Conversion succeeds.                                                          \
	 */                                                                                        \
	static inline int compat##_rate_to_value(clock_control_subsys_rate_t rate,                 \
						 uint32_t *value)                                  \
	{                                                                                          \
		*value = (uint32_t)(uintptr_t)rate;                                                \
		return 0;                                                                          \
	}

/**
 * Define a subsystem converter for a one-cell clock selector.
 *
 * @param compat Token used to form the generated function name.
 */
#define CLOCK_CONTROL_SUBSYS_1CELL(compat)                                                         \
	/**                                                                                        \
	 * Convert one clock-id cell to a subsystem selector.                                      \
	 *                                                                                         \
	 * @param cells One-cell clock identifier from the devicetree clock-ids property.          \
	 * @param storage Storage for a structured selector; unused by this scalar variant.        \
	 * @return The scalar subsystem selector encoded by @p cells[0].                           \
	 */                                                                                        \
	static inline clock_control_subsys_t compat##_cells_to_subsys(                             \
		const uint32_t *cells, union clock_control_emul_subsys *storage)                   \
	{                                                                                          \
		ARG_UNUSED(storage);                                                               \
                                                                                                   \
		return (clock_control_subsys_t)(uintptr_t)cells[0];                                \
	}

/**
 * Define all hooks for a one-cell clock controller compatible.
 *
 * @param compat Token naming the devicetree compatible and generated hook functions.
 */
#define CLOCK_CONTROL_EMUL_DEFINE_1CELL(compat)                                                    \
	CLOCK_CONTROL_SUBSYS_MATCH_1CELL(compat)                                                   \
	CLOCK_CONTROL_RATE_TO_VALUE_1CELL(compat)                                                  \
	CLOCK_CONTROL_SUBSYS_1CELL(compat)

CLOCK_CONTROL_EMUL_DEFINE_1CELL(zephyr_clock_controller_emul_id)
CLOCK_CONTROL_EMUL_DEFINE_1CELL(zephyr_clock_controller_emul_clkid)
CLOCK_CONTROL_EMUL_DEFINE_1CELL(zephyr_clock_controller_emul_clk_id)

/* Renesas R-Car CPG/MSSR: the selector is a {domain, module} descriptor. */

/**
 * Match an R-Car CPG/MSSR selector against devicetree cells.
 *
 * @param sys R-Car selector pointing to a @ref rcar_cpg_clk descriptor.
 * @param cells Two-cell clock identifier in domain, module order.
 * @param num_cells Number of cells in @p cells; must be two.
 * @return true if the selector's domain and module match @p cells, or false otherwise.
 */
static inline bool
zephyr_clock_controller_emul_rcar_cpg_mssr_subsys_match(clock_control_subsys_t sys,
							const uint32_t *cells, size_t num_cells)
{
	const struct rcar_cpg_clk *clk = sys;

	return clk != NULL && num_cells == 2U && cells[0] == clk->domain && cells[1] == clk->module;
}

/**
 * Convert an R-Car CPG/MSSR rate to a stored value.
 *
 * @param rate Scalar rate encoded in the opaque subsystem-rate pointer.
 * @param[out] value Converted rate value.
 * @retval 0 Conversion succeeds.
 */
static inline int
zephyr_clock_controller_emul_rcar_cpg_mssr_rate_to_value(clock_control_subsys_rate_t rate,
							 uint32_t *value)
{
	return zephyr_clock_controller_emul_id_rate_to_value(rate, value);
}

/**
 * Build an R-Car CPG/MSSR selector from devicetree cells.
 *
 * @param cells Two-cell clock identifier in domain, module order.
 * @param[out] storage Storage that receives the selector descriptor. Keep it valid while the
 *                    returned selector is in use.
 * @return A pointer to the R-Car selector stored in @p storage.
 */
static inline clock_control_subsys_t
zephyr_clock_controller_emul_rcar_cpg_mssr_cells_to_subsys(const uint32_t *cells,
							   union clock_control_emul_subsys *storage)
{
	storage->rcar_cpg_clk.domain = cells[0];
	storage->rcar_cpg_clk.module = cells[1];

	return &storage->rcar_cpg_clk;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_EMUL_H_ */
