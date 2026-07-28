/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IFX_CLOCK_SOURCE_BOARDS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IFX_CLOCK_SOURCE_BOARDS_H_

#if defined(CONFIG_SOC_SERIES_PSE84)
#include "ifx_clock_source_pse8xx.h"
#elif defined(CONFIG_SOC_SERIES_PSC3)
#include "ifx_clock_source_psc3xx.h"
#elif defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#include "ifx_clock_source_psoc4xx.h"
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IFX_CLOCK_SOURCE_BOARDS_H_ */
