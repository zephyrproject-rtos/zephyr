/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_TCC_CCU_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_TCC_CCU_H_

/** @brief Initializes the clock controller
 *
 */
void vcp_clock_init(void);

/** @brief Controls the PLL0PMS and PLL1PMS register
 *
 * @param id MPLL index.
 * @param rate Clock rate
 *
 * @return 0 on success
 */
int32_t clock_set_pll_rate(int32_t id, uint32_t rate);

/** @brief Gets the PLL rate
 *
 * @param id Clock index
 *
 * @return Clock rate
 */
uint32_t clock_get_pll_rate(int32_t id);

/** @brief Sets the PLL divisor value
 *
 * @param id Clock index
 * @param pll_div PLL divisor value
 *
 * @return 0 on success
 */
int32_t clock_set_pll_div(int32_t id, uint32_t pll_div);

/** @brief Controls the CPU_CLK_CTRL register
 *
 * @param id Clock index
 * @param rate Clock rate
 *
 * @return 0 on success
 */
int32_t clock_set_clk_ctrl_rate(int32_t id, uint32_t rate);

/** @brief Gets the clock rate for CPU_CLK_CTRL register
 *
 * @param id Clock index
 *
 * @return Clock rate
 */
uint32_t clock_get_clk_ctrl_rate(int32_t id);

/** @brief Checks the peripheral enable status
 *
 * @param id Clock index
 *
 * @return Clock rate
 */
int32_t clock_is_peri_enabled(int32_t id);

/** @brief Enables the peripheral clock
 *
 * @param id Clock index
 *
 * @return 0 on success
 */
int32_t clock_enable_peri(int32_t id);

/** @brief Disables the peripheral clock
 *
 * @param id Clock index
 *
 * @return 0 on success
 */
int32_t clock_disable_peri(int32_t id);

/** @brief Gets the peripheral clock rate
 *
 * @param id Clock index
 *
 * @return Clock rate
 */
uint32_t clock_get_peri_rate(int32_t id);

/** @brief Sets the peripheral clock rate (CLOCK_MCKC_PCLKCTRL register)
 *
 * @param id Clock index
 * @param rate Clock rate
 *
 * @return 0 on success
 */
int32_t clock_set_peri_rate(int32_t id, uint32_t rate);

/** @brief Gets the peripheral's IO bus clock power-down mask status (HCLK_MASK# register)
 *
 * @param id Bus clock index
 *
 * @return 0 on success
 */
int32_t clock_is_iobus_pwdn(int32_t id);

/** @brief Enables peripheral's IO bus clock power-down (HCLK_MASK# , SW_RESET# register)
 *
 * @param id Bus clock index
 * @param en Bus clock enable
 *
 * @return 0 on success
 */
int32_t clock_enable_iobus(int32_t id, bool en);

/** @brief Sets the peripheral's IO bus clock power-down mask status (HCLK_MASK# register)
 *
 * @param id Bus clock index
 * @param en Bus clock power-down enable
 *
 * @return 0 on success
 */
int32_t clock_set_iobus_pwdn(int32_t id, bool en);

/** @brief Controls the SW_RESET# register
 *
 * @param id Bus clock index
 * @param reset Bus clock reset enable
 *
 * @return 0 on success
 */
int32_t clock_set_sw_reset(int32_t id, bool reset);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_TCC_CCU_H_ */
