/*
 * Copyright (c) 2025 Tenstorrent AI ULC
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
 * @brief Determine the best rate a clock can produce
 *
 * This function is used to determine the best rate a clock can produce using
 * its parents. The subsystem will call this automatically with the rate a user
 * requests, but clock drivers can also call it if they need to request a
 * different rate from their parent then what has been offered.
 *
 * @param clk_hw Clock to round rate for
 * @param rate_req Requested rate to round
 * @return best possible rate on success, or negative value on error
 */
clock_freq_t clock_management_round_rate(const struct clk *clk_hw, clock_freq_t rate_req);

/**
 * @brief Set the rate of a clock
 *
 * This function is used to set the rate of a clock. The subsystem will call this
 * automatically with the rate a user requests, but clock drivers can also call it
 * if they need to request a different rate from their parent then what has been
 * offered.
 *
 * @param clk_hw Clock to set rate for
 * @param rate_req Requested rate to set
 * @return rate clock is set to on success, or negative value on error
 */
clock_freq_t clock_management_set_rate(const struct clk *clk_hw, clock_freq_t rate_req);

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_MANAGEMENT_CLOCK_HELPERS_H_ */
