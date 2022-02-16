/*
 * Copyright (c) 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_SOC_ARM_RENESAS_RCAR_COMMON_CLOCK_RCAR_H_
#define ZEPHYR_SOC_ARM_RENESAS_RCAR_COMMON_CLOCK_RCAR_H_

#include <dt-bindings/clock/renesas/renesas-cpg-mssr.h>

/**
 * @brief Enable or disable a core clock
 *
 * On success, the clock is enabled or disabled.
 *
 * @param base_address Clock controller base address
 * @param module Clock module number
 * @param rate Target rate for clock to be endisable
 * @param enable 1 to enable a clock, 0 to disable it
 *
 * @retval 0 on success
 * @retval -EINVAL if clock or rate not supported
 */
int soc_cpg_core_clock_endisable(uint32_t base_address, uint32_t module,
				 uint32_t rate, bool enable);

/**
 * @brief Obtain the clock rate of a given core clock
 *
 * @param base_address Clock controller base address
 * @param module Clock module number
 * @param[out] rate Clock rate
 *
 * @retval 0 on success
 * @retval -ENOTSUP if clock not supported
 */
int soc_cpg_get_rate(uint32_t base_address, uint32_t module, uint32_t *rate);

/**
 * @brief Get MSTPCR register offset for a given core clock
 *
 * @param reg Clock module reg index
 * @param[out] offset MSTPCR offset
 *
 * @retval 0 on success
 * @retval -EINVAL if out of range
 */
int soc_cpg_get_mstpcr_offset(uint32_t reg, uint16_t *offset);

/**
 * @brief Get SRCR register offset for a given core clock
 *
 * @param reg Clock module reg index
 * @param[out] offset SRCR offset
 *
 * @retval 0 on success
 * @retval -EINVAL if out of range
 */
int soc_cpg_get_srcr_offset(uint32_t reg, uint16_t *offset);

#endif /* ZEPHYR_SOC_ARM_RENESAS_RCAR_COMMON_CLOCK_RCAR_H_ */
