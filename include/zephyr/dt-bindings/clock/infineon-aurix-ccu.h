/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon AURIX clock control unit devicetree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_INFINEON_AURIX_CCU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_INFINEON_AURIX_CCU_H_

/**
 * @name Infineon AURIX clock domains
 * Identifiers for clocks = <&ccu AURIX_CCU_xxx>.
 * @{
 */
#define AURIX_CCU_SRI     0 /**< System resource interconnect, fSRI */
#define AURIX_CCU_SPB     1 /**< System peripheral bus, fSPB */
#define AURIX_CCU_FSI     2 /**< Flash interface, fFSI */
#define AURIX_CCU_STM     3 /**< System timer, fSTM */
#define AURIX_CCU_ASCLINF 4 /**< ASCLIN fast clock, fASCLINF */
#define AURIX_CCU_MCAN    5 /**< MCMCAN CAN clock, fMCAN */
#define AURIX_CCU_MCANH   6 /**< MCMCAN host clock, fMCANH */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_INFINEON_AURIX_CCU_H_ */
