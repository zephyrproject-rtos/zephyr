/**
 * @file
 * @brief NXP MCUX clock_control devicetree subsystem helpers.
 */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_MCUX_CLOCK_SUBSYS_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_MCUX_CLOCK_SUBSYS_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <fsl_clock.h>

/**
 * @brief Helpers for MCUX clock_control devicetree consumers.
 *
 * Some NXP SoCs use the MCUX SIM as a clock controller with @c \#clock-cells = 3:
 *
 * @code
 * <name offset bits>
 * @endcode
 *
 * Some NXP SoCs use the SCG K4 clock controller with @c \#clock-cells = 2:
 *
 * @code
 * <name mrcc_offset>
 * @endcode
 *
 * - Enabling/disabling a peripheral clock may require a provider-specific
 *   gate token derived from (offset, bits) via CLK_GATE_DEFINE() (SIM) or
 *   MRCC register address encoding from `mrcc_offset` (SCG K4).
 * - Retrieving a clock rate may require a provider-specific selector such as
 *   the `name` cell, suitable for CLOCK_GetFreq().
 *
 * Some controllers (notably Kinetis SIM) require different devicetree cells
 * for gate control vs rate queries. Use the `*_GATE_*` macros for
 * `clock_control_on/off()` and the `*_RATE_*` macros for
 * `clock_control_get_rate()`. Drivers may therefore store both a gate subsys
 * token and a rate subsys token.
 *
 * In the common "register clock" / "function clock" model:
 *
 * - The register clock is the clock used to access the peripheral registers
 *   and is typically controlled via `clock_control_on/off()`.
 * - The function clock is the clock that drives the peripheral function and
 *   is typically queried via `clock_control_get_rate()`.
 *
 * These helpers map each API usage to the controller-specific
 * `clock_control_subsys_t` token expected by the underlying clock controller
 * implementation. Some controllers use the same token for both concepts,
 * while others (e.g. SIM) use different devicetree cells/tokens for register
 * clock gating vs function clock rate.
 */

/**
 * @brief True if the clock specifier has a `name` cell.
 *
 * Some clock controllers (e.g. fixed-clock) use @c \#clock-cells = 0 and thus
 * do not provide a subsystem selector cell.
 */
#define NXP_MCUX_DT_CLOCK_HAS_NAME_CELL_BY_IDX(node_id, idx) \
	DT_PHA_HAS_CELL_AT_IDX(node_id, clocks, idx, name)

/**
 * @brief True if the instance clock specifier has a `name` cell.
 */
#define NXP_MCUX_DT_INST_CLOCK_HAS_NAME_CELL_BY_IDX(inst, idx) \
	DT_PHA_HAS_CELL_AT_IDX(DT_DRV_INST(inst), clocks, idx, name)

/**
 * @brief True if the clock controller for a DT node is a Kinetis SIM.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Clock specifier index within the node's `clocks` property.
 *
 * @return 1 if the clock controller is compatible with a Kinetis SIM, else 0.
 */
#define NXP_MCUX_DT_CLOCK_CTLR_IS_SIM_BY_IDX(node_id, idx) \
	UTIL_OR(DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR_BY_IDX(node_id, idx), nxp_kinetis_sim), \
		DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR_BY_IDX(node_id, idx), nxp_kinetis_ke1xf_sim))

/**
 * @brief True if the clock controller for a DT node is SCG K4.
 *
 * @param node_id Devicetree node identifier.
 * @param idx Clock specifier index within the node's `clocks` property.
 *
 * @return 1 if the clock controller is compatible with SCG K4, else 0.
 */
#define NXP_MCUX_DT_CLOCK_CTLR_IS_SCG_K4_BY_IDX(node_id, idx) \
	DT_NODE_HAS_COMPAT(DT_CLOCKS_CTLR_BY_IDX(node_id, idx), nxp_scg_k4)

/**
 * @brief True if the clock controller for a devicetree instance is a Kinetis SIM.
 *
 * @param inst Devicetree instance number.
 * @param idx Clock specifier index within the instance's `clocks` property.
 *
 * @return 1 if the clock controller is compatible with a Kinetis SIM, else 0.
 */
#define NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SIM_BY_IDX(inst, idx) \
	UTIL_OR(DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR_BY_IDX(inst, idx), nxp_kinetis_sim), \
		DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR_BY_IDX(inst, idx), nxp_kinetis_ke1xf_sim))

/**
 * @brief True if the instance clock controller is SCG K4.
 *
 * @param inst Devicetree instance number.
 * @param idx Clock specifier index within the instance's `clocks` property.
 *
 * @return 1 if the clock controller is compatible with SCG K4, else 0.
 */
#define NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SCG_K4_BY_IDX(inst, idx) \
	DT_NODE_HAS_COMPAT(DT_INST_CLOCKS_CTLR_BY_IDX(inst, idx), nxp_scg_k4)

/**
 * @brief True if the instance clock controller is a Kinetis SIM.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SIM_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return 1 if the clock controller is compatible with a Kinetis SIM, else 0.
 */
#define NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SIM(inst) \
	NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SIM_BY_IDX(inst, 0)

/**
 * @brief Clock subsys token for enabling/disabling the node clock (by index).
 */
#define NXP_MCUX_DT_CLOCK_GATE_SUBSYS_BY_IDX(node_id, idx) \
	COND_CODE_1(NXP_MCUX_DT_CLOCK_CTLR_IS_SIM_BY_IDX(node_id, idx), \
		(CLK_GATE_DEFINE(DT_CLOCKS_CELL_BY_IDX(node_id, idx, offset), \
				DT_CLOCKS_CELL_BY_IDX(node_id, idx, bits))), \
		(COND_CODE_1(NXP_MCUX_DT_CLOCK_CTLR_IS_SCG_K4_BY_IDX(node_id, idx), \
			((DT_CLOCKS_CELL_BY_IDX(node_id, idx, mrcc_offset) == 0) \
				? 0U \
				: MAKE_MRCC_REGADDR(MRCC_BASE, \
					DT_CLOCKS_CELL_BY_IDX(node_id, idx, mrcc_offset))), \
			(COND_CODE_1(NXP_MCUX_DT_CLOCK_HAS_NAME_CELL_BY_IDX(node_id, idx), \
				(DT_CLOCKS_CELL_BY_IDX(node_id, idx, name)), \
				(0U))))))

/**
 * @brief Clock subsys token for enabling/disabling the node clock.
 *
 * Convenience form of NXP_MCUX_DT_CLOCK_GATE_SUBSYS_BY_IDX() for index 0.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return A token usable as `clock_control_subsys_t` for `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_CLOCK_GATE_SUBSYS(node_id) \
	NXP_MCUX_DT_CLOCK_GATE_SUBSYS_BY_IDX(node_id, 0)

/**
 * @brief Pointer-form clock subsys token for enabling/disabling
 * the node clock.
 *
 * Convenience form of NXP_MCUX_DT_CLOCK_GATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_CLOCK_GATE_SUBSYS_PTR_BY_IDX(node_id, idx) \
	UINT_TO_POINTER(NXP_MCUX_DT_CLOCK_GATE_SUBSYS_BY_IDX(node_id, idx))

/**
 * @brief Pointer-form clock subsys token for enabling/disabling
 * the node clock.
 *
 * Convenience form of NXP_MCUX_DT_CLOCK_GATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_CLOCK_GATE_SUBSYS_PTR(node_id) \
	NXP_MCUX_DT_CLOCK_GATE_SUBSYS_PTR_BY_IDX(node_id, 0)

/**
 * @brief Clock subsys token for enabling/disabling the instance clock.
 *
 * If the instance clock controller is a Kinetis SIM, returns a packed gate
 * token derived from the `offset` and `bits` clock cells. If the instance
 * clock controller is SCG K4, returns an MRCC register address token derived
 * from `mrcc_offset` (`0` means no gate). Otherwise, returns the
 * controller-specific `name` clock cell.
 *
 * @param inst Devicetree instance number.
 * @param idx Clock specifier index within the instance's `clocks` property.
 *
 * @return A token usable as `clock_control_subsys_t` for `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_BY_IDX(inst, idx) \
	COND_CODE_1(NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SIM_BY_IDX(inst, idx), \
		(CLK_GATE_DEFINE(DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, offset), \
				DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, bits))), \
		(COND_CODE_1(NXP_MCUX_DT_INST_CLOCK_CTLR_IS_SCG_K4_BY_IDX(inst, idx), \
			((DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, mrcc_offset) == 0) \
				? 0U \
				: MAKE_MRCC_REGADDR(MRCC_BASE, \
					DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, mrcc_offset))), \
			(COND_CODE_1(NXP_MCUX_DT_INST_CLOCK_HAS_NAME_CELL_BY_IDX(inst, idx), \
				(DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, name)), \
				(0U))))))

/**
 * @brief Clock subsys token for enabling/disabling the instance clock.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return A token usable as `clock_control_subsys_t` for `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS(inst) \
	NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_BY_IDX(inst, 0)

/**
 * @brief Pointer-form clock subsys token for enabling/disabling
 * the instance clock.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_PTR_BY_IDX(inst, idx) \
	UINT_TO_POINTER(NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_BY_IDX(inst, idx))

/**
 * @brief Pointer-form clock subsys token for enabling/disabling
 * the instance clock.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_on/off()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_PTR(inst) \
	NXP_MCUX_DT_INST_CLOCK_GATE_SUBSYS_PTR_BY_IDX(inst, 0)

/**
 * @brief Clock subsys token for retrieving the node clock rate.
 */
#define NXP_MCUX_DT_CLOCK_RATE_SUBSYS_BY_IDX(node_id, idx) \
	COND_CODE_1(NXP_MCUX_DT_CLOCK_HAS_NAME_CELL_BY_IDX(node_id, idx), \
		(DT_CLOCKS_CELL_BY_IDX(node_id, idx, name)), (0U))

/**
 * @brief Clock subsys token for retrieving the node clock rate.
 *
 * Convenience form of NXP_MCUX_DT_CLOCK_RATE_SUBSYS_BY_IDX() for index 0.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return A token usable as `clock_control_subsys_t` for `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_CLOCK_RATE_SUBSYS(node_id) \
	NXP_MCUX_DT_CLOCK_RATE_SUBSYS_BY_IDX(node_id, 0)

/**
 * @brief Pointer-form clock subsys token for retrieving
 * the node clock rate.
 *
 * Convenience form of NXP_MCUX_DT_CLOCK_RATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_CLOCK_RATE_SUBSYS_PTR_BY_IDX(node_id, idx) \
	UINT_TO_POINTER(NXP_MCUX_DT_CLOCK_RATE_SUBSYS_BY_IDX(node_id, idx))

/**
 * @brief Pointer-form clock subsys token for retrieving
 * the node clock rate.
 *
 * Convenience form of NXP_MCUX_DT_CLOCK_RATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param node_id Devicetree node identifier.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_CLOCK_RATE_SUBSYS_PTR(node_id) \
	NXP_MCUX_DT_CLOCK_RATE_SUBSYS_PTR_BY_IDX(node_id, 0)

/**
 * @brief Clock subsys token for retrieving the instance clock rate.
 *
 * @param inst Devicetree instance number.
 * @param idx Clock specifier index within the instance's `clocks` property.
 *
 * @return A token usable as `clock_control_subsys_t` for `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_BY_IDX(inst, idx) \
	COND_CODE_1(NXP_MCUX_DT_INST_CLOCK_HAS_NAME_CELL_BY_IDX(inst, idx), \
		(DT_INST_CLOCKS_CELL_BY_IDX(inst, idx, name)), (0U))

/**
 * @brief Clock subsys token for retrieving the instance clock rate.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return A token usable as `clock_control_subsys_t` for `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS(inst) \
	NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_BY_IDX(inst, 0)

/**
 * @brief Pointer-form clock subsys token for retrieving
 * the instance clock rate.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_PTR_BY_IDX(inst, idx) \
	UINT_TO_POINTER(NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_BY_IDX(inst, idx))

/**
 * @brief Pointer-form clock subsys token for retrieving
 * the instance clock rate.
 *
 * Convenience form of NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_PTR_BY_IDX() for index 0.
 *
 * @param inst Devicetree instance number.
 *
 * @return A pointer-form token usable as `clock_control_subsys_t` for
 * `clock_control_get_rate()`.
 */
#define NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_PTR(inst) \
	NXP_MCUX_DT_INST_CLOCK_RATE_SUBSYS_PTR_BY_IDX(inst, 0)

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NXP_MCUX_CLOCK_SUBSYS_H_ */
