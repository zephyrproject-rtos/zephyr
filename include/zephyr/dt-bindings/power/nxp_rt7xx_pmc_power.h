/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Named PMC power-rail identifiers for NXP RT7xx SoCs.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_POWER_NXP_RT7XX_PMC_POWER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_POWER_NXP_RT7XX_PMC_POWER_H_

/*
 * Named rail identifiers for use with
 *   nxp,power-rails = <&pmc0 NXP_RT7XX_PMC_*>;
 *
 * The PMC identifier space is a flat coordinate over the PDRUNCFG0..5
 * register array:
 *     NXP_PMC_POWER_ID(idx, bit) = idx * 32 + bit
 */

/** @brief Build a flat PMC rail id from PDRUNCFGn register index and bit. */
#define NXP_PMC_POWER_ID(pdruncfg_idx, bit) ((pdruncfg_idx) * 32 + (bit))

/* PMC peripheral array / periphery power rails (PDRUNCFG4 / PDRUNCFG5). */
/** @brief LCDIF Active Power Down (PDRUNCFG4 bit 21). */
#define NXP_RT7XX_PMC_APD_LCDIF NXP_PMC_POWER_ID(4, 21)
/** @brief LCDIF Periphery Power Down (PDRUNCFG5 bit 21). */
#define NXP_RT7XX_PMC_PPD_LCDIF NXP_PMC_POWER_ID(5, 21)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_POWER_NXP_RT7XX_PMC_POWER_H_ */
