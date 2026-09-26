/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_NXP_IMX_IMX93_A55_SOC_H_
#define _SOC_NXP_IMX_IMX93_A55_SOC_H_

#include <stdint.h>

uint32_t common_clock_set_freq(uint32_t clock_name, uint32_t rate);
uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);

/**
 * @brief Program ARM_PLL to a core frequency.
 *
 * Runs the Cortex-A55 cluster off a step clock that is safe at the VDD_SOC
 * level set by the boot loader, reprograms ARM_PLL and selects ARM_PLL again
 * once it has locked.
 *
 * The clock select and ARM_PLL are shared by every Cortex-A55 core, so this
 * moves the whole cluster.
 *
 * @param frequency Core frequency in Hz. Only the operating points described by
 *                  the ARM PLL devicetree node can be selected.
 *
 * @retval 0 On success.
 * @retval -ENOTSUP No ARM_PLL operating point produces the frequency.
 */
int imx9_arm_pll_set_rate(uint32_t frequency);

#endif /* _SOC_NXP_IMX_IMX93_A55_SOC_H_ */
