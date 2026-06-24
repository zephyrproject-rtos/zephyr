/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * APIs available to clock drivers when implementing support for the
 * Clock Driver interface. Clock drivers should only call these APIs directly
 * in cases where their implementation requires, it, typically the
 * clock management subsystem will call them automatically
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_HELPERS_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_HELPERS_H_

#include <zephyr/drivers/clock_management/clock.h>

/**
 * @brief Clock Driver Interface
 * @defgroup clock_driver_helpers Clock Driver Helpers
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks the children of a clock to validate they can support a given rate
 *
 * This function will validate that children of the provided clock can support
 * the new rate proposed. Some clock implementations may need to call this if
 * they will reconfigure into intermediate states in the process of changing
 * their rate, to make sure the clock tree can also support those rates.
 *
 * @param clk_hw Clock to check children for
 * @param new_rate Proposed new rate of the clock
 * @return 0 if all children can support the new rate, or negative value on error
 */
int clock_children_check_rate(const struct clk *clk_hw, clock_freq_t new_rate);

/**
 * @brief Check the rate of a given clock
 *
 * This function will check the current rate of the provided clock and
 * return it. Typically clock implementations can rely on the parent rate provided
 * to them, but this function can be used in cases outside of that where a clock
 * frequency needs to be read.
 *
 * @param clk_hw Clock to check the rate for
 * @return clock rate on success, or negative value on error
 */
clock_freq_t clock_management_clk_rate(const struct clk *clk_hw);

/**
 * @brief Determine the best rate a clock can produce, and optionally apply it
 *
 * This function is used to determine the best rate a clock can produce using
 * its parents. When @p commit is true, the rate is also applied to hardware.
 * The subsystem will call this automatically with the rate a user requests,
 * but clock drivers can also call it if they need to request a different rate
 * from their parent than what has been offered.
 *
 * @param clk_hw Clock to query/set rate for
 * @param rate_req Requested rate
 * @param commit if true, apply the rate to hardware; if false, only query
 * @return best possible rate on success, or negative value on error
 */
clock_freq_t clock_management_best_rate(const struct clk *clk_hw, clock_freq_t rate_req,
					bool commit);

/**
 * @brief Common function to validate a potential parent for a mux clock
 *
 * Validate a potential parent for a clock multiplexer. This function is
 * intended to be used by clock mux drivers that have a register layout
 * compatible with the parameters of this function, to avoid code duplication.
 * If the given parent is valid, this function returns the parent.
 */
int clock_management_mux_validate_parent(uint32_t parent_cnt, uint32_t parent);

/**
 * @brief Common function to get clock parent of a mux clock
 *
 * Get the current configured parent of a clock multiplexer. This function is
 * intended to be used by clock mux drivers that have a register layout
 * compatible with the parameters of this function, to avoid code duplication.
 * Manufacturers may place certain requirements on when and how the multiplexer
 * can be switched. These requirements are to be handled by the caller.
 * @param reg Register address of the multiplexer control register
 * @param mask_width Width of the multiplexer field in bits
 * @param mask_offset Bit offset of the multiplexer field within the register
 * @param parent_cnt Number of parents supported by the multiplexer
 * @return index of the current parent, or a negative errno on failure
 */
int clock_management_mux_get_parent(uintptr_t reg, uint8_t mask_width, uint8_t mask_offset,
				    uint32_t parent_cnt);

/**
 * @brief Common function to set clock parent of a mux clock
 *
 * Set the parent of a clock multiplexer. This function is intended to be used
 * by clock mux drivers that have a register layout compatible with the
 * parameters of this function, to avoid code duplication. Manufacturers may
 * place certain requirements on when and how the multiplexer can be switched.
 * These requirements are to be handled by the caller.
 * @param reg Register address of the multiplexer control register
 * @param mask_width Width of the multiplexer field in bits
 * @param mask_offset Bit offset of the multiplexer field within the register
 * @param parent_cnt Number of parents supported by the multiplexer
 * @param parent Index of the parent to set
 * @return 0 on success, or a negative errno on failure
 */
int clock_management_mux_set_parent(uintptr_t reg, uint8_t mask_width, uint8_t mask_offset,
				    uint32_t parent_cnt, uint32_t parent);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_HELPERS_H_ */
